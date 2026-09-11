#ifndef PAGE_RENDERER_H
#define PAGE_RENDERER_H

#include <lvgl.h>

namespace ui {

// Button event structure
struct ButtonEvent {
    enum Type {
        kUpClick,
        kDownClick,
        kUpDoubleClick,
        kDownDoubleClick,
        kUpLongPress,
        kDownLongPress,
        kBootClick,
        kBootDoubleClick,
        kBootLongPress,
    };
    Type type;
};

// PageRenderer base class interface (Spec §1)
// All page renderers must implement this interface
class PageRenderer {
public:
    virtual ~PageRenderer() = default;

    // Create the page UI (called within the LVGL lock)
    virtual void Create(lv_obj_t* parent) = 0;

    // Destroy the page UI (called within the LVGL lock)
    virtual void Destroy() = 0;

    // Update the page data (called within the LVGL lock)
    virtual void Update() = 0;

    // Handle an input event
    // Returns true if the event was consumed
    virtual bool HandleInput(const ButtonEvent& event) = 0;

    // Get the root object
    virtual lv_obj_t* root() const = 0;

    // Append streaming text to the current bubble (used for LLM streaming)
    // Returns true if the append succeeded
    virtual bool AppendText(const char* chunk) { (void)chunk; return false; }

    // Begin a new streaming response (clears the current bubble or creates a new one)
    virtual void BeginStream() {}

    // End the streaming response
    virtual void EndStream() {}
};

}  // namespace ui

#endif  // PAGE_RENDERER_H
