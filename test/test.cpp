#include <cstdio>
#include <iostream>
#include <thread>
#include <chrono>
#include "GL_Commdlg.hpp"
// #define COLOR_PICKER_IMPLEMENTATION
// #include "new_test.hpp"

// ============================================================
// GL_Commdlg.hpp - Complete Functionality Test
// This test demonstrates all public APIs provided by the library.
// Each test opens a dialog and waits for user interaction.
// ============================================================

static void testMessageBox()
{
    std::cout << "=== Test: messageBox (style 0 - GDI drawn) ===\n";
    std::cout << "A message box with word-wrapped text should appear. "
                 "The dialog auto-resizes to fit the content.\n";
    int result = GLDLG::messageBox(
        "messageBox Test(Style 0)",
        "This is like the traditional MessageBox function.\r\n" "However, you can completely customize the buttons in the dialog box.\r\n"
        "And these is no limitations in the number of the buttons!\r\n"
        ,{
            {0, "OK"}, 
            {1, "Cancel"}, 
            {2, "Retry"},
            {3, "Custom 1"},
            {4, "Custom 2"},
            {5, "Custom 3"}
        }
    );//Default in style 0
    std::cout << "Result: button ID = " << result << "\n\n";
}

static void testMessageBoxRich()
{
    std::cout << "=== Test: messageBox (style 1 - rich/edit) ===\n";
    std::cout << "A rich message box with [Yes] [No] [Maybe] buttons should appear.\n";
    int result = GLDLG::messageBox(
        "messageBox Test(Style 1)",
        "This is a rich message box.\nIt supports multi-line text with word wrapping.\n\nYou can display longer messages here,such as debugging infomations and clauses,etc.",
        {{10, "Yes"}, {20, "No"}, {30, "Maybe"}},
        NULL,
        1 //Use style 1
    );
    std::cout << "Result: button ID = " << result << "\n\n";
}

static void testPromptDialog()
{
    std::cout << "=== Test: promptDialog ===\n";
    std::cout << "A input dialog should appear. Enter some text and click OK.\n";
    std::string output;
    bool confirmed = GLDLG::promptDialog(
        "Input Test",
        "Please enter your name below:",
        output,
        "Default Name"
    );
    if (confirmed) {
        std::cout << "Confirmed. Input: \"" << output << "\"\n";
    } else {
        std::cout << "Cancelled.\n";
    }
    std::cout << "\n";
}

static void testMultilinePromptDialog()
{
    std::cout << "=== Test: promptDialog (Multiline) ===\n";
    std::cout << "A multiline input dialog should appear. Enter some text and click OK.\n";
    std::string output;
    bool confirmed = GLDLG::promptDialog(
        "Input Test",
        "Please enter multiline text:",
        output,
        "Default Text\nLine 2\nLine 3",
        NULL,
        true // Enable multiline mode
    );
    if (confirmed) {
        std::cout << "Confirmed. Input: \"" << output << "\"\n";
    } else {
        std::cout << "Cancelled.\n";
    }
    std::cout << "\n";
}

static void testGetOpenFileName()
{
    std::cout << "=== Test: getOpenFileName ===\n";
    std::cout << "An Open File dialog should appear. Select a file and click Open.\n";
    try {
        std::string path = GLDLG::getOpenFileName(
            {"Text Files (*.txt)|*.txt", "All Files (*.*)|*.*"},
            "Select a text file",
            "",
            "",
            "txt"
        );
        if (path.empty()) {
            std::cout << "User cancelled.\n";
        } else {
            std::cout << "Selected file: " << path << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    std::cout << "\n";
}

static void testGetSaveFileName()
{
    std::cout << "=== Test: getSaveFileName ===\n";
    std::cout << "A Save File dialog should appear. Choose a path and click Save.\n";
    try {
        std::string path = GLDLG::getSaveFileName(
            {"Text Files (*.txt)|*.txt", "All Files (*.*)|*.*"},
            "Save as...",
            "",
            "untitled.txt",
            "txt"
        );
        if (path.empty()) {
            std::cout << "User cancelled.\n";
        } else {
            std::cout << "Save path: " << path << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    std::cout << "\n";
}

static void testGetOpenMultipleFileNames()
{
    std::cout << "=== Test: getOpenMultipleFileNames ===\n";
    std::cout << "A multi-select file dialog should appear. "
                 "Select multiple files and click Open.\n";
    try {
        auto files = GLDLG::getOpenMultipleFileNames(
            {"All Files (*.*)|*.*"},
            "Select multiple files"
        );
        if (files.empty()) {
            std::cout << "User cancelled.\n";
        } else {
            std::cout << "Selected " << files.size() << " file(s):\n";
            for (size_t i = 0; i < files.size(); ++i) {
                std::cout << "  [" << i << "] " << files[i] << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    std::cout << "\n";
}

static void testGetOpenDirectoryName()
{
    std::cout << "=== Test: getOpenDirectoryName ===\n";
    std::cout << "A folder browser dialog should appear. Select a folder.\n";
    try {
        std::string dir = GLDLG::getOpenDirectoryName(
            "Please select a folder:",
            ""
        );
        if (dir.empty()) {
            std::cout << "User cancelled.\n";
        } else {
            std::cout << "Selected directory: " << dir << "\n";
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    std::cout << "\n";
}

static void testGetOpenDirectoryNames()
{
    std::cout << "=== Test: getOpenDirectoryNames ===\n";
    std::cout << "A multi-select folder dialog should appear. "
                 "Select multiple folders and click Open.\n";
    try {
        auto dirs = GLDLG::getOpenDirectoryNames(
            "Select one or more folders:",
            ""
        );
        if (dirs.empty()) {
            std::cout << "User cancelled.\n";
        } else {
            std::cout << "Selected " << dirs.size() << " director(ies):\n";
            for (size_t i = 0; i < dirs.size(); ++i) {
                std::cout << "  [" << i << "] " << dirs[i] << "\n";
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    std::cout << "\n";
}

static void testChooseColor()
{
    std::cout << "=== Test: CreateDynamicColorPicker ===\n";
    std::cout << "A non-blocking color picker dialog should appear. "
                 "Pick a color and close the dialog.\n";

    auto picker = GLDLG::CreateDynamicColorPicker(
        "Pick a Color",
        {255, 0, 0},
        false
    );

    picker.SetCallback([](GLDLG::DynamicColorPicker::DynamicColorCallbackMessageType type,GLDLG::ColorRGBA color)->GLDLG::ColorRGBA{
        if(type == GLDLG::DynamicColorPicker::DynamicColorCallbackMessageType::Dragging){
            std::cout << "Color is changing: ("
                << (int)color.r << ", " << (int)color.g << ", "
              << (int)color.b << ", " << (int)color.a << ")\n";
        }
        else if(type == GLDLG::DynamicColorPicker::DynamicColorCallbackMessageType::Released){
            std::cout << "Color is determined: ("
                << (int)color.r << ", " << (int)color.g << ", "
              << (int)color.b << ", " << (int)color.a << ")\n";
        }
        return color;
    });

    std::cout << "Color picker is running. Close the window when done.\n";

    while (!picker.IsFinished()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    GLDLG::ColorRGBA color = picker.GetColor();
    std::cout << "Color picker closed. Final color: ("
              << (int)color.r << ", " << (int)color.g << ", "
              << (int)color.b << ", " << (int)color.a << ")\n";
    std::cout << "\n";
}

static void testChooseColorAlpha()
{
    std::cout << "=== Test: CreateDynamicColorPicker (with alpha) ===\n";
    std::cout << "A color picker with semi-transparent initial color should appear.\n";

    auto picker = GLDLG::CreateDynamicColorPicker(
        "Pick a Color (Alpha)",
        {0, 128, 255, 128}
    );

    picker.SetCallback([](GLDLG::DynamicColorPicker::DynamicColorCallbackMessageType type,GLDLG::ColorRGBA color)->GLDLG::ColorRGBA{
        if(type == GLDLG::DynamicColorPicker::DynamicColorCallbackMessageType::Dragging){
            std::cout << "Color is changing: ("
                << (int)color.r << ", " << (int)color.g << ", "
              << (int)color.b << ", " << (int)color.a << ")\n";
        }
        else if(type == GLDLG::DynamicColorPicker::DynamicColorCallbackMessageType::Released){
            std::cout << "Color is determined: ("
                << (int)color.r << ", " << (int)color.g << ", "
              << (int)color.b << ", " << (int)color.a << ")\n";
        }
        return color;
    });

    std::cout << "Color picker is running. Close the window when done.\n";

    while (!picker.IsFinished()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    GLDLG::ColorRGBA color = picker.GetColor();
    std::cout << "Color picker closed. Final color: ("
              << (int)color.r << ", " << (int)color.g << ", "
              << (int)color.b << ", " << (int)color.a << ")\n";
    std::cout << "\n";
}

static void testChooseFont()
{
    std::cout << "=== Test: chooseFont ===\n";
    std::cout << "A font selection dialog should appear. Pick a font.\n";
    try {
        GLDLG::chooseFontInfo cfi;
        GLDLG::chooseFont(cfi);
        std::cout << "Selected font:\n";
        std::cout << "  Name: " << cfi.fontFaceName << "\n";
        std::cout << "  Size: " << cfi.fontPointSize << "pt\n";
        std::cout << "  Path: " << (cfi.fontPath.empty() ? "(not found)" : cfi.fontPath) << "\n";
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << "\n";
    }
    std::cout << "\n";
}

static void testDynamicProgressBar()
{
    std::cout << "=== Test: CreateDynamicProgressBar ===\n";
    std::cout << "A non-blocking progress bar dialog should appear. "
                 "Watch it progress from 0 to 100.\n";

    auto bar = GLDLG::CreateDynamicProgressBar(
        "Progress Test",
        "Downloading files...",
        NULL
    );

    // Simulate progress updates
    for (uint64_t i = 0; i <= 100; i += 10) {
        bar.SetValue(i, 100, "Processing: " + std::to_string(i) + "%");
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }

    bar.SetValue(100, 100, "Complete!");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "Closing progress bar...\n";
    bar.Close();

    // Wait for the thread to finish
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "Progress bar test finished.\n\n";
}

static void testDynamicSlider()
{
    std::cout << "=== Test: CreateDynamicSlider ===\n";
    std::cout << "A non-blocking slider dialog should appear. "
                 "Try dragging the slider.\n";

    auto slider = GLDLG::CreateDynamicSlider(
        "Slider Test",
        "Drag the slider to adjust value:",
        0,     // min
        100,   // max
        50,    // initial value
        [](GLDLG::DynamicSliderCallbackMessageType type, int value) -> int {
            // You could modify or clamp the value here
            if (type == GLDLG::DynamicSliderCallbackMessageType::Dragging) {
                std::cout << "Slider is being dragged: " << value << "\n";
            }
            else if(type == GLDLG::DynamicSliderCallbackMessageType::Released){
                std::cout << "Slider is released: " << value << "\n";
            }
            return value;
        },
        NULL
    );

    std::cout << "Slider is running. Close the window when done.\n";

    // Wait while the slider is open (poll every 200ms)
    while (!slider.IsFinished()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    int cur = 0, min = 0, max = 0;
    std::string msg;
    slider.GetSliderInfo(cur, min, max, msg);
    std::cout << "Slider closed. Final value: " << cur
              << " (range: " << min << " - " << max << ")\n";
    std::cout << "Slider test finished.\n\n";
}

// ============================================================
// Entry point
// ============================================================
int main()
{

    // RGBAColor c;
    // chooseColorEx(c,true);

    std::cout << "Each test will open a dialog. Interact with it,\n";
    std::cout << "then check the console output for results.\n";
    std::cout << "Press Ctrl+C at any time to abort.\n\n";

    int test = GLDLG::messageBox("Select a test","Select a test",{
        {1,"All"},
        {100,"All Native"},
        {101,"All Extended"},
        {2,"Normal Messagebox"},
        {3,"Rich Messagebox"},
        {4,"Prompt"},
        {44,"Prompt(Multiline)"},
        {5,"Open File"},
        {6,"Save File"},
        {7,"Open Files"},
        {8,"Open Directory"},
        {9,"Open Directories"},
        {10,"Choose Color"},
        {11,"Choose Color(Alpha)"},
        {12,"Choose Font"},
        {13,"Progress Bar"},
        {14,"Slider"},
    });

    // --- Blocking Dialogs (sequential, user must interact) ---
    if(test == 1 || test == 2 || test == 101) testMessageBox();
    if(test == 1 || test == 3 || test == 101) testMessageBoxRich();
    if(test == 1 || test == 4 || test == 101) testPromptDialog();
    if(test == 1 || test == 44 || test == 101) testMultilinePromptDialog();
    if(test == 1 || test == 5 || test == 100) testGetOpenFileName();
    if(test == 1 || test == 6 || test == 100) testGetSaveFileName();
    if(test == 1 || test == 7 || test == 100) testGetOpenMultipleFileNames();
    if(test == 1 || test == 8 || test == 100) testGetOpenDirectoryName();
    if(test == 1 || test == 9 || test == 100) testGetOpenDirectoryNames();
    
    if(test == 1 || test == 12 || test == 100) testChooseFont();

    // --- Dynamic (non-blocking) Dialogs ---
    if(test == 1 || test == 13 || test == 101) testDynamicProgressBar();
    if(test == 1 || test == 14 || test == 101) testDynamicSlider();
    if(test == 1 || test == 10 || test == 101) testChooseColor();
    if(test == 1 || test == 11 || test == 101) testChooseColorAlpha();

    std::cout << "============================================\n";
    std::cout << "  All tests completed!\n";
    std::cout << "============================================\n";
    return 0;
}