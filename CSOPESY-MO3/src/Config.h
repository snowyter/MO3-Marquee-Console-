#pragma once
// Config.h
// ---------------------------------------------------------------------------
// All the tunable settings of the program, and the function that loads them
// from config.txt at startup.
//
// WHY: during the quiz you may NOT recompile. Anything a test case might ask
// you to change (starting text, speed, polling rate, ...) must be editable in
// config.txt instead of in the code.
//
// STATUS: finished -- AppConfig holds the defaults below, and loadConfig()
// in Config.cpp fills them in from config.txt (missing file = keep defaults).
// ---------------------------------------------------------------------------
#include <string>
#include <vector>

#include "Marquee.h"

// Every tunable number in one place. The values written here are the
// DEFAULTS, used when config.txt is missing or a line in it is invalid.
struct AppConfig {
    std::string marqueeText = "Hello world in marquee!";
    int marqueeSpeedMs = 100;       // animation refresh (set_speed changes it)
    int pollMs = 10;                // how often the keyboard is checked
    bool startRunning = false;      // animate right away, before start_marquee?
    Marquee::Direction direction = Marquee::Direction::LeftToRight;
    size_t maxInputLength = 256;    // longest command the user can type
};

// Allowed range for poll_ms in config.txt.
constexpr int MIN_POLL_MS = 1;
constexpr int MAX_POLL_MS = 1000;

// Reads 'path' (normally "config.txt") and overwrites the matching fields of
// 'config'. Fields not mentioned in the file keep their defaults.
//
// Returns true if the file was found and read, false if it could not be
// opened (then 'config' is left completely unchanged).
//
// Every problem (unknown key, bad value, ...) is added to 'warnings' as a
// human-readable sentence; main() shows them in red in the output log.
// A bad line must NEVER stop the program -- skip it and keep the default.
bool loadConfig(const std::string& path, AppConfig& config, std::vector<std::string>& warnings);
