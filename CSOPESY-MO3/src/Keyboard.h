#pragma once
// Keyboard.h
/*
// ---------------------------------------------------------------------------
// NON-BLOCKING keyboard input. std::getline / std::cin wait until the user
// presses Enter, freezing everything else. poll() instead returns
// immediately: either with one key that was pressed, or "nothing yet".
// That's what lets the marquee keep moving while the user types.
// ---------------------------------------------------------------------------
*/

struct KeyEvent {
    enum Kind { None, Char, Enter, Backspace, Escape, Other };
    Kind kind = None;
    char ch = 0;        // the character, when kind == Char
};

class Keyboard {
public:
    Keyboard();    // Linux/macOS: switches the terminal to "raw" key-by-key mode
    ~Keyboard();   // ...and switches it back

    // Not copyable (there's only one keyboard)
    Keyboard(const Keyboard&) = delete;
    Keyboard& operator=(const Keyboard&) = delete;

    // If a key is waiting, fills 'out' and returns true
    // If not, returns false straight away (never waits)
    bool poll(KeyEvent& out);
};
