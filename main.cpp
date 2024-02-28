#include <cpprealm/sdk.hpp>
#include <string>

#include "ftxui/component/component.hpp"  // for Checkbox, Renderer, Horizontal, Vertical, Input, Menu, Radiobox, ResizableSplitLeft, Tab
#include "ftxui/component/component_base.hpp"  // for ComponentBase, Component
#include "ftxui/component/component_options.hpp"  // for MenuOption, InputOption
#include "ftxui/component/event.hpp"              // for Event, Event::Custom
#include "ftxui/component/screen_interactive.hpp"  // for Component, ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for text, color, operator|, bgcolor, filler, Element, vbox, size, hbox, separator, flex, window, graph, EQUAL, paragraph, WIDTH, hcenter, Elements, bold, vscroll_indicator, HEIGHT, flexbox, hflow, border, frame, flex_grow, gauge, paragraphAlignCenter, paragraphAlignJustify, paragraphAlignLeft, paragraphAlignRight, dim, spinner, LESS_THAN, center, yframe, GREATER_THAN
#include "ftxui/dom/flexbox_config.hpp"  // for FlexboxConfig
#include "ftxui/dom/node.hpp"
#include "ftxui/dom/table.hpp"
#include "ftxui/screen/color.hpp"  // for Color, Color::BlueLight, Color::RedLight, Color::Black, Color::Blue, Color::Cyan, Color::CyanLight, Color::GrayDark, Color::GrayLight, Color::Green, Color::GreenLight, Color::Magenta, Color::MagentaLight, Color::Red, Color::White, Color::Yellow, Color::YellowLight, Color::Default, Color::Palette256, ftxui
#include "ftxui/screen/color_info.hpp"  // for ColorInfo
#include "ftxui/screen/screen.hpp"      // for Size, Dimensions
#include "ftxui/screen/terminal.hpp"    // for Size, Dimensions
#include "scroller.hpp"

using namespace ftxui;

namespace realm {

    struct Item {
        realm::primary_key<realm::object_id> _id{realm::object_id::generate()};
        bool isComplete;
        std::string summary;
        std::string owner_id;
    };
    REALM_SCHEMA(Item, _id, isComplete, summary, owner_id)

}  // namespace realm

void createItem(std::string summary, bool isComplete, realm::db database);
ftxui::Component makeItemRow(realm::managed<realm::Item> item, realm::db database);
void deleteItem(realm::managed<realm::Item> itemToDelete, realm::db database);
void markComplete(realm::managed<realm::Item> itemToMarkComplete, realm::db database);

int main() {
    auto screen = ftxui::ScreenInteractive::FitComponent();

    // Open a database at a path
    auto relative_realm_path_directory = "db/";
    std::filesystem::create_directories(relative_realm_path_directory);
    std::filesystem::path path = std::filesystem::current_path().append(relative_realm_path_directory);
    path = path.append("item_objects");
    path = path.replace_extension("realm");
    auto config = realm::db_config();
    config.set_path(path);
    auto database = realm::db(std::move(config));
    auto items = database.objects<realm::Item>();

    // Add a new Item
    std::string newTaskSummary = "";
    auto inputNewTaskSummary =
            Input(&newTaskSummary, "Enter new task summary");

    auto newTaskIsComplete = false;
    auto newTaskCompletionStatus = Checkbox(" Complete ", &newTaskIsComplete);

    std::string saveButtonLabel = " Save ";
    auto saveButton = Button(&saveButtonLabel, [&]{
            createItem(newTaskSummary, newTaskIsComplete, database);
    });

    auto newTaskLayout = Container::Horizontal(
            {inputNewTaskSummary, newTaskCompletionStatus, saveButton});

    auto newTaskRenderer = Renderer(newTaskLayout, [&] {
        auto content = hbox({
                                    inputNewTaskSummary->Render() | flex,
                                    newTaskCompletionStatus->Render() | center,
                                    saveButton->Render(),
                            }) | size(WIDTH, GREATER_THAN, 80) | center;
        return window(text(L" Add a new task "), content);
    });

    // Lay out and render scrollable task list
    auto renderTasks = Renderer([&] {
        Elements tasks;
        for (int i = 0; i < items.size(); i++) {
            std::string completionString = (items[i].isComplete) ? " Complete " : " Incomplete ";
            std::string mineOrNot = (items[i].owner_id == "me") ? " Mine " : " Them ";
            auto taskRow = hbox({
                                        text(items[i].summary) | flex,
                                        align_right(text(completionString)),
                                        align_right(text(mineOrNot))
            }) | size(WIDTH, GREATER_THAN, 80);
            tasks.push_back(taskRow);
        }
        auto content = vbox(std::move(tasks));
        return content;
    });

    auto scroller = Scroller(renderTasks);

    scroller = Renderer(scroller, [scroller] {
        return vbox({
                            scroller->Render() | flex,
                    });
    });

    auto scrollerContainer = scroller;
    scrollerContainer =
            Renderer(scrollerContainer, [scrollerContainer] { return scrollerContainer->Render() | flex; });

    scrollerContainer = CatchEvent(scrollerContainer, [&](Event event) {
        if (event == Event::Character('d')) {
            // Get index of selected item in the scroller
//            auto scrollerIndex = scroller.getScrollerIndex();
//            auto managedItemAtIndex = items[scrollerIndex];
//            deleteItem(managedItemAtIndex, database);
            return true;
        }

//        if (event == Event::Character('c')) {
//            auto scrollerIndex = scroller.getScrollerIndex();
//            auto managedItemAtIndex = items[scrollerIndex];
//            markComplete(managedItemAtIndex, database);
//            return true;
//        }

        if (event == Event::Character('q') || event == Event::Escape) {
            screen.ExitLoopClosure()();
            return true;
        }

        return false;
    });

    // Lay out and render dashboard
    ftxui::Element taskTableHeaderRow = hbox({
                                                     text(L" Summary ") | flex | bold,
                                                     align_right(text(L" Status ")),
                                                     align_right(text(L" Owner ")),
                                             });

    auto dashboardLayout = Container::Vertical({
        scrollerContainer, newTaskLayout
    });

    auto mainRenderer = Renderer(dashboardLayout, [&] {
        auto content =
                vbox({
                    taskTableHeaderRow,
                    separator(),
                    scrollerContainer->Render(),
                    separator(),
                    hbox({
                        inputNewTaskSummary->Render() | flex,
                        newTaskCompletionStatus->Render() | center,
                        saveButton->Render(),
                        }) | size(WIDTH, GREATER_THAN, 80)
                }) | center;;
        return window(text(L" Todo Tracker "), content);
    });

    try {
        screen.Loop(mainRenderer);
        //screen.Loop(scroller);
    } catch(...) {
        std::cout << "The app crashed with an error." << std::endl;
    }
    return 0;
}

void createItem(std::string summary, bool isComplete, realm::db database)
{
    auto item = realm::Item{
        .isComplete = isComplete,
        .summary = summary,
        .owner_id = "me"
    };

    database.write([&item, &database]{
        database.add(std::move(item));
    });
};

void deleteItem(realm::managed<realm::Item> itemToDelete, realm::db database) {
    database.write( [&itemToDelete, &database]{
        database.remove(itemToDelete);
    });
};

void markComplete(realm::managed<realm::Item> itemToMarkComplete, realm::db database) {
    database.write( [&itemToMarkComplete, &database]{
        itemToMarkComplete.isComplete = true;
    });
};
