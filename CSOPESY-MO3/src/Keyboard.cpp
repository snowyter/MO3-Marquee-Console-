// Keyboard.cpp
#include "Keyboard.h"

#ifdef _WIN32
// -------------------------------------- Windows --------------------------------------         
#include <conio.h>   // _kbhit(): "is a key waiting?"   _getch(): "read it"

Keyboard::Keyboard() {}
Keyboard::~Keyboard() {}

bool Keyboard::poll(KeyEvent& out) {
    if (!_kbhit()) return false;          // nothing pressed -> return at once

    int c = _getch();                     // doesn't echo, doesn't wait for Enter

    if (c == 0 || c == 224) {             // arrow/function keys arrive as TWO
        _getch();                         // codes: a prefix, then the key
        out.kind = KeyEvent::Other;       // We read and ignore both
    } else if (c == '\r') {
        out.kind = KeyEvent::Enter;
    } else if (c == '\b') {
        out.kind = KeyEvent::Backspace;
    } else if (c == 27) {
        out.kind = KeyEvent::Escape;
    } else if (c >= 32 && c < 127) {      // printable ASCII
        out.kind = KeyEvent::Char;
        out.ch = static_cast<char>(c);
    } else {
        out.kind = KeyEvent::Other;       // tab, Ctrl+letter, accents, ...
    }
    return true;
}

#else
// -------------------------------------- Linux / macOS --------------------------------------
// Not needed for the Windows build -- lets the project also run on Linux/Mac
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

namespace {
    termios originalSettings;
    bool haveOriginal = false;
    int originalFlags = 0;
    int pendingByte = -1;   // a byte read too far while checking for a key sequence

    bool readByte(unsigned char& c) {
        if (pendingByte >= 0) { c = static_cast<unsigned char>(pendingByte); pendingByte = -1; return true; }
        return read(STDIN_FILENO, &c, 1) == 1;
    }
}

Keyboard::Keyboard() {
    // Turn off line buffering (ICANON) and echo, so keys arrive one at a time
    if (tcgetattr(STDIN_FILENO, &originalSettings) == 0) {
        haveOriginal = true;
        termios raw = originalSettings;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    }
    // O_NONBLOCK: read() returns immediately when no key is waiting
    originalFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, originalFlags | O_NONBLOCK);
}

Keyboard::~Keyboard() {
    fcntl(STDIN_FILENO, F_SETFL, originalFlags);
    if (haveOriginal) tcsetattr(STDIN_FILENO, TCSANOW, &originalSettings);
}

bool Keyboard::poll(KeyEvent& out) {
    unsigned char c;
    if (!readByte(c)) return false;

    if (c == 27) {                         // Escape, or the start of an arrow key
        unsigned char extra;               // sequence like ESC [ A
        if (!readByte(extra)) {
            out.kind = KeyEvent::Escape;   // ESC on its own
        } else if (extra == '[' || extra == 'O') {
            // Eat the rest of the sequence: parameter bytes (0x30-0x3F) and
            // intermediate bytes (0x20-0x2F), then the final byte (0x40+).
            // Arrows, Home/End, PgUp/PgDn and F-keys are all ignored this way.
            while (readByte(extra) && extra < 0x40) {
                // keep eating until the final byte is consumed
            }
            out.kind = KeyEvent::Other;    // ignored
        } else {
            pendingByte = extra;           // a normal key typed right after Esc
            out.kind = KeyEvent::Escape;   // -> clear the line, keep that key
        }
    } else if (c == '\n' || c == '\r') {
        out.kind = KeyEvent::Enter;
    } else if (c == 127 || c == '\b') {
        out.kind = KeyEvent::Backspace;
    } else if (c >= 32 && c < 127) {
        out.kind = KeyEvent::Char;
        out.ch = static_cast<char>(c);
    } else {
        out.kind = KeyEvent::Other;
    }
    return true;
}
#endif
