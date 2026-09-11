#ifndef WIFI_RENDERER_H
#define WIFI_RENDERER_H

#include <lvgl.h>
#include <string>

namespace ui {

// WiFi status visualization (Spec §5)
// Three states: connecting, connected, disconnected

enum class WifiState {
    Connecting,   // Connecting: WiFi icon blinking + "Connecting..." + progress bar
    Connected,    // Connected: solid WiFi icon + SSID + signal strength
    Disconnected, // Disconnected: WiFi icon with an X + "Disconnected" + "Press BOOT to reconnect"
};

struct WifiStatus {
    WifiState state = WifiState::Disconnected;
    std::string ssid;
    int signal_strength = 0;   // dBm (typically -30 to -90)
    int progress = 0;          // Connection progress (0-100)
    bool server_connected = false;
    std::string server_uri;
};

class WifiRenderer {
public:
    WifiRenderer();
    ~WifiRenderer();

    // Create the WiFi status panel
    void Create(lv_obj_t* parent, int x, int y, int w, int h);

    // Update the display
    void Update(const WifiStatus& status);

    // Get the root object
    lv_obj_t* root() const { return panel_; }

    // Show/hide
    void Show();
    void Hide();

    // Stop the blinking animation
    void StopBlinking();

private:
    void RenderConnecting(const WifiStatus& status);
    void RenderConnected(const WifiStatus& status);
    void RenderDisconnected(const WifiStatus& status);

    // Get the WiFi signal strength icon
    const char* GetWifiIcon(int signal_dbm);

    // Convert dBm to a percentage
    int SignalToPercent(int dbm) const;

    lv_obj_t* panel_ = nullptr;
    lv_obj_t* wifi_icon_ = nullptr;
    lv_obj_t* status_label_ = nullptr;
    lv_obj_t* ssid_label_ = nullptr;
    lv_obj_t* progress_bar_ = nullptr;
    lv_obj_t* hint_label_ = nullptr;
    lv_obj_t* server_label_ = nullptr;

    WifiState current_state_ = WifiState::Disconnected;
    bool is_blinking_ = false;
};

}  // namespace ui

#endif  // WIFI_RENDERER_H
