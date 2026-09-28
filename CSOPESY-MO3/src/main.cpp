// main.cpp  --  ENTRY POINT of the CSOPESY Marquee Operator
/*
// ---------------------------------------------------------------------------
   Follows the OS "boot sequence" (week 2 lec)
    1. Bootstrapping         -> Console::init(), load config.txt
    2. Kernel initialization -> create Marquee, Display, CommandInterpreter
    3. Start services        -> start the marquee ANIMATION thread
    4. Main loop             -> this thread POLLS the keyboard
    5. Shutdown and cleanup  -> stop + join the animation thread

   Two threads run at the same time:
    - Animation thread: every <speed> ms, moves the marquee one step and
      redraws the screen.
    - Main thread: every <pollMs> ms, checks for key presses (without
      waiting), updates the typed text, runs a command on Enter, and redraws.
// ---------------------------------------------------------------------------
*/
#include <chrono>
#include <csignal>
#include <string>
#include <thread>
#include <vector>

#include "CommandInterpreter.h"
#include "Config.h"
#include "Console.h"
#include "Display.h"
#include "Keyboard.h"
#include "Marquee.h"

namespace {
    // Ctrl+C should go through the normal shutdown path (so the terminal is
    // restored), instead of killing the process and leaving it in fullscreen.
    volatile std::sig_atomic_t interrupted = 0;
    void handleInterrupt(int) { interrupted = 1; }
}

int main() {
    // ---------- Bootstrapping ----------
    Console::init();
    std::signal(SIGINT, handleInterrupt);

    // Settings: start from the defaults in Config.h, then let config.txt
    // override them. A missing file is fine -- defaults are used.
    AppConfig config;
    std::vector<std::string> configWarnings;
    if (!loadConfig("config.txt", config, configWarnings)) {
        configWarnings.push_back("config.txt could not be opened; using the built-in defaults.");
    }

    // ---------- Kernel initialization ----------
    Marquee marquee(config.marqueeText, config.marqueeSpeedMs);
    marquee.setDirection(config.direction);
    if (config.startRunning) marquee.start();

    Display display(marquee);

    CommandInterpreter interpreter(
        marquee,
        // How messages are shown: added to the display's output log
        [&display](MsgType type, const std::string& text) { display.addMessage(type, text); }
    );

    Keyboard keyboard;   // switches keyboard to key-by-key mode

    Console::enterFullscreen();
    for (const std::string& warning : configWarnings) {
        display.addMessage(MsgType::Error, warning);   // bad lines in config.txt
    }
    display.addMessage(MsgType::Info, "Type 'help' to see the list of commands.");
    display.setInput(interpreter.prompt(), "");
    display.draw();

    // ---------- Start services: the animation thread ----------
    // The code inside [ ... ] { ... } runs on a SEPARATE thread, in parallel with the keyboard loop below
    std::thread animator([&marquee, &display] {
        while (marquee.waitForNextTick()) {   // sleep one interval (false = exiting)
            marquee.tick();                   // move one step (if running)
            display.draw();                   // show it
        }
    });

    // ---------- Main loop: keyboard polling ----------
    std::string typed;   // what the user has typed so far on this line

    while (!interpreter.exitRequested() && !interrupted) {
        bool changed = false;
        KeyEvent key;

        // Handle EVERY key that's waiting, then redraw once
        while (!interpreter.exitRequested() && keyboard.poll(key)) {
            changed = true;
            switch (key.kind) {
                case KeyEvent::Char:
                    if (typed.size() < config.maxInputLength) typed += key.ch;
                    break;
                case KeyEvent::Backspace:
                    if (!typed.empty()) typed.pop_back();
                    break;
                case KeyEvent::Escape:            // Esc clears the line
                    typed.clear();
                    break;
                case KeyEvent::Enter:
                    // Echo the line into the log, then run it
                    display.addMessage(MsgType::Command, interpreter.prompt() + typed);
                    interpreter.execute(typed);
                    typed.clear();
                    break;
                default:                          // arrows etc.: ignored
                    break;
            }
        }

        if (changed) {
            display.setInput(interpreter.prompt(), typed);
            display.draw();
        }

        // Rest a little so this loop doesn't use 100% of a CPU core
        std::this_thread::sleep_for(std::chrono::milliseconds(config.pollMs));
    }

    // ---------- Shutdown and cleanup ----------
    marquee.shutdown();   // wake the animation thread and tell it to finish
    animator.join();      // wait until it has actually finished
    std::this_thread::sleep_for(std::chrono::milliseconds(400)); // let "Goodbye" show
    Console::leaveFullscreen();
    Console::print(MsgType::Info, "CSOPESY terminated.");
    return 0;
}
