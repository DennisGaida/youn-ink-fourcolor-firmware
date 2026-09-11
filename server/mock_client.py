#!/usr/bin/env python3
"""
mock_client.py — simulates the ESP32 firmware for end-to-end testing against llmserve.py

Usage:
    python3 mock_client.py                          # default localhost:9001
    python3 mock_client.py --server ws://192.168.1.100:9001  # specify server

Test flow:
    1. (optional) UDP Discovery
    2. WebSocket connection + hello handshake
    3. Send ptt_start + simulated PCM16 audio
    4. Send ptt_stop
    5. Receive and print all server responses (ASR → LLM → TTS → summary)
"""

import asyncio
import json
import logging
import socket
import struct
import sys
import time
import wave
from pathlib import Path

import websockets

logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
logger = logging.getLogger("mock_client")

# ─── Configuration ───────────────────────────────────────────────
DEFAULT_SERVER = "ws://127.0.0.1:9001"
DISCOVERY_PORT = 8766
SIMULATED_AUDIO_DURATION_MS = 2000  # simulate 2 seconds of recording
SAMPLE_RATE = 16000
CHANNELS = 1
BITS_PER_SAMPLE = 16


def generate_dummy_audio(duration_ms: int) -> bytes:
    """Generate simulated PCM16 16kHz mono audio (silence + light noise)"""
    import random
    num_samples = int(duration_ms * SAMPLE_RATE / 1000)
    # Semi-silence (low-amplitude noise simulating speech), so ASR doesn't immediately return empty
    data = struct.pack(f"<{num_samples}h", *[random.randint(-100, 100) for _ in range(num_samples)])
    return data


def sign_discovery_reply(host_id, host_name, ws_url, nonce, secret):
    """Generate an HMAC-SHA256 signature"""
    import hmac
    import hashlib
    message = f"discover_reply|{host_id}|{host_name}|{ws_url}|{nonce}"
    return hmac.new(
        secret.encode("utf-8"),
        message.encode("utf-8"),
        hashlib.sha256
    ).hexdigest()


async def run_discovery() -> str | None:
    """UDP device discovery; returns wsUrl or None"""
    logger.info("📡 Sending UDP discover_host...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
    sock.settimeout(3.0)

    request = json.dumps({
        "type": "discover_host",
        "service": "vibecoding-voice",
        "deviceId": "mock-client-001",
        "nonce": "mock-nonce-1234",
    }).encode("utf-8")

    sock.sendto(request, ("255.255.255.255", DISCOVERY_PORT))

    try:
        data, addr = sock.recvfrom(1024)
        reply = json.loads(data.decode("utf-8"))
        logger.info("📡 Discovery reply from %s: %s", addr, json.dumps(reply, ensure_ascii=False))
        return reply.get("wsUrl")
    except socket.timeout:
        logger.warning("📡 Discovery timed out")
        return None
    finally:
        sock.close()


async def run_full_test(server_url: str):
    """Run the full PTT → ASR → LLM → TTS test flow"""

    results = {
        "hello_ack": False,
        "server_ready": False,
        "asr_result": None,
        "llm_chunks": 0,
        "llm_full_text": "",
        "tts_audio_received": False,
        "cli_summary": False,
        "intent_response": False,
        "intent_actions": [],
        "errors": [],
        "timing": {},
    }

    t0 = time.time()

    try:
        logger.info(f"🔌 Connecting WebSocket: {server_url}")
        async with websockets.connect(server_url) as ws:

            # ── Step 1: Hello handshake ──
            hello = json.dumps({
                "type": "hello",
                "deviceId": "mock-client-001",
                "boardType": "mock-test",
            })
            await ws.send(hello)
            logger.info("📨 Sent hello")

            # ── Step 2: Receive hello_ack + server_ready ──
            for _ in range(5):
                msg = await asyncio.wait_for(ws.recv(), timeout=10)
                if isinstance(msg, str):
                    data = json.loads(msg)
                    msg_type = data.get("type", "")
                    logger.info(f"📥 {msg_type}: {json.dumps(data, ensure_ascii=False)[:200]}")

                    if msg_type == "hello_ack":
                        results["hello_ack"] = True
                    elif msg_type == "server_ready":
                        results["server_ready"] = True
                        break

            if not results["hello_ack"]:
                results["errors"].append("hello_ack not received")
                return results
            if not results["server_ready"]:
                results["errors"].append("server_ready not received")
                return results

            results["timing"]["hello_ms"] = int((time.time() - t0) * 1000)
            logger.info(f"✅ Handshake succeeded ({results['timing']['hello_ms']}ms)")

            # ── Step 3: PTT start ──
            ptt_start = json.dumps({
                "type": "ptt_start",
                "deviceId": "mock-client-001",
            })
            await ws.send(ptt_start)
            logger.info("📨 Sent ptt_start")

            # ── Step 4: Send simulated audio ──
            audio_data = generate_dummy_audio(SIMULATED_AUDIO_DURATION_MS)
            chunk_size = 3200  # matches llmserve CHUNK_SIZE
            for i in range(0, len(audio_data), chunk_size):
                chunk = audio_data[i:i + chunk_size]
                await ws.send(chunk)
                await asyncio.sleep(0.05)  # simulate real-time sending
            logger.info(f"📨 Sent audio: {len(audio_data)} bytes ({SIMULATED_AUDIO_DURATION_MS}ms)")

            # ── Step 5: PTT stop ──
            ptt_stop = json.dumps({
                "type": "ptt_stop",
                "duration_ms": SIMULATED_AUDIO_DURATION_MS,
            })
            await ws.send(ptt_stop)
            logger.info("📨 Sent ptt_stop")

            results["timing"]["ptt_cycle_start"] = int((time.time() - t0) * 1000)

            # ── Step 6: Receive responses ──
            llm_done_received = False
            max_wait = 60  # wait at most 60 seconds
            response_timeout = time.time() + max_wait

            while time.time() < response_timeout:
                try:
                    msg = await asyncio.wait_for(ws.recv(), timeout=3)
                except asyncio.TimeoutError:
                    break

                if isinstance(msg, bytes):
                    # TTS audio frame: 2-byte header length + JSON header + PCM data
                    if len(msg) < 4:
                        continue
                    header_len = struct.unpack(">H", msg[:2])[0]
                    json_header = msg[2:2 + header_len]
                    try:
                        header = json.loads(json_header.decode("utf-8"))
                        if header.get("type") == "tts_audio":
                            pcm_len = len(msg) - 2 - header_len
                            results["tts_audio_received"] = True
                            logger.info(f"🎵 TTS audio: {pcm_len} bytes PCM ({pcm_len // 2} samples)")

                            # Save the TTS audio to a WAV file
                            save_tts_wav(msg, header_len)
                    except json.JSONDecodeError:
                        pass
                    continue

                if isinstance(msg, str):
                    data = json.loads(msg)
                    msg_type = data.get("type", "")
                    truncated = json.dumps(data, ensure_ascii=False)[:200]
                    logger.info(f"📥 {msg_type}: {truncated}")

                    if msg_type == "status":
                        status = data.get("status", "")
                        if status == "recording":
                            logger.info("🎤 Server confirms: recording")
                        elif status == "processing":
                            logger.info("⚙️ Server confirms: processing")

                    elif msg_type == "asr_interim":
                        logger.info(f"🗣️  ASR interim result: {data.get('text', '')}")

                    elif msg_type == "transcript_final":
                        results["asrresult"] = data.get("text", "")
                        logger.info(f"✅ ASR final result: {results['asrresult']}")

                    elif msg_type == "asr_final":
                        results["asrresult"] = data.get("text", "")
                        logger.info(f"✅ ASR final result: {results['asrresult']}")

                    elif msg_type == "cli_summary":
                        results["llm_chunks"] += 1
                        results["cli_summary"] = True
                        assistant_text = data.get("latestAssistantText", "")
                        results["llm_full_text"] = assistant_text
                        if data.get("done"):
                            llm_done_received = True
                            logger.info(f"✅ LLM done: {len(assistant_text)} characters")

                    elif msg_type == "llm_done":
                        results["llm_full_text"] = data.get("full_text", "")
                        llm_done_received = True
                        logger.info(f"✅ LLM done: {len(results['llm_full_text'])} characters")

                    elif msg_type == "error":
                        results["errors"].append(data.get("message", "unknown error"))
                        logger.error(f"❌ Server error: {data.get('message')}")

                    elif msg_type == "pong":
                        pass  # ignore pong

                    elif msg_type == "intent_response":
                        results["intent_response"] = True
                        display_text = data.get("displayText", "")
                        actions = data.get("actions", [])
                        results["intent_actions"] = actions
                        results["llm_full_text"] = display_text
                        logger.info(f"🎯 Intent: displayText='{display_text}', {len(actions)} actions")
                        for action in actions:
                            logger.info(f"   → {action.get('action', 'unknown')}")
                        llm_done_received = True  # intent_response means completion

            results["timing"]["total_ms"] = int((time.time() - t0) * 1000)

    except websockets.exceptions.ConnectionClosed as e:
        results["errors"].append(f"Connection closed: {e}")
    except Exception as e:
        results["errors"].append(f"Exception: {e}")

    return results


def save_tts_wav(frame: bytes, header_len: int):
    """Save a TTS PCM frame as a WAV file"""
    pcm_data = frame[2 + header_len:]
    if len(pcm_data) < 2:
        return

    filename = f"/tmp/mock_tts_output_{int(time.time())}.wav"
    try:
        with wave.open(filename, "wb") as wf:
            wf.setnchannels(1)
            wf.setsampwidth(2)  # 16-bit
            wf.setframerate(16000)
            wf.writeframes(pcm_data)
        logger.info(f"🎵 TTS WAV saved: {filename}")
    except Exception as e:
        logger.warning(f"Failed to save TTS WAV: {e}")


def print_test_report(results: dict):
    """Print the test report"""
    print("\n" + "=" * 60)
    print("  End-to-end test report")
    print("=" * 60)

    checks = [
        ("Hello handshake", results["hello_ack"]),
        ("Server Ready", results["server_ready"]),
        ("ASR recognition", results["asrresult"] is not None and len(results["asrresult"]) > 0),
        ("LLM response", results["llm_chunks"] > 0 or len(results["llm_full_text"]) > 0),
        ("CLI Summary / Intent", results["cli_summary"] or results["intent_response"]),
        ("TTS audio", results["tts_audio_received"]),
    ]

    all_pass = True
    for name, passed in checks:
        status = "✅ PASS" if passed else "❌ FAIL"
        if not passed:
            all_pass = False
        print(f"  {status}  {name}")

    if results["errors"]:
        print(f"\n  ⚠️  Errors ({len(results['errors'])}):")
        for err in results["errors"]:
            print(f"    - {err}")

    print(f"\n  ⏱️  Timing:")
    if "hello_ms" in results["timing"]:
        print(f"    Handshake: {results['timing']['hello_ms']}ms")
    if "ptt_cycle_start" in results["timing"]:
        print(f"    PTT cycle start: {results['timing']['ptt_cycle_start']}ms")
    if "total_ms" in results["timing"]:
        print(f"    Total time: {results['timing']['total_ms']}ms")

    print("=" * 60)
    if all_pass:
        print("  🎉 All checks passed!")
    else:
        print("  ❌ Some checks failed, see error info above")
    print("=" * 60 + "\n")


def main():
    server_url = DEFAULT_SERVER
    if "--server" in sys.argv:
        idx = sys.argv.index("--server")
        if idx + 1 < len(sys.argv):
            server_url = sys.argv[idx + 1]

    print(f"Mock Client — target server: {server_url}")
    print(f"Simulated audio: {SIMULATED_AUDIO_DURATION_MS}ms PCM16 16kHz")
    print()

    # Attempt UDP discovery
    try:
        import socket as _socket  # noqa: F811
        discovered_url = asyncio.run(run_discovery())
        if discovered_url:
            server_url = discovered_url
            print(f"Using discovered address: {server_url}")
        else:
            print(f"Using default address: {server_url}")
    except Exception as e:
        logger.warning(f"Discovery failed: {e}, using default address")
        print(f"Using default address: {server_url}")

    print()

    # Run the full test
    results = asyncio.run(run_full_test(server_url))
    print_test_report(results)

    # Exit code
    has_errors = bool(results["errors"])
    missing_core = not results["hello_ack"] or not results["server_ready"]
    sys.exit(1 if has_errors or missing_core else 0)


if __name__ == "__main__":
    main()
