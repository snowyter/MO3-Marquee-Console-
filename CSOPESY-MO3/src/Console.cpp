// Console.cpp
#include "Console.h"

#include <cstdio>    // std::fwrite, std::fflush, std::setvbuf
#include <iostream>

#ifdef _WIN32
#include <windows.h>     // console mode + window size (Windows only)
#else
#include <sys/ioctl.h>   // window size (Linux/macOS)
#include <unistd.h>
#endif

namespace {
    // ANSI color codes
    const char* const GREEN  = "\033[32m";
    const char* const YELLOW = "\033[33m";
    const char* const RED    = "\033[31m";
    const char* const CYAN   = "\033[36m";
    const char* const GREY   = "\033[90m";

    // The stdout buffer. 64 KB is far bigger than one full frame, so a frame
    // never gets split into several writes (a split write = visible tearing).
    char outputBuffer[1 << 16];
}

namespace Console {

const char* const RESET = "\033[0m";

void init() {
#ifdef _WIN32
    // Older Windows consoles ignore "\033[..." codes unless we turn on
    // ENABLE_VIRTUAL_TERMINAL_PROCESSING. (Windows Terminal already has it on)
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode)) {
        SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
    // _IOFBF = "fully buffered": output collects in outputBuffer and only
    // goes to the screen when we call fflush (see write())
    std::setvbuf(stdout, outputBuffer, _IOFBF, sizeof(outputBuffer));
}

void enterFullscreen() {
    // ?1049h = switch to alternate screen, 2J = clear it, H = cursor home
    write("\033[?1049h\033[2J\033[H");
}

void leaveFullscreen() {
    // ?25h = make sure the cursor is visible, ?1049l = back to normal screen
    write("\033[?25h\033[?1049l");
}

void getSize(int& cols, int& rows) {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info)) {
        // srWindow = the part of the console that's actually visible
        cols = info.srWindow.Right - info.srWindow.Left + 1;
        rows = info.srWindow.Bottom - info.srWindow.Top + 1;
        return;
    }
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        cols = ws.ws_col;
        rows = ws.ws_row;
        return;
    }
#endif
    cols = 80;  // fallback if the size can't be read
    rows = 25;
}

void write(const std::string& text) {
    std::fwrite(text.data(), 1, text.size(), stdout);
    std::fflush(stdout);   // push the whole thing to the screen at once
}

std::vector<Line> headerLines() {
    // R"(...)" is a raw string: backslashes are printed as-is
    return {
        {CYAN,  R"(  ____ ____   ___  ____  _____ ______   __)"},
        {CYAN,  R"( / ___/ ___| / _ \|  _ \| ____/ ___\ \ / /)"},
        {CYAN,  R"(| |   \___ \| | | | |_) |  _| \___ \\ V / )"},
        {CYAN,  R"(| |___ ___) | |_| |  __/| |___ ___) || |  )"},
        {CYAN,  R"( \____|____/ \___/|_|   |_____|____/ |_|  )"},
        {"",    ""},
        {GREEN, "Welcome to the CSOPESY Marquee Operator!"},
        {"",    ""},
        {"",    "Group developer:"},
        {"",    "  David, Justin Ice"},
        {"",    "  Limpin, Kryster Knowell"},
        {"",    "  Singson, Keith Railey"},
        {"",    ""},
        {"",    "Version date: 09/28/2026"},
        {"",    ""},
    };
}

const char* colorFor(MsgType type) {
    switch (type) {
        case MsgType::Command: return GREY;
        case MsgType::Success: return GREEN;
        case MsgType::Error:   return RED;
        case MsgType::Info:    return "";
    }
    return "";
}

void print(MsgType type, const std::string& text) {
    const char* color = colorFor(type);
    std::string line = *color ? std::string(color) + text + RESET : text;
    write(line + "\n");
}

} // namespace Console
