#pragma once
// Console.h
/*
// ---------------------------------------------------------------------------
   Low-level terminal helpers: turning on ANSI codes, switching to a
   full-screen buffer, reading the window size, writing a finished frame,
   and the header text. This is the ONLY file that knows about the OS's
   console (the Windows-specific code lives in Console.cpp).
// ---------------------------------------------------------------------------
*/
#include <string>
#include <vector>

#include "Message.h"

namespace Console {

    // One line of text plus the color to draw it in
    struct Line {
        const char* color;   // an ANSI color code, or "" for default
        std::string text;
    };

    extern const char* const RESET;   // ANSI "back to normal color"

    // Call once at startup: enables ANSI codes on Windows and gives stdout a
    // big buffer so a whole frame can be sent to the screen in one go.
    void init();

    // Switch to / from the "alternate screen": a separate, fixed-size screen
    // with no scrollback (like how nano or vim take over the terminal).
    // Leaving it restores whatever was in the terminal before.
    void enterFullscreen();
    void leaveFullscreen();

    // Current size of the visible console window, in characters.
    void getSize(int& cols, int& rows);

    // Sends a finished frame to the screen in one write, then flushes.
    void write(const std::string& text);

    // The CSOPESY banner, group members and version date.
    std::vector<Line> headerLines();

    // Which color each kind of message is drawn in.
    const char* colorFor(MsgType type);

    // Prints one colored line the normal way (used after leaving fullscreen).
    void print(MsgType type, const std::string& text);

} // namespace Console
