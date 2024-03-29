#include <algorithm>                           // for max, min
#include <ftxui/component/component_base.hpp>  // for Component, ComponentBase
#include <ftxui/component/event.hpp>  // for Event, Event::ArrowDown, Event::ArrowUp, Event::End, Event::Home, Event::PageDown, Event::PageUp
#include <memory>   // for shared_ptr, allocator, __shared_ptr_access
#include <utility>  // for move

#include "ftxui/component/component.hpp"  // for Make
#include "ftxui/component/mouse.hpp"  // for Mouse, Mouse::WheelDown, Mouse::WheelUp
#include "ftxui/dom/deprecated.hpp"  // for text
#include "ftxui/dom/elements.hpp"  // for operator|, Element, size, vbox, EQUAL, HEIGHT, dbox, reflect, focus, inverted, nothing, select, vscroll_indicator, yflex, yframe

#include "scroller.hpp"

ScrollerBase::ScrollerBase(ftxui::Component child) { Add(child); }

int ScrollerBase::getScrollerIndex() const {
    return indexOfSelectedItem;
}

ftxui::Element ScrollerBase::Render() {
    auto focused = Focused() ? ftxui::focus : ftxui::select;
    auto style = Focused() ? ftxui::inverted : ftxui::nothing;

    ftxui::Element background = ComponentBase::Render();
    background->ComputeRequirement();
    size_ = background->requirement().min_y;
    return ftxui::dbox({
                               std::move(background),
                               ftxui::vbox({
                                                   ftxui::text(L"") |
                                                   size(ftxui::HEIGHT, ftxui::EQUAL, indexOfSelectedItem),
                                                   ftxui::text(L"") | style | focused,
                                           }),
                       }) |
           ftxui::vscroll_indicator | ftxui::yframe | ftxui::yflex | reflect(box_);
}

bool ScrollerBase::OnEvent(ftxui::Event event) {
    if (event.is_mouse() && box_.Contain(event.mouse().x, event.mouse().y))
        TakeFocus();

    int selected_old = indexOfSelectedItem;
    if (event == ftxui::Event::ArrowUp || event == ftxui::Event::Character('k') ||
        (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelUp)) {
        indexOfSelectedItem--;
    }
    if ((event == ftxui::Event::ArrowDown || event == ftxui::Event::Character('j') ||
         (event.is_mouse() && event.mouse().button == ftxui::Mouse::WheelDown))) {
        indexOfSelectedItem++;
    }
    if (event == ftxui::Event::PageDown)
        indexOfSelectedItem += box_.y_max - box_.y_min;
    if (event == ftxui::Event::PageUp)
        indexOfSelectedItem -= box_.y_max - box_.y_min;
    if (event == ftxui::Event::Home)
        indexOfSelectedItem = 0;
    if (event == ftxui::Event::End)
        indexOfSelectedItem = size_;

    indexOfSelectedItem = std::max(0, std::min(size_ - 1, indexOfSelectedItem));
    //setSelectedItem
    return selected_old != indexOfSelectedItem;
}

bool ScrollerBase::Focusable() const { return true; }

std::shared_ptr<ScrollerBase> Scroller(ftxui::Component child) {
    return ftxui::Make<ScrollerBase>(std::move(child));
}

// Based on Arthur Sonzogni/git-tui (MIT License)