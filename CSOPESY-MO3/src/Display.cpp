// Display.cpp
#include "Display.h"

#include <vector>

#include "Console.h"

namespace {
    const size_t MAX_LOG = 500;   // keep at most this many messages

    // Cuts 's' to exactly 'width' characters, padding with 'fill' if short
    std::string fitTo(const std::string& s, int width, char fill) {
        if (width <= 0) return "";
        std::string out = s.substr(0, static_cast<size_t>(width));
        out.resize(static_cast<size_t>(width), fill);
        return out;
    }

    // Splits a long message into pieces that each fit on one screen row,
    // breaking at a space when possible so words aren't cut in half
    std::vector<std::string> wrap(const std::string& text, int width) {
        std::vector<std::string> pieces;
        std::string rest = text;
        const size_t w = static_cast<size_t>(width);
        while (rest.size() > w) {
            size_t cut = rest.rfind(' ', w);                     // last space that fits
            if (cut == std::string::npos || cut == 0) cut = w;   // one huge word: hard cut
            pieces.push_back(rest.substr(0, cut));
            rest = rest.substr(cut);
            if (!rest.empty() && rest[0] == ' ') rest = "  " + rest.substr(1);  // indent continuation
        }
        pieces.push_back(rest);
        return pieces;
    }
}

Display::Display(Marquee& marquee) : marquee_(marquee) {}

void Display::addMessage(MsgType type, const std::string& text) {
    std::lock_guard<std::mutex> lock(mtx_);
    log_.push_back({type, text});
    if (log_.size() > MAX_LOG) log_.pop_front();
}

void Display::setInput(const std::string& prompt, const std::string& typed) {
    std::lock_guard<std::mutex> lock(mtx_);
    prompt_ = prompt;
    typed_ = typed;
}

void Display::draw() {
    std::lock_guard<std::mutex> lock(mtx_);

    int cols = 0, rows = 0;
    Console::getSize(cols, rows);
    if (cols < 20 || rows < 6) return;   // window too tiny to draw into

    // Never write into the very last column
    const int width = cols - 1;

    // ---------- Build the pieces of the frame ----------
    std::vector<Console::Line> header = Console::headerLines();

    const std::string status = marquee_.isRunning() ? "RUNNING" : "STOPPED";
    const std::string title = "+-- MARQUEE [" + status + "]  speed: " +
                              std::to_string(marquee_.getSpeed()) + " ms ";
    std::vector<Console::Line> box = {
        {"\033[36m", fitTo(title, width - 1, '-') + "+"},
        {"\033[33m", "| " + marquee_.render(width - 4) + " |"},
        {"\033[36m", "+" + std::string(static_cast<size_t>(width - 2), '-') + "+"},
        {"", ""},
    };

    // prompt + typed text
    std::string inputLine = prompt_ + typed_;
    if (static_cast<int>(inputLine.size()) > width) {
        inputLine = "<" + inputLine.substr(inputLine.size() - static_cast<size_t>(width - 1));
    }

    // ---------- Decide how much fits ----------
    // If the window is short, drop the header so the marquee, output and prompt still fit
    int fixedRows = static_cast<int>(box.size()) + 1;       // box + prompt
    int logRows = rows - fixedRows - static_cast<int>(header.size());
    if (logRows < 3) {
        header.clear();
        logRows = rows - fixedRows;
    }

    // Take log lines from the NEWEST backwards until the space is full
    std::vector<Console::Line> logLines;
    for (auto it = log_.rbegin(); it != log_.rend() && static_cast<int>(logLines.size()) < logRows; ++it) {
        std::vector<std::string> pieces = wrap(it->text, width);
        for (auto p = pieces.rbegin(); p != pieces.rend() && static_cast<int>(logLines.size()) < logRows; ++p) {
            logLines.push_back({Console::colorFor(it->type), *p});
        }
    }

    // ---------- Put the rows in order: header, box, log (oldest first), input ----------
    std::vector<Console::Line> frameRows = header;
    frameRows.insert(frameRows.end(), box.begin(), box.end());
    frameRows.insert(frameRows.end(), logLines.rbegin(), logLines.rend());
    frameRows.push_back({"", inputLine});

    // ---------- Turn the rows into one string of text + ANSI codes ----------
    std::string frame;
    frame.reserve(static_cast<size_t>(cols * rows * 2));
    frame += "\033[?25l";                        // hide cursor while drawing

    if (cols != lastCols_ || rows != lastRows_) {
        frame += "\033[2J";                      // window resized: wipe leftovers
        lastCols_ = cols;
        lastRows_ = rows;
    }
    frame += "\033[H";                           // cursor to top-left (row 1, col 1)

    for (size_t i = 0; i < frameRows.size(); ++i) {
        const Console::Line& row = frameRows[i];
        if (*row.color) frame += row.color;
        frame += row.text.substr(0, static_cast<size_t>(width));
        if (*row.color) frame += Console::RESET;
        frame += "\033[K";                       // erase rest of this row (old text)
        if (i + 1 < frameRows.size()) frame += "\r\n";
    }
    frame += "\033[J";                           // erase everything below the prompt

    // Put the blinking cursor right after what the user typed
    frame += "\033[" + std::to_string(frameRows.size()) + ";" +
             std::to_string(inputLine.size() + 1) + "H";
    frame += "\033[?25h";                        // show cursor again

    // ---------- One write = the whole frame appears at once ----------
    Console::write(frame);
}
