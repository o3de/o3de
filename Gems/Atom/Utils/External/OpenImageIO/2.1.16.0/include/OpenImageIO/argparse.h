// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/// \file
/// \brief Simple parsing of program command-line arguments.

#pragma once

#if defined(_MSC_VER)
// Ignore warnings about DLL exported classes with member variables that are template classes.
// This happens with the std::string m_errmessage member of ArgParse below.
#    pragma warning(disable : 4251)
#endif

#include <functional>
#include <memory>
#include <vector>

#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/strutil.h>


OIIO_NAMESPACE_BEGIN


class ArgOption;  // Forward declaration



/////////////////////////////////////////////////////////////////////////////
///
/// \class ArgParse
///
/// Argument Parsing
///
/// The parse function takes a list of options and variables or functions
/// for storing option values and return <0 on failure:
///
/// \code
///    static int parse_files (int argc, const char *argv[])
///    {
///        for (int i = 0;  i < argc;  i++)
///            filenames.push_back (argv[i]);
///        return 0;
///    }
///
///    static int blah_callback (int argc, const char *argv[])
///    {
///        std::cout << "blah argument was " << argv[1] << "\n";
///        return 0;
///    }
///
///    ...
///
///    ArgParse ap;
///
///    ap.options ("Usage: myapp [options] filename...",
///            "%*", parse_objects, "",
///            "-camera %f %f %f", &camera[0], &camera[1], &camera[2],
///                  "set the camera position",
///            "-lookat %f %f %f", &lx, &ly, &lz,
///                  "set the position of interest",
///            "-oversampling %d", &oversampling,  "oversamping rate",
///            "-passes %d", &passes, "number of passes",
///            "-lens %f %f %f", &aperture, &focalDistance, &focalLength,
///                   "set aperture, focal distance, focal length",
///            "-format %d %d %f", &width, &height, &aspect,
///                   "set width, height, aspect ratio",
///            "-v", &verbose, "verbose output",
///            "-q %!", &verbose, "quiet mode",
///            "--blah %@ %s", blahcallback, "Make the callback",
///            NULL);
///
///    if (ap.parse (argc, argv) < 0) {
///        std::cerr << ap.geterror() << std::endl;
///        ap.usage ();
///        return EXIT_FAILURE;
///    }
/// \endcode
///
/// The available argument types are:
///    - no \% argument - bool flag
///    - \%! - a bool flag, but set it to false if the option is set
///    - \%d - 32bit integer
///    - \%f - 32bit float
///    - \%F - 64bit float (double)
///    - \%s - std::string
///    - \%L - std::vector<std::string>  (takes 1 arg, appends to list)
///    - \%@ - a function pointer for a callback function will be invoked
///            immediately.  The prototype for the callback is
///                  int callback (int argc, char *argv[])
///    - \%* - catch all non-options and pass individually as an (argc,argv)
///            sublist to a callback, each immediately after it's found
///    - \%1 - catch all non-options that occur before any option is
///            encountered (like %*, but only for those prior to the first
///            real option.
///
/// The argument type specifier may be followed by an optional colon (:) and
/// then a human-readable parameter that will be printed in the help
/// message. For example,
///
///     "--foo %d:WIDTH %d:HEIGHT", ...
///
/// indicates that `--foo` takes two integer arguments, and the help message
/// will be rendered as:
///
///     --foo WIDTH HEIGHT    Set the foo size
///
/// There are several special format tokens:
///    - "<SEPARATOR>" - not an option at all, just a description to print
///                     in the usage output.
///
/// Notes:
///   - If an option doesn't have any arguments, a bool flag argument is
///     assumed.
///   - No argument destinations are initialized.
///   - The empty string, "", is used as a global sublist (ie. "%*").
///   - Sublist functions are all of the form "int func(int argc, char **argv)".
///   - If a sublist function returns -1, parse() will terminate early.
///   - It is perfectly legal for the user to append ':' and more characters
///     to the end of an option name, it will match only the portion before
///     the semicolon (but a callback can detect the full string, this is
///     useful for making arguments:  myprog --flag:myopt=1 foobar
///
/////////////////////////////////////////////////////////////////////////////


class OIIO_API ArgParse {
public:
    ArgParse(int argc = 0, const char** argv = NULL);
    ~ArgParse();

    /// Declare the command line options.  After the introductory
    /// message, parameters are a set of format strings and variable
    /// pointers.  Each string contains an option name and a scanf-like
    /// format string to enumerate the arguments of that option
    /// (eg. "-option %d %f %s").  The format string is followed by a
    /// list of pointers to the argument variables, just like scanf.  A
    /// NULL terminates the list.  Multiple calls to options() will
    /// append additional options.
    int options(const char* intro, ...);

    /// With the options already set up, parse the command line.
    /// Return 0 if ok, -1 if it's a malformed command line.
    int parse(int argc, const char** argv);

    /// Return any error messages generated during the course of parse()
    /// (and clear any error flags).  If no error has occurred since the
    /// last time geterror() was called, it will return an empty string.
    std::string geterror() const;

    /// Print the usage message to stdout.  The usage message is
    /// generated and formatted automatically based on the command and
    /// description arguments passed to parse().
    void usage() const;

    /// Print a brief usage message to stdout.  The usage message is
    /// generated and formatted automatically based on the command and
    /// description arguments passed to parse().
    void briefusage() const;

    /// Return the entire command-line as one string.
    ///
    std::string command_line() const;

    // Type for a callback that writes something to the output stream.
    typedef std::function<void(const ArgParse& ap, std::ostream&)> callback_t;

    // Set callbacks to run that will print any matter you want as part
    // of the verbose usage, before and after the options are detailed.
    void set_preoption_help(callback_t callback);
    void set_postoption_help(callback_t callback);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;  // PIMPL pattern
};



// Define symbols that let client applications determine if newly added
// features are supported.
#define OIIO_ARGPARSE_SUPPORTS_BRIEFUSAGE 1
#define OIIO_ARGPARSE_SUPPORTS_HUMAN_PARAMNAME 1

OIIO_NAMESPACE_END
