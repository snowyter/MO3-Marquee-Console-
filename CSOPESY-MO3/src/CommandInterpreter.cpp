// CommandInterpreter.cpp
#include "CommandInterpreter.h"

#include <algorithm> // std::max
#include <cctype>    // std::isspace, std::isdigit, std::tolower
#include <utility>   // std::move

/*
// ===========================================================================
   Small text helpers. They live in an unnamed namespace == only this cpp file
   can see them : )
// ===========================================================================
*/
namespace {

// Removes spaces/tabs/newlines (and Windows '\r') from both ends
std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Commands are case-insensitive, and '-' is treated like '_',
// so "START_MARQUEE", "start-marquee" and "start_marquee" all work
std::string normalise(const std::string& name) {
    std::string out = toLower(name);
    for (char& c : out) if (c == '-') c = '_';
    return out;
}

// "hello" or 'hello' -> hello  (only if the quotes match at both ends)
std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front()) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// Tabs/other control characters would mess with the marquee's spacing,
// so tabs become spaces and anything else invisible is dropped
std::string sanitise(const std::string& s) {
    std::string out;
    for (char c : s) {
        unsigned char u = static_cast<unsigned char>(c);
        if (c == '\t')            out += ' ';
        else if (u >= 32 && u != 127) out += c;
    }
    return out;
}

// Turns the text after "set_speed" into a number of milliseconds
// Returns true on success; on failure fills 'error' with a message
// Accepts "100" and "100ms" / "100 ms". Rejects everything else
bool parseSpeed(const std::string& raw, int& result, std::string& error) {
    std::string s = toLower(trim(raw));

    // Allow an "ms" unit at the end (optional)
    if (s.size() >= 2 && s.compare(s.size() - 2, 2, "ms") == 0) {
        s = trim(s.substr(0, s.size() - 2));
    }

    if (s.empty()) {
        error = "No speed given.";
        return false;
    }
    if (s[0] == '-') {
        error = "Speed must be a positive number of milliseconds (got '" + trim(raw) + "').";
        return false;
    }
    if (s[0] == '+') s = s.substr(1);

    for (char c : s) {
        if (c == '.' || c == ',') {
            error = "Speed must be a whole number of milliseconds (got '" + trim(raw) + "').";
            return false;
        }
        if (!std::isdigit(static_cast<unsigned char>(c))) {
            error = "'" + trim(raw) + "' is not a valid number.";
            return false;
        }
    }
    if (s.empty()) {
        error = "'" + trim(raw) + "' is not a valid number.";
        return false;
    }

    // Skip leading zeros, then check the length BEFORE converting,
    // so a huge number like 99999999999999 can't overflow an int
    size_t firstNonZero = s.find_first_not_of('0');
    std::string digits = (firstNonZero == std::string::npos) ? "0" : s.substr(firstNonZero);
    long long value = (digits.size() > 9) ? 1000000000LL : std::stoll(digits);

    if (value < Marquee::MIN_SPEED_MS || value > Marquee::MAX_SPEED_MS) {
        error = "Speed must be between " + std::to_string(Marquee::MIN_SPEED_MS) +
                " and " + std::to_string(Marquee::MAX_SPEED_MS) + " ms (got '" + trim(raw) + "').";
        return false;
    }

    result = static_cast<int>(value);
    return true;
}

} // namespace

// ===========================================================================
// Setup
// ===========================================================================
CommandInterpreter::CommandInterpreter(Marquee& marquee, PrintFn print)
    : marquee_(marquee), print_(std::move(print)) {
    registerCommands();
}

void CommandInterpreter::add(const std::string& name, const std::string& usage,
                             const std::string& description, bool takesArgs,
                             std::function<void(const std::string&)> run) {
    commands_[normalise(name)] = Command{name, usage, description, takesArgs, std::move(run)};
}

void CommandInterpreter::registerCommands() {
    // ---- The six commands required by the MO3 spec ----
    add("help", "help [command]", "Displays the commands and their descriptions.", true,
        [this](const std::string& a) { cmdHelp(a); });
    add("start_marquee", "start_marquee", "Starts the marquee animation.", false,
        [this](const std::string&) { cmdStart(); });
    add("stop_marquee", "stop_marquee", "Stops the marquee animation.", false,
        [this](const std::string&) { cmdStop(); });
    add("set_text", "set_text [text]",
        "Sets the marquee text. With no text, asks for it on the next line.", true,
        [this](const std::string& a) { cmdSetText(a); });
    add("set_speed", "set_speed [milliseconds]",
        "Sets the marquee refresh speed in ms. With no value, asks for it on the next line.", true,
        [this](const std::string& a) { cmdSetSpeed(a); });
    add("exit", "exit", "Terminates the console.", false,
        [this](const std::string&) { cmdExit(); });
    helpOrder_ = {"help", "start_marquee", "stop_marquee", "set_text", "set_speed", "exit"};
}

// ===========================================================================
// Main entry point: one line of input
// ===========================================================================
void CommandInterpreter::execute(const std::string& line) {
    //  If a previous 'set_text' / 'set_speed' asked for a value,
    //  this whole line IS that value, not a new command
    if (pending_ != Pending::None) {
        Pending what = pending_;
        pending_ = Pending::None;
        if (what == Pending::Text && trim(line).empty()) {
            print_(MsgType::Info, "set_text cancelled. Text was not changed.");
            return;
        }
        if (what == Pending::Text)  applyText(line);
        if (what == Pending::Speed) applySpeed(line);
        return;
    }

    // Ignore blank lines (just pressing Enter)
    std::string input = trim(line);
    if (input.empty()) return;

    // Split into the command word and "everything after it"
    // We keep the rest as ONE string so 'set_text Hello   world' keeps
    // its spaces, instead of splitting it into separate words
    size_t space = input.find_first_of(" \t");
    std::string name = (space == std::string::npos) ? input : input.substr(0, space);
    std::string args = (space == std::string::npos) ? "" : trim(input.substr(space));

    // Look the command up
    auto it = commands_.find(normalise(name));
    if (it == commands_.end()) {
        print_(MsgType::Error, "Unknown command: '" + name + "'. Type 'help' to see available commands.");
        return;
    }
    const Command& cmd = it->second;

    // Commands like 'exit' or 'start_marquee' shouldn't get extra words
    if (!cmd.takesArgs && !args.empty()) {
        print_(MsgType::Error, "'" + cmd.name + "' does not take any arguments. Usage: " + cmd.usage);
        return;
    }

    cmd.run(args);
}

std::string CommandInterpreter::prompt() const {
    switch (pending_) {
        case Pending::Text:  return "Enter marquee text> ";
        case Pending::Speed: return "Enter speed in ms> ";
        default:             return "Command> ";
    }
}

// ===========================================================================
// Command handlers
// ===========================================================================
void CommandInterpreter::cmdHelp(const std::string& args) {
    // 'help set_speed' -> details for one command
    if (!args.empty()) {
        auto it = commands_.find(normalise(args));
        if (it == commands_.end()) {
            print_(MsgType::Error, "No help for unknown command: '" + args + "'.");
            return;
        }
        print_(MsgType::Info, "Usage: " + it->second.usage);
        print_(MsgType::Info, "  " + it->second.description);
        return;
    }

    // Plain 'help' -> table of all commands, with descriptions lined up.
    size_t width = 0;
    for (const std::string& key : helpOrder_) width = std::max(width, commands_[key].usage.size());

    print_(MsgType::Info, "Available commands:");
    for (const std::string& key : helpOrder_) {
        const Command& c = commands_[key];
        print_(MsgType::Info, "  " + c.usage + std::string(width - c.usage.size() + 3, ' ') + c.description);
    }
}

void CommandInterpreter::cmdStart() {
    if (marquee_.start()) print_(MsgType::Success, "Marquee started.");
    else                  print_(MsgType::Info, "Marquee is already running.");
}

void CommandInterpreter::cmdStop() {
    if (marquee_.stop()) print_(MsgType::Success, "Marquee stopped.");
    else                 print_(MsgType::Info, "Marquee is already stopped.");
}

void CommandInterpreter::cmdSetText(const std::string& args) {
    if (args.empty()) {
        // "Support both" styles: no text -> ask for it on the next line.
        pending_ = Pending::Text;
        print_(MsgType::Info, "Type the new marquee text and press Enter (blank line cancels).");
        return;
    }
    applyText(args);
}

void CommandInterpreter::applyText(const std::string& raw) {
    std::string text = sanitise(stripQuotes(trim(raw)));
    if (trim(text).empty()) {
        print_(MsgType::Error, "Marquee text cannot be empty. Text was not changed.");
        return;
    }
    marquee_.setText(text);
    print_(MsgType::Success, "Marquee text set to: \"" + text + "\"");
}

void CommandInterpreter::cmdSetSpeed(const std::string& args) {
    if (args.empty()) {
        pending_ = Pending::Speed;
        print_(MsgType::Info, "Type the new speed in milliseconds (" +
                              std::to_string(Marquee::MIN_SPEED_MS) + "-" +
                              std::to_string(Marquee::MAX_SPEED_MS) + ") and press Enter (blank line cancels).");
        return;
    }
    applySpeed(args);
}

void CommandInterpreter::applySpeed(const std::string& raw) {
    if (trim(raw).empty()) {
        print_(MsgType::Info, "set_speed cancelled. Speed is still " +
                              std::to_string(marquee_.getSpeed()) + " ms.");
        return;
    }
    int ms = 0;
    std::string error;
    if (!parseSpeed(raw, ms, error)) {
        print_(MsgType::Error, error + " Speed is still " + std::to_string(marquee_.getSpeed()) + " ms.");
        return;
    }
    marquee_.setSpeed(ms);
    print_(MsgType::Success, "Marquee speed set to " + std::to_string(ms) + " ms.");
}

void CommandInterpreter::cmdExit() {
    print_(MsgType::Info, "Exiting CSOPESY. Goodbye!");
    exit_ = true;
}
