#pragma once
// CommandInterpreter.h
/*
// ---------------------------------------------------------------------------
   Takes one line the user typed, figures out which command it is, checks the
   arguments, and runs it.

   It never reads the keyboard or prints to the screen directly. Instead:
     - main() hands it each finished line via execute(line)
     - it reports results through the `print` function it was given
// --------------------------------------------------------------------------- 
*/
#include <atomic>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "Marquee.h"
#include "Message.h"

class CommandInterpreter {
public:
    using PrintFn = std::function<void(MsgType, const std::string&)>;

    CommandInterpreter(Marquee& marquee, PrintFn print);

    // Run one full line of input (whatever the user typed before Enter)
    void execute(const std::string& line);

    // What to show before the cursor. Normally "Command> ", but when
    // 'set_text' / 'set_speed' is typed with no value, it asks for the value
    // on the next line and the prompt changes to say so
    std::string prompt() const;

    // Becomes true after the 'exit' command
    bool exitRequested() const { return exit_; }

private:
    // One entry in the command table
    struct Command {
        std::string name;         // what the user types
        std::string usage;        // e.g. "set_speed [milliseconds]"
        std::string description;  // one line for 'help'
        bool takesArgs;           // false = extra words are an error
        std::function<void(const std::string& args)> run;
    };

    // What the interpreter is waiting for on the next line, if anything
    enum class Pending { None, Text, Speed };

    void registerCommands();
    void add(const std::string& name, const std::string& usage,
             const std::string& description, bool takesArgs,
             std::function<void(const std::string&)> run);

    // Command handlers
    void cmdHelp(const std::string& args);
    void cmdStart();
    void cmdStop();
    void cmdSetText(const std::string& args);
    void cmdSetSpeed(const std::string& args);
    void cmdExit();

    // Shared by "set_text <text>" and the "Enter marquee text> " follow-up
    void applyText(const std::string& raw);
    void applySpeed(const std::string& raw);

    Marquee& marquee_;
    PrintFn print_;

    std::map<std::string, Command> commands_;  // key = normalised name
    std::vector<std::string> helpOrder_;        // order shown in 'help'

    Pending pending_ = Pending::None;
    std::atomic<bool> exit_{false};
};
