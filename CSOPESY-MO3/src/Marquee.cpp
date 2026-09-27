// Marquee.cpp
#include "Marquee.h"

#include <algorithm> // std::max
#include <chrono>
#include <utility>   // std::move

// std::lock_guard locks the mutex when it's created and unlocks it
// automatically when the function returns -- you can't forget to unlock

namespace {
    // Blank columns between the end of the text and the start of its next pass (so long texts don't run straight into themselves)
    const int GAP = 5;
}

Marquee::Marquee(std::string text, int speedMs)
    : text_(std::move(text)), speedMs_(speedMs) {}

void Marquee::wake() {
    changed_ = true;
    wakeUp_.notify_all();
}

bool Marquee::start() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (running_) return false;
    running_ = true;
    wake();
    return true;
}

bool Marquee::stop() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (!running_) return false;
    running_ = false;
    wake();
    return true;
}

bool Marquee::isRunning() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return running_;
}

void Marquee::setText(const std::string& text) {
    std::lock_guard<std::mutex> lock(mtx_);
    text_ = text;
    offset_ = 0;   // new text starts from the beginning of the strip
    wake();
}

std::string Marquee::getText() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return text_;
}

void Marquee::setSpeed(int speedMs) {
    std::lock_guard<std::mutex> lock(mtx_);
    speedMs_ = speedMs;
    wake();   // don't make the user wait out the OLD (maybe very long) delay
}

int Marquee::getSpeed() const {
    std::lock_guard<std::mutex> lock(mtx_);
    return speedMs_;
}

void Marquee::setDirection(Direction direction) {
    std::lock_guard<std::mutex> lock(mtx_);
    direction_ = direction;
}

bool Marquee::waitForNextTick() {
    // condition_variable needs a unique_lock (a lock it can release while
    // sleeping, so other threads can still call setSpeed() etc.)
    std::unique_lock<std::mutex> lock(mtx_);

    // Sleep up to speedMs_, but stop early if changed_ or shutdown_ becomes
    // true (someone called wake() / shutdown()).
    wakeUp_.wait_for(lock, std::chrono::milliseconds(speedMs_),
                     [this] { return changed_ || shutdown_; });

    changed_ = false;
    return !shutdown_;
}

void Marquee::tick() {
    std::lock_guard<std::mutex> lock(mtx_);
    if (running_) ++offset_;   // stopped = frozen in place
}

std::string Marquee::render(int width) const {
    std::lock_guard<std::mutex> lock(mtx_);
    if (width <= 0) return "";

    // Picture the text written on a loop of paper 'loop' characters long:
    //   [text][blank padding up to 'loop'] -> then it repeats.
    // If the text is shorter than the strip, the loop is exactly the strip,
    // so the text wraps around from one edge to the other. If it's longer,
    // the loop is text + GAP, and the strip shows a sliding window of it
    const long long textLen = static_cast<long long>(text_.size());
    const long long loop = std::max<long long>(width, textLen + GAP);

    std::string strip(static_cast<size_t>(width), ' ');
    for (int col = 0; col < width; ++col) {
        // Which character of the loop is under this column right now?
        long long index = (direction_ == Direction::LeftToRight)
                              ? col - offset_    // text shifts right as offset grows
                              : col + offset_;   // text shifts left as offset grows
        index = ((index % loop) + loop) % loop;  // wrap into 0..loop-1 (never negative)
        if (index < textLen) strip[static_cast<size_t>(col)] = text_[static_cast<size_t>(index)];
    }
    return strip;
}

void Marquee::shutdown() {
    std::lock_guard<std::mutex> lock(mtx_);
    shutdown_ = true;
    wakeUp_.notify_all();
}
