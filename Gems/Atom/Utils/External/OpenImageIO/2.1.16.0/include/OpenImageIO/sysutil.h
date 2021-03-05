// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/////////////////////////////////////////////////////////////////////////
/// @file  sysutil.h
///
/// @brief Platform-independent utilities for various OS, hardware, and
/// system resource functionality, all in namespace Sysutil.
/////////////////////////////////////////////////////////////////////////


#pragma once

#include <ctime>
#include <string>

#ifdef __MINGW32__
#    include <malloc.h>  // for alloca
#endif

#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/platform.h>
#include <OpenImageIO/string_view.h>

// Allow client software to know if this version has Sysutil::stacktrace().
#define OIIO_HAS_STACKTRACE 1



OIIO_NAMESPACE_BEGIN

/// @namespace  Sysutil
///
/// @brief Platform-independent utilities for various OS, hardware, and
/// system resource functionality.
namespace Sysutil {

/// The amount of memory currently being used by this process, in bytes.
/// If resident==true (the default), it will report just the resident
/// set in RAM; if resident==false, it returns the full virtual arena
/// (which can be misleading because gcc allocates quite a bit of
/// virtual, but not actually resident until malloced, memory per
/// thread).
OIIO_API size_t
memory_used(bool resident = true);

/// The amount of physical RAM on this machine, in bytes.
/// If it can't figure it out, it will return 0.
OIIO_API size_t
physical_memory();

/// Convert calendar time pointed by 'time' into local time and save it in
/// 'converted_time' variable
OIIO_API void
get_local_time(const time_t* time, struct tm* converted_time);

/// Return the full path of the currently-running executable program.
///
OIIO_API std::string
this_program_path();

/// Return the value of an environment variable (or the empty string_view
/// if it is not found in the environment.)
OIIO_API string_view
getenv(string_view name);

/// Sleep for the given number of microseconds.
///
OIIO_API void
usleep(unsigned long useconds);

/// Try to put the process into the background so it doesn't continue to
/// tie up any shell that it was launched from.  The arguments are the
/// argc/argv that describe the program and its command line arguments.
/// Return true if successful, false if it was unable to do so.
OIIO_API bool
put_in_background(int argc, char* argv[]);

/// Number of virtual cores available on this platform (including
/// hyperthreads).
OIIO_API unsigned int
hardware_concurrency();

/// Number of full hardware cores available on this platform (does not
/// include hyperthreads). This is not always accurate and on some
/// platforms will return the number of virtual cores.
OIIO_API unsigned int
physical_concurrency();

/// Get the maximum number of open file handles allowed on this system.
OIIO_API size_t
max_open_files();

/// Return a string containing a readable stack trace from the point where
/// it was called. Return an empty string if not supported on this platform.
OIIO_API std::string
stacktrace();

/// Turn on automatic stacktrace dump to the named file if the program
/// crashes. Return true if this is properly set up, false if it is not
/// possible on this platform. The name may be "stdout" or "stderr" to
/// merely print the trace to stdout or stderr, respectively. If the name
/// is "", it will disable the auto-stacktrace printing.
OIIO_API bool
setup_crash_stacktrace(string_view filename);

/// Try to figure out how many columns wide the terminal window is. May not
/// be correct on all systems, will default to 80 if it can't figure it out.
OIIO_API int
terminal_columns();

/// Try to figure out how many rows tall the terminal window is. May not be
/// correct on all systems, will default to 24 if it can't figure it out.
OIIO_API int
terminal_rows();


/// Term object encapsulates information about terminal output for the sake
/// of constructing ANSI escape sequences.
class OIIO_API Term {
public:
    /// Default ctr: assume ANSI escape sequences are ok.
    Term() noexcept {}
    /// Construct from a FILE*: ANSI codes ok if the file describes a
    /// live console, otherwise they will be supressed.
    Term(FILE* file);
    /// Construct from a stream: ANSI codes ok if the file describes a
    /// live console, otherwise they will be supressed.
    Term(const std::ostream& stream);

    /// ansi("appearance") returns the ANSI escape sequence for the named
    /// command (if ANSI codes are ok, otherwise it will return the empty
    /// string). Accepted commands include: "default", "bold", "underscore",
    /// "blink", "reverse", "concealed", "black", "red", "green", "yellow",
    /// "blue", "magenta", "cyan", "white", "black_bg", "red_bg",
    /// "green_bg", "yellow_bg", "blue_bg", "magenta_bg", "cyan_bg",
    /// "white_bg". Commands may be combined with "," for example:
    /// "bold,green,white_bg".
    std::string ansi(string_view command) const;

    /// ansi("appearance", "text") returns the text, with the formatting
    /// command, then the text, then the formatting command to return to
    /// default appearance.
    std::string ansi(string_view command, string_view text) const
    {
        return std::string(ansi(command)) + std::string(text) + ansi("default");
    }

    /// Extended color control: take RGB values from 0-255
    std::string ansi_fgcolor(int r, int g, int b);
    std::string ansi_bgcolor(int r, int g, int b);

    bool is_console() const noexcept { return m_is_console; }

private:
    bool m_is_console = true;  // Default: assume ANSI escape sequences ok.
};


}  // namespace Sysutil

OIIO_NAMESPACE_END
