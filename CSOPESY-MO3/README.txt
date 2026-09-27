CSOPESY - MO3 Marquee Operator
==============================

Group members:
  TODO: Lastname, Firstname
  TODO: Lastname, Firstname
  TODO: Lastname, Firstname

Entry point:
  src/main.cpp  (contains the main() function)

Requirements:
  - Windows 10/11
  - VS Code with the "C/C++" extension (Microsoft)
  - WinLibs MinGW-w64 (GCC with POSIX threads). Install it once with:
        winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT --scope user
    Then, in Command Prompt, MOVE it to C:\winlibs (a path with no spaces --
    GCC's linker fails if its path has a space, e.g. "C:\Users\ASUS TUF";
    a junction/symlink does NOT work because GCC resolves it back):
        move "%LOCALAPPDATA%\Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64" C:\winlibs
    Check with:  C:\winlibs\bin\g++ --version   (should print 16.x)
    The VS Code build/run settings in .vscode/ point to C:\winlibs\bin.
    NOTE: the old "MinGW.org" compiler in C:\MinGW (GCC 6.x) will NOT work:
    it has no std::thread / std::mutex.

How to run:
  1. Open THIS folder (the one containing src/ and .vscode/) in VS Code
     (File > Open Folder).
  2. Press F5 (Run > Start Debugging). This builds csopesy.exe and opens it
     in a new console window.

  Or from a terminal in this folder:
     g++ -std=c++17 -O2 -static -pthread src/*.cpp -o csopesy.exe
     csopesy.exe

Commands:
  help [command]            Displays the commands and their descriptions
  start_marquee             Starts the marquee animation
  stop_marquee              Stops the marquee animation
  set_text [text]           Sets the marquee text (no text = asks on next line)
  set_speed [milliseconds]  Sets the marquee refresh speed in ms, 1-60000
                            (no value = asks on next line)
  exit                      Terminates the console

Keys while typing:
  Enter = run command, Backspace = delete, Esc = clear the line

Source files (src/):
  main.cpp                boot sequence, animation thread, keyboard polling loop
  CommandInterpreter.*    parses and validates commands, runs them
  Marquee.*               marquee state + scrolling logic (thread-safe)
  Display.*               draws each full frame (header, marquee, output, prompt)
  Keyboard.*              non-blocking key reading (_kbhit/_getch on Windows)
  Console.*               ANSI setup, fullscreen buffer, window size, header text
  Config.*                settings (AppConfig) + config.txt loader  [STUB: TODO(groupmate)]
  Message.h               message types (Command / Info / Success / Error)

Settings (config.txt, next to README.txt):
  text=...            starting marquee text
  speed_ms=100        starting speed, 1-60000
  poll_ms=10          keyboard check interval, 1-1000
  start_running=false true = animate immediately
  direction=left_to_right   or right_to_left
  Edit and re-run -- no recompiling needed.
  NOTE: takes effect once loadConfig() in src/Config.cpp is implemented
  (search the code for "TODO(groupmate)").
