#ifndef TODO_PAGE_H
#define TODO_PAGE_H

#include <lvgl.h>
#include <string>
#include <vector>

namespace ui {

// Todo task item
struct TodoItem {
    std::string text;
    bool completed = false;
};

// Todo page - task list
class TodoPage {
public:
    TodoPage(lv_obj_t* parent);
    ~TodoPage();

    // Clear the list
    void Clear();

    // Set the task list
    void SetItems(const std::vector<TodoItem>& items);

    // Add a task
    void AddItem(const std::string& text, bool completed = false);

    // Update task status
    void UpdateItem(int index, bool completed);

    // Remove a task
    void RemoveItem(int index);

    // Refresh display
    void Refresh();

private:
    lv_obj_t* list_ = nullptr;        // lv_list container
    std::vector<lv_obj_t*> items_;    // List item widgets (button objects)
    std::vector<std::string> texts_;  // Original text (without prefix)
};

}  // namespace ui

#endif  // TODO_PAGE_H