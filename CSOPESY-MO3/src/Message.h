#pragma once
// Message.h
/*
// ---------------------------------------------------------------------------
   The kinds of messages the program can show. The interpreter only says
   WHAT kind of message it is; the Console/Display decides HOW it looks
   (color). Keeping this separate means the display can change without
   touching the interpreter at all.
   ---------------------------------------------------------------------------
*/

enum class MsgType {
    Command,  // echo of what the user typed, e.g. "Command> help"
    Info,     // normal output (help text, notices)
    Success,  // a command did what was asked
    Error     // bad command / bad argument
};
