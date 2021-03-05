// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#ifndef OPENIMAGEIO_VERSION_H
#define OPENIMAGEIO_VERSION_H


// Versioning of the OpenImageIO software
#define OIIO_VERSION_MAJOR 2
#define OIIO_VERSION_MINOR 1
#define OIIO_VERSION_PATCH 16
#define OIIO_VERSION_TWEAK 0
#define OIIO_VERSION_RELEASE_TYPE 

#define OIIO_VERSION (10000 * OIIO_VERSION_MAJOR + \
                        100 * OIIO_VERSION_MINOR + \
                              OIIO_VERSION_PATCH)
// We also define the old name for backwards compatibility purposes.
#define OPENIMAGEIO_VERSION OIIO_VERSION

// Magic macros to make OIIO_VERSION_STRING that looks like "1.2.3"
#define OIIO_MAKE_VERSION_STRING2(a,b,c,d) #a "." #b "." #c #d
#define OIIO_MAKE_VERSION_STRING(a,b,c,d) OIIO_MAKE_VERSION_STRING2(a,b,c,d)
#define OIIO_VERSION_STRING \
    OIIO_MAKE_VERSION_STRING(OIIO_VERSION_MAJOR, \
                             OIIO_VERSION_MINOR, OIIO_VERSION_PATCH, \
                             OIIO_VERSION_RELEASE_TYPE)
#define OIIO_INTRO_STRING "OpenImageIO " OIIO_VERSION_STRING " http://www.openimageio.org"


// Establish the name spaces
namespace OpenImageIO_v2_1 { }
namespace OIIO = OpenImageIO_v2_1;

// Macros to use in each file to enter and exit the right name spaces.
#define OIIO_NAMESPACE OpenImageIO_v2_1
#define OIIO_NAMESPACE_STRING "OpenImageIO_v2_1"
#define OIIO_NAMESPACE_BEGIN namespace OpenImageIO_v2_1 {
#define OIIO_NAMESPACE_END }
#define OIIO_NAMESPACE_USING using namespace OIIO;


/// Each imageio DSO/DLL should include this statement:
///      DLLPUBLIC int FORMAT_imageio_version = OPENIMAGEIO_PLUGIN_VERSION;
/// libOpenImageIO will check for compatibility this way.
/// This should get bumped any time we change the API in any way that
/// will make previously-compiled plugins break.
///
/// History:
/// Version 3 added supports_rectangles() and write_rectangle() to
/// ImageOutput, and added stride parameters to the ImageInput read
/// routines.
/// Version 10 represents forking from NVIDIA's open source version,
/// with which we break backwards compatibility.
/// Version 11 teased apart subimage versus miplevel specification in
/// the APIs and per-channel formats (introduced in OIIO 0.9).
/// Version 12 added read_scanlines(), write_scanlines(), read_tiles(),
///     write_tiles(), and ImageInput::supports(). (OIIO 1.0)
/// Version 13 added ImageInput::valid_file().  (OIIO 1.1)
/// Version 14 added ImageOutput::open() variety for multiple subimages.
/// Version 15 added support for "deep" images (changing ImageSpec,
///     ImageInput, ImageOutput).
/// Version 16 changed the ImageInput functions taking channel ranges
///     from firstchan,nchans to chbegin,chend.
/// Version 17 changed to int supports(string_view) rather than
///     bool supports(const std::string&)). (OIIO 1.6)
/// Version 18 changed to add an m_threads member to ImageInput/Output.
/// Version 19 changed the definition of DeepData.
/// Version 20 added FMT_imageio_library_version() to plugins. (OIIO 1.7)
/// Version 21 changed the signatures of ImageInput methods: added
///     subimage,miplevel params to many read_*() methods; changed thread
///     safety expectations; removed newspec param from seek_subimage;
///     added spec(subimage,miplevel) and spec_dimensions(subimage,miplevel).
///     (OIIO 1.9)
/// Version 22 changed the signatures of ImageInput/ImageOutput create()
///     to return unique_ptr. (OIIO 1.9)

#define OIIO_PLUGIN_VERSION 22

#define OIIO_PLUGIN_NAMESPACE_BEGIN OIIO_NAMESPACE_BEGIN
#define OIIO_PLUGIN_NAMESPACE_END OIIO_NAMESPACE_END

#ifdef EMBED_PLUGINS
#define OIIO_PLUGIN_EXPORTS_BEGIN
#define OIIO_PLUGIN_EXPORTS_END
#else
#define OIIO_PLUGIN_EXPORTS_BEGIN extern "C" {
#define OIIO_PLUGIN_EXPORTS_END }
#endif

// Which CPP standard (11, 14, etc.) was this copy of OIIO *built* with?
#define OIIO_BUILD_CPP 11

// DEPRECATED(2.1): old macros separately giving compatibility.
#define OIIO_BUILD_CPP11 (11 >= 11)
#define OIIO_BUILD_CPP14 (11 >= 14)
#define OIIO_BUILD_CPP17 (11 >= 17)
#define OIIO_BUILD_CPP20 (11 >= 20)

#endif

