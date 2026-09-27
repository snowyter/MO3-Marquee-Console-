// Config.cpp
/*
// ---------------------------------------------------------------------------
   STUB FILE -- to be completed by the group (Step 5).
   Search for "TODO(groupmate)". Each function says exactly what it must do.
   The program already compiles and runs with these stubs: loadConfig() just
   reports "not implemented" by returning false, so the defaults are used.

   config.txt format (one setting per line):
  
       # lines starting with # are comments, blank lines are ignored
       text=Hello world in marquee!
       speed_ms=100
       poll_ms=10
       start_running=false
       direction=left_to_right

   Test your finished loader against these cases (expected result in []):
       speed_ms=250            [speed 250]
       speed_ms = 250          [speed 250 -- spaces around '=' are allowed]
       speed_ms=abc            [warning, speed stays 100]
       speed_ms=0              [warning, out of range 1-60000]
       poll_ms=5000            [warning, out of range 1-1000]
       start_running=TRUE      [true -- case-insensitive]
       start_running=maybe     [warning, stays false]
       text=                   [warning, text cannot be empty]
       text=  two  spaces      [text is "two  spaces" -- inner spaces kept]
       colour=red              [warning, unknown key 'colour']
       no equals sign here     [warning, line N has no '=']
       (file missing)          [loadConfig returns false, no crash]
// ---------------------------------------------------------------------------
*/
#include "Config.h"

#include <cctype>    // std::isspace, std::isdigit, std::tolower
#include <fstream>   // std::ifstream  (reading a file line by line)
#include <string>

namespace {

// ---------------------------------------------------------------------------
// TODO(groupmate): trimSpaces
// Return `s` without spaces, tabs, '\r' or '\n' at the START and END.
// Keep spaces in the middle ("  two  spaces " -> "two  spaces").
// Hint: CommandInterpreter.cpp already has a trim() you can copy.
// ---------------------------------------------------------------------------
[[maybe_unused]] std::string trimSpaces(const std::string& s) {
    // TODO(groupmate): implement. Returning `s` unchanged for now.
    return s;
}

// ---------------------------------------------------------------------------
// TODO(groupmate): toLowerCopy
// Return a lowercase copy of `s` ("TRUE" -> "true"). Used for keys and for
// the values of start_running and direction.
// Hint: CommandInterpreter.cpp already has a toLower() you can copy.
// ---------------------------------------------------------------------------
[[maybe_unused]] std::string toLowerCopy(const std::string& s) {
    // TODO(groupmate): implement. Returning `s` unchanged for now.
    return s;
}

// ---------------------------------------------------------------------------
// TODO(groupmate): parseIntInRange
// Turn `text` into a whole number and store it in `out`.
// Return true only if ALL of these hold:
//   - `text` is not empty and contains only digits 0-9 (no '-', '.', spaces)
//   - the number is between `min` and `max` (inclusive)
// Otherwise return false and leave `out` unchanged.
// Watch out: "99999999999999" must not overflow an int -- reject any number
// with more than 9 digits BEFORE converting (see parseSpeed() in
// CommandInterpreter.cpp for the same trick).
// ---------------------------------------------------------------------------
[[maybe_unused]] bool parseIntInRange(const std::string& text, int min, int max, int& out) {
    // TODO(groupmate): implement.
    (void)text; (void)min; (void)max; (void)out;   // silences "unused" warnings; delete when done
    return false;
}

// ---------------------------------------------------------------------------
// TODO(groupmate): parseBool
// Accept (case-insensitive): true/false, yes/no, on/off, 1/0.
// Store the result in `out` and return true; for anything else return false
// and leave `out` unchanged.
// ---------------------------------------------------------------------------
[[maybe_unused]] bool parseBool(const std::string& text, bool& out) {
    // TODO(groupmate): implement.
    (void)text; (void)out;
    return false;
}

// ---------------------------------------------------------------------------
// TODO(groupmate): parseDirection
// Accept (case-insensitive) "left_to_right" or "right_to_left" and store
// Marquee::Direction::LeftToRight / RightToLeft in `out`. Return true on
// success, false otherwise (leave `out` unchanged).
// ---------------------------------------------------------------------------
[[maybe_unused]] bool parseDirection(const std::string& text, Marquee::Direction& out) {
    // TODO(groupmate): implement.
    (void)text; (void)out;
    return false;
}

} // namespace

// ---------------------------------------------------------------------------
// TODO(groupmate): loadConfig  (the main job -- uses all the helpers above)
//
// Steps:
//   1. Open the file:   std::ifstream file(path);
//      If it can't be opened (if (!file)), return false straight away.
//   2. Read it line by line:   std::string line; int lineNo = 0;
//                              while (std::getline(file, line)) { ++lineNo; ... }
//   3. For each line:
//        a. line = trimSpaces(line). Skip it if empty or if it starts with '#'.
//        b. Find '=' (line.find('=')). If there is none, add the warning
//           "config.txt line N: expected key=value" and continue.
//        c. key   = toLowerCopy(trimSpaces(part before '='))
//           value = trimSpaces(part after '=')   (do NOT lowercase the text!)
//        d. Handle each key -- on a bad value, add a warning and keep the default:
//             "text"          -> must not be empty           -> config.marqueeText
//             "speed_ms"      -> parseIntInRange(value, Marquee::MIN_SPEED_MS,
//                                  Marquee::MAX_SPEED_MS, ...) -> config.marqueeSpeedMs
//             "poll_ms"       -> parseIntInRange(value, MIN_POLL_MS, MAX_POLL_MS, ...)
//                                                              -> config.pollMs
//             "start_running" -> parseBool(...)               -> config.startRunning
//             "direction"     -> parseDirection(...)          -> config.direction
//             anything else   -> warning "config.txt line N: unknown setting 'key'"
//   4. Return true.
//
// Warning text should say the line number, the problem, and that the default
// is being used, e.g.:
//   "config.txt line 3: speed_ms 'abc' is not a whole number from 1 to 60000; using 100."
// ---------------------------------------------------------------------------
bool loadConfig(const std::string& path, AppConfig& config, std::vector<std::string>& warnings) {
    // TODO(groupmate): implement (see the steps above). Until then this stub
    // reads nothing, so the program runs with the defaults from Config.h.
    (void)path; (void)config; (void)warnings;
    return false;
}
