// Config.cpp
/*
// ---------------------------------------------------------------------------
   Reads config.txt at startup so the settings can be changed WITHOUT
   recompiling (the quiz says: only the parameters may change).

   config.txt format (one setting per line):

       # lines starting with # are comments, blank lines are ignored
       text=Hello world in marquee!
       speed_ms=100
       poll_ms=10
       start_running=false
       direction=left_to_right

   Every problem (unknown key, bad value, ...) becomes a warning in `warnings`
   and the setting keeps its default -- one bad line never stops the program.

   Cases this handles (expected result in []):
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

// Cuts spaces, tabs, '\r' and '\n' off the START and END of `s`.
// Spaces in the middle stay ("  two  spaces " -> "two  spaces").
std::string trimSpaces(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return s.substr(start, end - start);
}

// A lowercase copy of `s` ("TRUE" -> "true"). Used for keys and for the
// values of start_running and direction -- never for the marquee text.
std::string toLowerCopy(const std::string& s) {
    std::string out = s;
    for (char& c : out) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return out;
}

// Turns `text` into a whole number and stores it in `out`.
// True only if `text` is all digits and the number is between min and max
// (inclusive); on false `out` is left alone.
bool parseIntInRange(const std::string& text, int min, int max, int& out) {
    if (text.empty()) return false;

    for (char c : text) {
        if (!std::isdigit(static_cast<unsigned char>(c))) return false;
    }

    // Check the length BEFORE converting, so a huge number like
    // "99999999999999" can't overflow an int (same trick as parseSpeed()).
    size_t firstNonZero = text.find_first_not_of('0');
    std::string digits = (firstNonZero == std::string::npos) ? "0" : text.substr(firstNonZero);
    if (digits.size() > 9) return false;

    long long value = std::stoll(digits);
    if (value < min || value > max) return false;

    out = static_cast<int>(value);
    return true;
}

// Accepts true/false, yes/no, on/off and 1/0 (any letter case).
bool parseBool(const std::string& text, bool& out) {
    std::string value = toLowerCopy(trimSpaces(text));

    if (value == "true"  || value == "yes" || value == "on"  || value == "1") { out = true;  return true; }
    if (value == "false" || value == "no"  || value == "off" || value == "0") { out = false; return true; }
    return false;
}

// Accepts left_to_right / right_to_left (any letter case; '-' works like '_',
// the same way command names do).
bool parseDirection(const std::string& text, Marquee::Direction& out) {
    std::string value = toLowerCopy(trimSpaces(text));
    for (char& c : value) if (c == '-') c = '_';

    if (value == "left_to_right") { out = Marquee::Direction::LeftToRight; return true; }
    if (value == "right_to_left") { out = Marquee::Direction::RightToLeft; return true; }
    return false;
}

} // namespace

// Reads `path` line by line and overwrites the matching fields of `config`.
// Returns false (and leaves `config` untouched) if the file can't be opened.
bool loadConfig(const std::string& path, AppConfig& config, std::vector<std::string>& warnings) {
    std::ifstream file(path);
    if (!file) return false;   // no file -> the caller decides what to say

    std::string line;
    int lineNo = 0;
    while (std::getline(file, line)) {
        ++lineNo;

        // A UTF-8 byte-order mark (some Windows editors add one) would glue
        // itself to the first key, so drop it before anything else.
        if (lineNo == 1 && line.size() >= 3 &&
            static_cast<unsigned char>(line[0]) == 0xEF &&
            static_cast<unsigned char>(line[1]) == 0xBB &&
            static_cast<unsigned char>(line[2]) == 0xBF) {
            line = line.substr(3);
        }

        line = trimSpaces(line);
        if (line.empty() || line[0] == '#') continue;   // blank line or comment

        size_t eq = line.find('=');
        if (eq == std::string::npos) {
            warnings.push_back("config.txt line " + std::to_string(lineNo) + ": expected key=value");
            continue;
        }

        std::string key = toLowerCopy(trimSpaces(line.substr(0, eq)));
        std::string value = trimSpaces(line.substr(eq + 1));   // text is NOT lowercased
        if (key.empty()) {
            warnings.push_back("config.txt line " + std::to_string(lineNo) + ": expected key=value");
            continue;
        }

        if (key == "text") {
            if (value.empty()) {
                warnings.push_back("config.txt line " + std::to_string(lineNo) +
                                   ": text cannot be empty; using \"" + config.marqueeText + "\".");
            } else {
                config.marqueeText = value;
            }
        } else if (key == "speed_ms") {
            int ms = 0;
            if (parseIntInRange(value, Marquee::MIN_SPEED_MS, Marquee::MAX_SPEED_MS, ms)) {
                config.marqueeSpeedMs = ms;
            } else {
                warnings.push_back("config.txt line " + std::to_string(lineNo) + ": speed_ms '" + value +
                                   "' is not a whole number from " + std::to_string(Marquee::MIN_SPEED_MS) +
                                   " to " + std::to_string(Marquee::MAX_SPEED_MS) + "; using " +
                                   std::to_string(config.marqueeSpeedMs) + ".");
            }
        } else if (key == "poll_ms") {
            int ms = 0;
            if (parseIntInRange(value, MIN_POLL_MS, MAX_POLL_MS, ms)) {
                config.pollMs = ms;
            } else {
                warnings.push_back("config.txt line " + std::to_string(lineNo) + ": poll_ms '" + value +
                                   "' is not a whole number from " + std::to_string(MIN_POLL_MS) +
                                   " to " + std::to_string(MAX_POLL_MS) + "; using " +
                                   std::to_string(config.pollMs) + ".");
            }
        } else if (key == "start_running") {
            bool flag = false;
            if (parseBool(value, flag)) {
                config.startRunning = flag;
            } else {
                warnings.push_back("config.txt line " + std::to_string(lineNo) + ": start_running '" + value +
                                   "' is not true/false, yes/no, on/off or 1/0; using " +
                                   (config.startRunning ? "true." : "false."));
            }
        } else if (key == "direction") {
            Marquee::Direction dir = config.direction;
            if (parseDirection(value, dir)) {
                config.direction = dir;
            } else {
                warnings.push_back("config.txt line " + std::to_string(lineNo) + ": direction '" + value +
                                   "' is not left_to_right or right_to_left; using " +
                                   (config.direction == Marquee::Direction::LeftToRight ? "left_to_right."
                                                                                        : "right_to_left."));
            }
        } else {
            warnings.push_back("config.txt line " + std::to_string(lineNo) + ": unknown setting '" + key + "'");
        }
    }

    return true;
}
