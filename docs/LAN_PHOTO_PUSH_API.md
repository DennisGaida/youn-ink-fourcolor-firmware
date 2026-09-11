# LAN Photo Push API

This document records the HTTP interface exposed by the 2BP album firmware in LAN Wi-Fi mode, so a NAS, script, or other LAN service can push images to the device on a schedule.

## Prerequisites

The device must first connect to LAN Wi-Fi, then turn on `LAN Service` on the device settings page. Once enabled, the settings page will show the device's current LAN IP, for example:

```text
192.168.110.238
```

All subsequent endpoints use this IP:

```text
http://192.168.110.238
```

With the LAN service on, visiting `http://<device-IP>/` in a browser opens the image management page. A NAS or script can also call the API below directly.

## Image Format Requirements

The `/upload` endpoint does not accept common image files (JPG, PNG, WEBP) directly — it expects already-converted raw screen pixel data.

The device screen size is fixed at:

```text
400 x 300
```

Two upload formats are supported:

| format | meaning | file size |
| --- | --- | --- |
| `1bpp` | black/white, 1 bit per pixel | `15000 bytes` |
| `bwry2bpp` or `2bpp` | black/white/yellow/red four-color, 2 bits per pixel | `30000 bytes` |

If pushing regular images from a NAS on a schedule, first convert JPG/PNG to the bin format above on the NAS side, then call `/upload`.

## Quick Status Check

```bash
curl "http://192.168.110.238/status"
```

Example success response:

```json
{
  "status": "ready",
  "mode": "lan",
  "ip": "192.168.110.238",
  "url": "http://192.168.110.238/"
}
```

`mode=lan` means the LAN HTTP service is currently active; `mode=ap` means AP photo-transfer mode.

## Upload an Image

Upload a 2BP four-color image:

```bash
curl -X POST \
  "http://192.168.110.238/upload?format=bwry2bpp" \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@/path/to/image_400x300_2bpp.bin"
```

Upload a 1BP black/white image:

```bash
curl -X POST \
  "http://192.168.110.238/upload?format=1bpp" \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@/path/to/image_400x300_1bpp.bin"
```

Example success response:

```json
{
  "success": true,
  "id": "ap12345678901"
}
```

Common failure causes:

| Cause | Symptom |
| --- | --- |
| Wrong file size | Returns `needs 400x300 2bpp four-color data` or `needs 400x300 1bpp data` |
| Device HTTP service not enabled | NAS cannot connect to the device IP |
| IP changed | Re-read the LAN IP shown on the device settings page |
| Album storage full | Device fails to save the image |

## List Images

```bash
curl "http://192.168.110.238/photos"
```

Example response:

```json
{
  "photos": [
    {
      "id": "ap12345678901",
      "title": "WiFi Four-Color Photo",
      "date": "2026-05-21",
      "location": "WiFi AP",
      "body": "Phone WiFi transfer · 2 BP four-color",
      "width": 400,
      "height": 300,
      "size": 30000,
      "format": "bwry2bpp"
    }
  ]
}
```

Field reference:

| Field | Meaning |
| --- | --- |
| `id` | Image ID, used for subsequent read/delete/edit |
| `title` | Image title |
| `date` | Date string |
| `location` | Location |
| `body` | Description |
| `width` / `height` | Image dimensions |
| `size` | Raw data size |
| `format` | `1bpp` or `bwry2bpp` |

## Download Raw Image Data

```bash
curl \
  "http://192.168.110.238/photo?id=ap12345678901" \
  --output image.bin
```

Returns the raw bin data as it was saved for this image.

## Delete an Image

```bash
curl -X DELETE \
  "http://192.168.110.238/photo?id=ap12345678901"
```

Success response:

```json
{"success":true}
```

## Update Image Metadata

```bash
curl -X POST "http://192.168.110.238/photo/meta" \
  -H "Content-Type: application/json" \
  -d '{
    "id": "ap12345678901",
    "title": "Daily Photo",
    "date": "2026-05-21",
    "location": "NAS",
    "body": "NAS daily auto-push"
  }'
```

Success response:

```json
{"success":true}
```

## Reorder Images

Move up one position:

```bash
curl -X POST "http://192.168.110.238/photos/move" \
  -H "Content-Type: application/json" \
  -d '{"id":"ap12345678901","delta":-1}'
```

Move down one position:

```bash
curl -X POST "http://192.168.110.238/photos/move" \
  -H "Content-Type: application/json" \
  -d '{"id":"ap12345678901","delta":1}'
```

## Configure Slideshow Interval

Query the current slideshow settings:

```bash
curl "http://192.168.110.238/settings"
```

Example response:

```json
{
  "success": true,
  "slideshow_interval": 5,
  "service_running": true,
  "mode": "lan",
  "ip": "192.168.110.238",
  "url": "http://192.168.110.238/"
}
```

Set the slideshow interval:

```bash
curl -X POST "http://192.168.110.238/settings" \
  -H "Content-Type: application/json" \
  -d '{"slideshow_interval":5}'
```

Turn off the local HTTP service:

```bash
curl -X POST "http://192.168.110.238/settings" \
  -H "Content-Type: application/json" \
  -d '{"service_enabled":false}'
```

Turn off the service, turn off Wi-Fi, and enter power-save mode immediately:

```bash
curl -X POST "http://192.168.110.238/settings" \
  -H "Content-Type: application/json" \
  -d '{"service_enabled":false,"wifi_enabled":false,"sleep":true}'
```

Supported values:

| Value | Meaning |
| --- | --- |
| `0` | Slideshow off |
| `5` | 5 minutes |
| `10` | 10 minutes |
| `30` | 30 minutes |

## Example NAS Cron Job

This directory provides a reusable conversion script:

```text
docs/inkscreen_image_converter.js
```

It's extracted from the conversion algorithm in the device management HTML page, used to convert regular images into the raw bin format the device's `/upload` endpoint expects. CLI mode relies on `sharp` to decode and resize images:

```bash
npm install sharp
node docs/inkscreen_image_converter.js input.jpg daily_400x300_2bpp.bin bwry2bpp
```

It can also generate black/white 1BP:

```bash
node docs/inkscreen_image_converter.js input.jpg daily_400x300_1bpp.bin 1bpp
```

Assuming a NAS has already generated a `daily_400x300_2bpp.bin`, cron can push it once a day:

```bash
#!/bin/sh
DEVICE="192.168.110.238"
BIN="/volume1/photo/daily_400x300_2bpp.bin"

curl -fsS -X POST \
  "http://${DEVICE}/upload?format=bwry2bpp" \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@${BIN}"
```

If you want to add a description after uploading, parse the returned `id` and then call `/photo/meta`. For example:

```bash
#!/bin/sh
DEVICE="192.168.110.238"
BIN="/volume1/photo/daily_400x300_2bpp.bin"

RESP=$(curl -fsS -X POST \
  "http://${DEVICE}/upload?format=bwry2bpp" \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@${BIN}")

ID=$(printf "%s" "$RESP" | sed -n 's/.*"id":"\([^"]*\)".*/\1/p')

if [ -n "$ID" ]; then
  TODAY=$(date +%F)
  curl -fsS -X POST "http://${DEVICE}/photo/meta" \
    -H "Content-Type: application/json" \
    -d "{\"id\":\"${ID}\",\"title\":\"Daily Photo\",\"date\":\"${TODAY}\",\"location\":\"NAS\",\"body\":\"NAS auto-push\"}"
fi
```

## Suggested Future Enhancements

The API currently supports scheduled NAS pushes, but letting a NAS upload JPG/PNG directly would need an additional conversion step, either on the firmware side or the NAS side.

Recommended approach:

1. Convert on the NAS side: use a script on the NAS to convert JPG/PNG to `400x300 bwry2bpp bin`, then call `/upload`. This is the most memory-efficient option for the device.
2. Add `/upload-image` on the firmware side: the device receives JPG/PNG directly and converts it. Easier to develop, but higher memory and decoding cost on the ESP32 side.

Option 1 is currently recommended.
