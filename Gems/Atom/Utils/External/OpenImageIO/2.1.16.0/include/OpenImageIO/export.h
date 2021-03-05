// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once


/// \file
/// Macros necessary for proper symbol export from dynamic libraries.


///
/// On Windows, when compiling code that will end up in a DLL, symbols
/// must be marked as 'exported' (i.e. __declspec(dllexport)) or they
/// won't be visible to programs linking against the DLL.
///
/// In addition, when compiling the application code that calls the DLL,
/// if a routine is marked as 'imported' (i.e. __declspec(dllimport)),
/// the compiler can be smart about eliminating a level of calling
/// indirection.  But you DON'T want to use __declspec(dllimport) when
/// calling a function from within its own DLL (it will still compile
/// correctly, just not with maximal efficiency).  Which is quite the
/// dilemma since the same header file is used by both the library and
/// its clients.  Sheesh!
///
/// But on Linux/OSX as well, we want to only have the DSO export the
/// symbols we designate as the public interface.  So we link with
/// -fvisibility=hidden to default to hiding the symbols.  See
/// http://gcc.gnu.org/wiki/Visibility
///
/// We solve this awful mess by defining these macros:
///
/// OIIO_API - used for the OpenImageIO public API.  Normally, assumes
///               that it's being seen by a client of the library, and
///               therefore declare as 'imported'.  But if
///               OpenImageIO_EXPORT is defined (as is done by CMake
///               when compiling the library itself), change the
///               declaration to 'exported'.
/// OIIO_EXPORT - explicitly exports a symbol that isn't part of the
///               public API but still needs to be visible.
/// OIIO_LOCAL -  explicitly hides a symbol that might otherwise be
///               exported
///
///

#if defined(_WIN32) || defined(__CYGWIN__)
#    ifdef OIIO_STATIC_DEFINE
#        define OIIO_IMPORT
#        define OIIO_EXPORT
#    else
#        define OIIO_IMPORT __declspec(dllimport)
#        define OIIO_EXPORT __declspec(dllexport)
#    endif
#    define OIIO_LOCAL
#else
#    define OIIO_IMPORT __attribute__((visibility("default")))
#    define OIIO_EXPORT __attribute__((visibility("default")))
#    define OIIO_LOCAL __attribute__((visibility("hidden")))
#endif

#if defined(OpenImageIO_EXPORTS) || defined(OpenImageIO_Util_EXPORTS)
#    define OIIO_API OIIO_EXPORT
#else
#    define OIIO_API OIIO_IMPORT
#endif
