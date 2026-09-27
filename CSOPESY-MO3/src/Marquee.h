#pragma once
// Marquee.h
/*
// ---------------------------------------------------------------------------
   The marquee's state (text, speed, running, position) and its animation
   logic. It knows nothing about the screen -- it just answers "what should
   the strip look like right now?" via render().

   Two threads use this object at the same time:
    - the ANIMATION thread calls waitForNextTick() / tick() / render()
    - the INPUT thread (through the interpreter) calls start/stop/setText...
   Every method locks mtx_ so they never read/write the data at once.
// ---------------------------------------------------------------------------
*/
#include <condition_variable>
#include <mutex>
#include <string>

class Marquee {
public:
    // Allowed range for set_speed, in milliseconds
    static constexpr int MIN_SPEED_MS = 1;
    static constexpr int MAX_SPEED_MS = 60000;   // 1 minute

    // Which way the text travels across the strip
    enum class Direction { LeftToRight, RightToLeft };

    explicit Marquee(std::string text = "Hello world in marquee!", int speedMs = 100);

    // start()/stop() return false if the marquee was ALREADY in that state,
    // so the interpreter can tell the user instead of silently doing nothing
    bool start();
    bool stop();
    bool isRunning() const;

    void setText(const std::string& text);
    std::string getText() const;

    void setSpeed(int speedMs);
    int getSpeed() const;

    void setDirection(Direction direction);

    // ---- Animation (used by the animation thread) ----

    // Sleeps for one "speed" interval. Wakes up EARLY if the settings change
    // (so a new speed applies right away) or if shutdown() is called
    // Returns false once the program is shutting down
    bool waitForNextTick();

    // Moves the text one column (only if running)
    void tick();

    // Returns exactly 'width' characters: the visible part of the strip
    std::string render(int width) const;

    // Tells the animation thread to finish (used by 'exit')
    void shutdown();

private:
    void wake();   // wakes waitForNextTick() up early. Caller holds mtx_

    mutable std::mutex mtx_;          // "mutable" lets const methods lock it
    std::condition_variable wakeUp_;  // what waitForNextTick() sleeps on

    std::string text_;
    int speedMs_;
    bool running_ = false;
    Direction direction_ = Direction::LeftToRight;
    long long offset_ = 0;            // how many steps the text has moved

    bool changed_ = false;            // settings changed -> wake up early
    bool shutdown_ = false;           // program is exiting
};
