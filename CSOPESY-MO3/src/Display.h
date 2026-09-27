#pragma once
// Display.h
/*
// ---------------------------------------------------------------------------
   Draws the whole screen ("a frame") from the current state:

       header (banner, members, version)
       +-- MARQUEE [RUNNING] speed: 100 ms --------+
       | the scrolling text                        |
       +-------------------------------------------+

       output log (recent commands and their results)
       Command> what the user is typing_

   This is "immediate mode": we don't edit bits of the screen here and there.
   Every draw() rebuilds the frame from scratch and sends it in ONE write.
  
   Both threads call draw() (animation thread every tick, input thread on
   every key press). A mutex makes sure only one frame is being built and
   written at a time, so two frames can never get mixed together.
// ---------------------------------------------------------------------------
*/
#include <deque>
#include <mutex>
#include <string>

#include "Marquee.h"
#include "Message.h"

class Display {
public:
    explicit Display(Marquee& marquee);

    // Adds a line to the output log (thread-safe)
    void addMessage(MsgType type, const std::string& text);

    // Updates the bottom line: the prompt and what's been typed so far
    void setInput(const std::string& prompt, const std::string& typed);

    // Builds and shows one frame
    void draw();

private:
    struct LogEntry {
        MsgType type;
        std::string text;
    };

    Marquee& marquee_;
    std::mutex mtx_;              // protects everything below + the screen

    std::deque<LogEntry> log_;    // newest at the back
    std::string prompt_ = "Command> ";
    std::string typed_;

    int lastCols_ = 0;            // window size last frame, to spot resizes
    int lastRows_ = 0;
};
