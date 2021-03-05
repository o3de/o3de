// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off



#pragma once

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/ustring.h>


// Define symbols that let client applications determine if newly added
// features are supported.

// Is the close() method present?
#define OIIO_IMAGECACHE_SUPPORTS_CLOSE 1

// Does invalidate() support the optional `force` flag?
#define OIIO_IMAGECACHE_INVALIDATE_FORCE 1



OIIO_NAMESPACE_BEGIN

namespace pvt {
// Forward declaration
class ImageCacheImpl;
class ImageCacheFile;
class ImageCachePerThreadInfo;
};  // namespace pvt



/// Define an API to an abstract class that manages image files,
/// caches of open file handles as well as tiles of pixels so that truly
/// huge amounts of image data may be accessed by an application with low
/// memory footprint.
class OIIO_API ImageCache {
public:
    /// @{
    /// @name Creating and destroying an image cache
    ///
    /// ImageCache is an abstract API described as a pure virtual class.
    /// The actual internal implementation is not exposed through the
    /// external API of OpenImageIO.  Because of this, you cannot construct
    /// or destroy the concrete implementation, so two static methods of
    /// ImageCache are provided:

    /// Create a ImageCache and return a raw pointer to it.  This should
    /// only be freed by passing it to `ImageCache::destroy()`!
    ///
    /// @param  shared
    ///     If `true`, the pointer returned will be a shared ImageCache (so
    ///     that multiple parts of an application that request an ImageCache
    ///     will all end up with the same one). If `shared` is `false`, a
    ///     completely unique ImageCache will be created and returned.
    ///
    /// @returns
    ///     A raw pointer to an ImageCache, which can only be freed with
    ///     `ImageCache::destroy()`.
    ///
    /// @see    ImageCache::destroy
    static ImageCache* create(bool shared = true);

    /// Destroy an allocated ImageCache, including freeing all system
    /// resources that it holds.
    ///
    /// It is safe to destroy even a shared ImageCache, as the implementation
    /// of `destroy()` will recognize a shared one and only truly release
    /// its resources if it has been requested to be destroyed as many times as
    /// shared ImageCache's were created.
    ///
    /// @param  cache
    ///     Raw pointer to the ImageCache to destroy.
    ///
    /// @param  teardown
    ///     For a shared ImageCache, if the `teardown` parameter is
    ///     `true`, it will try to truly destroy the shared cache if
    ///     nobody else is still holding a reference (otherwise, it will
    ///     leave it intact). This parameter has no effect if `cache` was
    ///     not the single globally shared ImageCache.
    static void destroy(ImageCache* cache, bool teardown = false);

    /// @}


    /// @{
    /// @name Setting options and limits for the image cache
    ///
    /// These are the list of attributes that can bet set or queried by
    /// attribute/getattribute:
    ///
    /// - `int max_open_files` :
    ///           The maximum number of file handles that the image cache
    ///           will hold open simultaneously.  (Default = 100)
    /// - `float max_memory_MB` :
    ///           The maximum amount of memory (measured in MB) used for the
    ///           internal "tile cache." (Default: 256.0 MB)
    /// - `string searchpath` :
    ///           The search path for images: a colon-separated list of
    ///           directories that will be searched in order for any image
    ///           filename that is not specified as an absolute path.
    ///           (Default: "")
    /// - `string plugin_searchpath` :
    ///           The search path for plugins: a colon-separated list of
    ///           directories that will be searched in order for any OIIO
    ///           plugins, if not found in OIIO's `lib` directory.
    ///           (Default: "")
    /// - `int autotile` ,
    ///   `int autoscanline` :
    ///           These attributes control how the image cache deals with
    ///           images that are not "tiled" (i.e., are stored as
    ///           scanlines).
    ///
    ///           If `autotile` is set to 0 (the default), an untiled image
    ///           will be treated as if it were a single tile of the
    ///           resolution of the whole image.  This is simple and fast,
    ///           but can lead to poor cache behavior if you are
    ///           simultaneously accessing many large untiled images.
    ///
    ///           If `autotile` is nonzero (e.g., 64 is a good recommended
    ///           value), any untiled images will be read and cached as if
    ///           they were constructed in tiles of size:
    ///
    ///               - `autotile * autotile`
    ///                     if `autoscanline` is 0
    ///               - `width * autotile`
    ///                     if `autoscanline` is nonzero.
    ///
    ///           In both cases, this should lead more efficient caching.
    ///           The `autoscanline` determines whether the "virtual tiles"
    ///           in the cache are square (if `autoscanline` is 0, the
    ///           default) or if they will be as wide as the image (but only
    ///           `autotile` scanlines high).  You should try in your
    ///           application to see which leads to higher performance.
    /// - `int autoscanline` :
    ///           autotile using full width tiles
    /// - `int automip` :
    ///           If 0 (the default), an untiled single-subimage file will
    ///           only be able to utilize that single subimage.
    ///           If nonzero, any untiled, single-subimage (un-MIP-mapped)
    ///           images will have lower-resolution MIP-map levels generated
    ///           on-demand if pixels are requested from the lower-res
    ///           subimages (that don't really exist). Essentially this
    ///           makes the ImageCache pretend that the file is MIP-mapped
    ///           even if it isn't.
    /// - `int accept_untiled` :
    ///           When nonzero, ImageCache accepts untiled images as usual.
    ///           When zero, ImageCache will reject untiled images with an
    ///           error condition, as if the file could not be properly
    ///           read. This is sometimes helpful for applications that want
    ///           to enforce use of tiled images only. (default=1)
    /// - `int accept_unmipped` :
    ///           When nonzero, ImageCache accepts un-MIPmapped images as
    ///           usual.  When set to zero, ImageCache will reject
    ///           un-MIPmapped images with an error condition, as if the
    ///           file could not be properly read. This is sometimes helpful
    ///           for applications that want to enforce use of MIP-mapped
    ///           images only. (Default: 1)
    /// - `int statistics:level` :
    ///           verbosity of statistics auto-printed.
    /// - `int forcefloat` :
    ///           If set to nonzero, all image tiles will be converted to
    ///           `float` type when stored in the image cache.  This can be
    ///           helpful especially for users of ImageBuf who want to
    ///           simplify their image manipulations to only need to
    ///           consider `float` data. The default is zero, meaning that
    ///           image pixels are not forced to be `float` when in cache.
    /// - `int failure_retries` :
    ///           When nonzero (the default), ImageCache accepts
    ///           un-MIPmapped images as usual.  When set to zero,
    ///           ImageCache will reject un-MIPmapped images with an error
    ///           condition, as if the file could not be properly read. This
    ///           is sometimes helpful for applications that want to enforce
    ///           use of MIP-mapped images only. (Default: 1)
    /// - `int deduplicate` :
    ///           When nonzero, the ImageCache will notice duplicate images
    ///           under different names if their headers contain a SHA-1
    ///           fingerprint (as is done with `maketx`-produced textures)
    ///           and handle them more efficiently by avoiding redundant
    ///           reads.  The default is 1 (de-duplication turned on). The
    ///           only reason to set it to 0 is if you specifically want to
    ///           disable the de-duplication optimization.
    /// - `string substitute_image` :
    ///           When set to anything other than the empty string, the
    ///           ImageCache will use the named image in place of *all*
    ///           other images.  This allows you to run an app using OIIO
    ///           and (if you can manage to get this option set)
    ///           automagically substitute a grid, zone plate, or other
    ///           special debugging image for all image/texture use.
    /// - `int unassociatedalpha` :
    ///           When nonzero, will request that image format readers try
    ///           to leave input images with unassociated alpha as they are,
    ///           rather than automatically converting to associated alpha
    ///           upon reading the pixels.  The default is 0, meaning that
    ///           the automatic conversion will take place.
    /// - `int max_errors_per_file` :
    ///           The maximum number of errors that will be printed for each
    ///           file. The default is 100. If your output is cluttered with
    ///           error messages and after the first few for each file you
    ///           aren't getting any helpful additional information, this
    ///           can cut down on the clutter and the runtime. (default:
    ///           100)
    ///
    /// - `string options`
    ///           This catch-all is simply a comma-separated list of
    ///           `name=value` settings of named options, which will be
    ///           parsed and individually set. Example:
    ///
    ///                ic->attribute ("options", "max_memory_MB=512.0,autotile=1");
    ///
    ///           Note that if an option takes a string value that must
    ///           itself contain a comma, it is permissible to enclose the
    ///           value in either single (` ' ' `) or double (` " " `) quotes.
    ///
    /// **Read-only attributes**
    ///
    /// Additionally, there are some read-only attributes that can be
    /// queried with `getattribute()` even though they cannot be set via
    /// `attribute()`:
    ///
    /// - `int total_files` :
    ///           The total number of unique file names referenced by calls
    ///           to the ImageCache.
    ///
    /// - `string[] all_filenames` :
    ///           An array that will be filled with the list of the names of
    ///           all files referenced by calls to the ImageCache. (The
    ///           array is of `ustring` or `char*`.)
    ///
    /// - `int64 stat:cache_memory_used` :
    ///           Total bytes used by tile cache.
    ///
    /// - `int stat:tiles_created` ,
    ///   `int stat:tiles_current` ,
    ///   `int stat:tiles_peak` :
    ///           Total times created, still allocated (at the time of the
    ///           query), and the peak number of tiles in memory at any
    ///           time.
    ///
    /// - `int stat:open_files_created` ,
    ///   `int stat:open_files_current` ,
    ///   `int stat:open_files_peak` :
    ///           Total number of times a file was opened, number still
    ///           opened (at the time of the query), and the peak number of
    ///           files opened at any time.
    ///
    /// - `int stat:find_tile_calls` :
    ///           Number of times a filename was looked up in the file cache.
    ///
    /// - `int64 stat:image_size` :
    ///           Total size (uncompressed bytes of pixel data) of all
    ///           images referenced by the ImageCache. (Note: Prior to 1.7,
    ///           this was called `stat:files_totalsize`.)
    ///
    /// - `int64 stat:file_size` :
    ///           Total size of all files (as on disk, possibly compressed)
    ///           of all images referenced by the ImageCache.
    ///
    /// - `int64 stat:bytes_read` :
    ///           Total size (uncompressed bytes of pixel data) read.
    ///
    /// - `int stat:unique_files` :
    ///           Number of unique files opened.
    ///
    /// - `float stat:fileio_time` :
    ///           Total I/O-related time (seconds).
    ///
    /// - `float stat:fileopen_time` :
    ///           I/O time related to opening and reading headers (but not
    ///           pixel I/O).
    ///
    /// - `float stat:file_locking_time` :
    ///           Total time (across all threads) that threads blocked
    ///           waiting for access to the file data structures.
    ///
    /// - `float stat:tile_locking_time` :
    ///           Total time (across all threads) that threads blocked
    ///           waiting for access to the tile cache data structures.
    ///
    /// - `float stat:find_file_time` :
    ///           Total time (across all threads) that threads spent looking
    ///           up files by name.
    ///
    /// - `float stat:find_tile_time` :
    ///           Total time (across all threads) that threads spent looking
    ///           up individual tiles.
    ///
    /// The following member functions of ImageCache allow you to set (and
    /// in some cases retrieve) options that control the overall behavior of
    /// the image cache:

    /// Set a named attribute (i.e., a property or option) of the
    /// ImageCache.
    ///
    /// Example:
    ///
    ///     ImageCache *ic;
    ///     ...
    ///     int maxfiles = 50;
    ///     ic->attribute ("max_open_files", TypeDesc::INT, &maxfiles);
    ///
    ///     const char *path = "/my/path";
    ///     ic->attribute ("searchpath", TypeDesc::STRING, &path);
    ///
    ///     // There are specialized versions for setting a single int,
    ///     // float, or string without needing types or pointers:
    ///     ic->attribute ("max_open_files", 50);
    ///     ic->attribute ("max_memory_MB", 4000.0f);
    ///     ic->attribute ("searchpath", "/my/path");
    ///
    /// Note: When passing a string, you need to pass a pointer to the
    /// `char*`, not a pointer to the first character.  (Rationale: for an
    /// `int` attribute, you pass the address of the `int`.  So for a
    /// string, which is a `char*`, you need to pass the address of the
    /// string, i.e., a `char**`).
    ///
    /// @param  name    Name of the attribute to set.
    /// @param  type    TypeDesc describing the type of the attribute.
    /// @param  val     Pointer to the value data.
    /// @returns        `true` if the name and type were recognized and the
    ///                 attribute was set, or `false` upon failure
    ///                 (including it being an unrecognized attribute or not
    ///                 of the correct type).
    ///
    virtual bool attribute (string_view name, TypeDesc type,
                            const void *val) = 0;

    /// Specialized `attribute()` for setting a single `int` value.
    virtual bool attribute (string_view name, int val) = 0;
    /// Specialized `attribute()` for setting a single `float` value.
    virtual bool attribute (string_view name, float val) = 0;
    virtual bool attribute (string_view name, double val) = 0;
    /// Specialized `attribute()` for setting a single string value.
    virtual bool attribute (string_view name, string_view val) = 0;

    /// Get the named attribute, store it in `*val`. All of the attributes
    /// that may be set with the `attribute() call` may also be queried with
    /// `getattribute()`.
    ///
    /// Examples:
    ///
    ///     ImageCache *ic;
    ///     ...
    ///     int maxfiles;
    ///     ic->getattribute ("max_open_files", TypeDesc::INT, &maxfiles);
    ///
    ///     const char *path;
    ///     ic->getattribute ("searchpath", TypeDesc::STRING, &path);
    ///
    ///     // There are specialized versions for retrieving a single int,
    ///     // float, or string without needing types or pointers:
    ///     int maxfiles;
    ///     ic->getattribute ("max_open_files", maxfiles);
    ///     const char *path;
    ///     ic->getattribute ("searchpath", &path);
    ///
    /// Note: When retrieving a string, you need to pass a pointer to the
    /// `char*`, not a pointer to the first character. Also, the `char*`
    /// will end up pointing to characters owned by the ImageCache; the
    /// caller does not need to ever free the memory that contains the
    /// characters.
    ///
    /// @param  name    Name of the attribute to retrieve.
    /// @param  type    TypeDesc describing the type of the attribute.
    /// @param  val     Pointer where the attribute value should be stored.
    /// @returns        `true` if the name and type were recognized and the
    ///                 attribute was retrieved, or `false` upon failure
    ///                 (including it being an unrecognized attribute or not
    ///                 of the correct type).
    virtual bool getattribute (string_view name, TypeDesc type,
                               void *val) const = 0;

    /// Specialized `attribute()` for retrieving a single `int` value.
    virtual bool getattribute (string_view name, int &val) const = 0;
    /// Specialized `attribute()` for retrieving a single `float` value.
    virtual bool getattribute (string_view name, float &val) const = 0;
    virtual bool getattribute (string_view name, double &val) const = 0;
    /// Specialized `attribute()` for retrieving a single `string` value
    /// as a `char*`.
    virtual bool getattribute (string_view name, char **val) const = 0;
    /// Specialized `attribute()` for retrieving a single `string` value
    /// as a `std::string`.
    virtual bool getattribute (string_view name, std::string &val) const = 0;

    /// @}


    /// @{
    /// @name Opaque data for performance lookups
    ///
    /// The ImageCache implementation needs to maintain certain per-thread
    /// state, and some methods take an opaque `Perthread` pointer to this
    /// record. There are three options for how to deal with it:
    ///
    /// 1. Don't worry about it at all: don't use the methods that want
    ///    `Perthread` pointers, or always pass `nullptr` for any
    ///    `Perthread*1 arguments, and ImageCache will do
    ///    thread-specific-pointer retrieval as necessary (though at some
    ///    small cost).
    ///
    /// 2. If your app already stores per-thread information of its own, you
    ///    may call `get_perthread_info(nullptr)` to retrieve it for that
    ///    thread, and then pass it into the functions that allow it (thus
    ///    sparing them the need and expense of retrieving the
    ///    thread-specific pointer). However, it is crucial that this
    ///    pointer not be shared between multiple threads. In this case, the
    ///    ImageCache manages the storage, which will automatically be
    ///    released when the thread terminates.
    ///
    /// 3. If your app also wants to manage the storage of the `Perthread`,
    ///    it can explicitly create one with `create_perthread_info()`, pass
    ///    it around, and eventually be responsible for destroying it with
    ///    `destroy_perthread_info()`. When managing the storage, the app
    ///    may reuse the `Perthread` for another thread after the first is
    ///    terminated, but still may not use the same `Perthread` for two
    ///    threads running concurrently.

    /// Define an opaque data type that allows us to have a pointer to
    /// certain per-thread information that the ImageCache maintains. Any
    /// given one of these should NEVER be shared between running threads.
    typedef pvt::ImageCachePerThreadInfo Perthread;

    /// Retrieve a Perthread, unique to the calling thread. This is a
    /// thread-specific pointer that will always return the Perthread for a
    /// thread, which will also be automatically destroyed when the thread
    /// terminates.
    ///
    /// Applications that want to manage their own Perthread pointers (with
    /// `create_thread_info` and `destroy_thread_info`) should still call
    /// this, but passing in their managed pointer. If the passed-in
    /// thread_info is not NULL, it won't create a new one or retrieve a
    /// TSP, but it will do other necessary housekeeping on the Perthread
    /// information.
    virtual Perthread* get_perthread_info(Perthread* thread_info = NULL) = 0;

    /// Create a new Perthread. It is the caller's responsibility to
    /// eventually destroy it using `destroy_thread_info()`.
    virtual Perthread* create_thread_info() = 0;

    /// Destroy a Perthread that was allocated by `create_thread_info()`.
    virtual void destroy_thread_info(Perthread* thread_info) = 0;

    /// Define an opaque data type that allows us to have a handle to an
    /// image (already having its name resolved) but without exposing any
    /// internals.
    typedef pvt::ImageCacheFile ImageHandle;

    /// Retrieve an opaque handle for fast image lookups.  The opaque
    /// `pointer thread_info` is thread-specific information returned by
    /// `get_perthread_info()`.  Return NULL if something has gone horribly
    /// wrong.
    virtual ImageHandle* get_image_handle (ustring filename,
                                            Perthread *thread_info=NULL) = 0;

    /// Return true if the image handle (previously returned by
    /// `get_image_handle()`) is a valid image that can be subsequently read.
    virtual bool good(ImageHandle* file) = 0;

    /// @}


    /// @{
    /// @name   Getting information about images
    ///

    /// Given possibly-relative `filename`, resolve it and use the true path
    /// to the file, with searchpath logic applied.
    virtual std::string resolve_filename(const std::string& filename) const = 0;

    /// Get information or metadata about the named image and store it in
    /// `*data`.
    ///
    /// Data names may include any of the following:
    ///
    /// - `"exists"` : Stores the value 1 (as an `int`) if the file exists and
    ///   is an image format that OpenImageIO can read, or 0 if the file
    ///   does not exist, or could not be properly read as an image. Note
    ///   that unlike all other queries, this query will "succeed" (return
    ///   `true`) even if the file does not exist.
    ///
    /// - `"udim"` : Stores the value 1 (as an `int`) if the file is a
    ///   "virtual UDIM" or texture atlas file (as described in
    ///   :ref:`sec-texturesys-udim`) or 0 otherwise.
    ///
    /// - `"subimages"` : The number of subimages in the file, as an `int`.
    ///
    /// - `"resolution"` : The resolution of the image file, which is an
    ///   array of 2 integers (described as `TypeDesc(INT,2)`).
    ///
    /// - `"miplevels"` : The number of MIPmap levels for the specified
    ///   subimage (an integer).
    ///
    /// - `"texturetype"` : A string describing the type of texture of the
    ///   given file, which describes how the texture may be used (also
    ///   which texture API call is probably the right one for it). This
    ///   currently may return one of: `"unknown"`, `"Plain Texture"`,
    ///   `"Volume Texture"`, `"Shadow"`, or `"Environment"`.
    ///
    /// - `"textureformat"` : A string describing the format of the given
    ///   file, which describes the kind of texture stored in the file. This
    ///   currently may return one of: `"unknown"`, `"Plain Texture"`,
    ///   `"Volume Texture"`, `"Shadow"`, `"CubeFace Shadow"`,
    ///   `"Volume Shadow"`, `"LatLong Environment"`, or
    ///   `"CubeFace Environment"`. Note that there are several kinds of
    ///   shadows and environment maps, all accessible through the same API
    ///   calls.
    ///
    /// - `"channels"` : The number of color channels in the file (an
    ///   `int`).
    ///
    /// - `"format"` : The native data format of the pixels in the file (an
    ///   integer, giving the `TypeDesc::BASETYPE` of the data). Note that
    ///   this is not necessarily the same as the data format stored in the
    ///   image cache.
    ///
    /// - `"cachedformat"` : The native data format of the pixels as stored
    ///   in the image cache (an integer, giving the `TypeDesc::BASETYPE` of
    ///   the data).  Note that this is not necessarily the same as the
    ///   native data format of the file.
    ///
    /// - `"datawindow"` : Returns the pixel data window of the image, which
    ///   is either an array of 4 integers (returning xmin, ymin, xmax,
    ///   ymax) or an array of 6 integers (returning xmin, ymin, zmin, xmax,
    ///   ymax, zmax). The z values may be useful for 3D/volumetric images;
    ///   for 2D images they will be 0).
    ///
    /// - `"displaywindow"` : Returns the display (a.k.a. "full") window of
    ///   the image, which is either an array of 4 integers (returning xmin,
    ///   ymin, xmax, ymax) or an array of 6 integers (returning xmin, ymin,
    ///   zmin, xmax, ymax, zmax). The z values may be useful for
    ///   3D/volumetric images; for 2D images they will be 0).
    ///
    /// - `"worldtocamera"` : The viewing matrix, which is a 4x4 matrix (an
    ///   `Imath::M44f`, described as `TypeDesc(FLOAT,MATRIX)`), giving the
    ///   world-to-camera 3D transformation matrix that was used when  the
    ///   image was created. Generally, only rendered images will have this.
    ///
    /// - `"worldtoscreen"` : The projection matrix, which is a 4x4 matrix
    ///   (an `Imath::M44f`, described as `TypeDesc(FLOAT,MATRIX)`), giving
    ///   the matrix that projected points from world space into a 2D screen
    ///   coordinate system where $x$ and $y$ range from -1 to +1.
    ///   Generally, only rendered images will have this.
    ///
    /// - `"averagecolor"` : If available in the metadata (generally only
    ///   for files that have been processed by `maketx`), this will return
    ///   the average color of the texture (into an array of `float`).
    ///
    /// - `"averagealpha"` : If available in the metadata (generally only
    ///   for files that have been processed by `maketx`), this will return
    ///   the average alpha value of the texture (into a `float`).
    ///
    /// - `"constantcolor"` : If the metadata (generally only for files that
    ///   have been processed by `maketx`) indicates that the texture has
    ///   the same values for all pixels in the texture, this will retrieve
    ///   the constant color of the texture (into an array of floats). A
    ///   non-constant image (or one that does not have the special metadata
    ///   tag identifying it as a constant texture) will fail this query
    ///   (return `false`).
    ///
    /// - `"constantalpha"` : If the metadata indicates that the texture has
    ///   the same values for all pixels in the texture, this will retrieve
    ///   the constant alpha value of the texture (into a float). A
    ///   non-constant image (or one that does not have the special metadata
    ///   tag identifying it as a constant texture) will fail this query
    ///   (return `false`).
    ///
    /// - `"stat:tilesread"` : Number of tiles read from this file
    ///   (`int64`).
    ///
    /// - `"stat:bytesread"` : Number of bytes of uncompressed pixel data
    ///   read from this file (`int64`).
    ///
    /// - `"stat:redundant_tiles"` : Number of times a tile was read, where
    ///   the same tile had been rad before. (`int64`).
    ///
    /// - `"stat:redundant_bytesread"` : Number of bytes (of uncompressed
    ///   pixel data) in tiles that were read redundantly. (`int64`).
    ///
    /// - `"stat:redundant_bytesread"` : Number of tiles read from this file (`int`).
    ///
    /// - `"stat:image_size"` : Size of the uncompressed image pixel data
    /// of this image, in bytes (`int64`).
    ///
    /// - `"stat:file_size"` : Size of the disk file (possibly compressed)
    ///   for this image, in bytes (`int64`).
    ///
    /// - `"stat:timesopened"` : Number of times this file was opened
    ///   (`int`).
    ///
    /// - `"stat:iotime"` : Time (in seconds) spent on all I/O for this file
    ///   (`float`).
    ///
    /// - `"stat:mipsused"` : Stores 1 if any MIP levels beyond the highest
    ///   resolution were accesed, otherwise 0. (`int`)
    ///
    /// - `"stat:is_duplicate"` : Stores 1 if this file was a duplicate of
    ///   another image, otherwise 0. (`int`)
    ///
    /// - *Anything else*  : For all other data names, the the metadata of
    ///   the image file will be searched for an item that matches both the
    ///   name and data type.
    ///
    ///
    ///
    /// @param  filename
    ///             The name of the image.
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to query.
    /// @param  dataname
    ///             The name of the metadata to retrieve.
    /// @param  datatype
    ///             TypeDesc describing the data type.
    /// @param  data
    ///             Pointer to the caller-owned memory where the values
    ///             should be stored. It is the caller's responsibility to
    ///             ensure that `data` points to a large enough storage area
    ///             to accommodate the `datatype` requested.
    ///
    /// @returns
    ///             `true` if `get_image_info()` is able to find the
    ///             requested `dataname` for the image and it matched the
    ///             requested `datatype`.  If the requested data was not
    ///             found or was not of the right data type, return `false`.
    ///             Except for the `"exists"` query, a file that does not
    ///             exist or could not be read properly as an image also
    ///             constitutes a query failure that will return `false`.
    virtual bool get_image_info (ustring filename, int subimage, int miplevel,
                         ustring dataname, TypeDesc datatype, void *data) = 0;
    /// A more efficient variety of `get_image_info()` for cases where you
    /// can use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    virtual bool get_image_info (ImageHandle *file, Perthread *thread_info,
                         int subimage, int miplevel,
                         ustring dataname, TypeDesc datatype, void *data) = 0;

    /// Copy the ImageSpec associated with the named image (the first
    /// subimage & miplevel by default, or as set by `subimage` and
    /// `miplevel`).
    ///
    /// @param  filename
    ///             The name of the image.
    /// @param  spec
    ///             ImageSpec into which will be copied the spec for the
    ///             requested image.
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to query.
    /// @param  native
    ///             If `false` (the default), then the spec retrieved will
    ///             accurately describe the image stored internally in the
    ///             cache, whereas if `native` is `true`, the spec retrieved
    ///             will reflect the contents of the original file.  These
    ///             may differ due to use of certain ImageCache settings
    ///             such as `"forcefloat"` or `"autotile"`.
    /// @returns
    ///             `true` upon success, `false` upon failure failure (such
    ///             as being unable to find, open, or read the file, or if
    ///             it does not contain the designated subimage or MIP
    ///             level).
    virtual bool get_imagespec (ustring filename, ImageSpec &spec,
                                int subimage=0, int miplevel=0,
                                bool native=false) = 0;
    /// A more efficient variety of `get_imagespec()` for cases where you
    /// can use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    virtual bool get_imagespec (ImageHandle *file, Perthread *thread_info,
                                ImageSpec &spec,
                                int subimage=0, int miplevel=0,
                                bool native=false) = 0;

    /// Return a pointer to an ImageSpec associated with the named image
    /// (the first subimage & MIP level by default, or as set by `subimage`
    /// and `miplevel`) if the file is found and is an image format that can
    /// be read, otherwise return `nullptr`.
    ///
    /// This method is much more efficient than `get_imagespec()`, since it
    /// just returns a pointer to the spec held internally by the ImageCache
    /// (rather than copying the spec to the user's memory). However, the
    /// caller must beware that the pointer is only valid as long as nobody
    /// (even other threads) calls `invalidate()` on the file, or
    /// `invalidate_all()`, or destroys the ImageCache.
    ///
    /// @param  filename
    ///             The name of the image.
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to query.
    /// @param  native
    ///             If `false` (the default), then the spec retrieved will
    ///             accurately describe the image stored internally in the
    ///             cache, whereas if `native` is `true`, the spec retrieved
    ///             will reflect the contents of the original file.  These
    ///             may differ due to use of certain ImageCache settings
    ///             such as `"forcefloat"` or `"autotile"`.
    /// @returns
    ///             A pointer to the spec, if the image is found and able to
    ///             be opened and read by an available image format plugin,
    ///             and the designated subimage and MIP level exists.
    virtual const ImageSpec *imagespec (ustring filename, int subimage=0,
                                        int miplevel=0, bool native=false) = 0;
    /// A more efficient variety of `imagespec()` for cases where you can
    /// use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    virtual const ImageSpec *imagespec (ImageHandle *file,
                                        Perthread *thread_info,
                                        int subimage=0, int miplevel=0,
                                        bool native=false) = 0;
    /// @}

    /// @{
    /// @name   Getting Pixels

    /// For an image specified by name, retrieve the rectangle of pixels
    /// from the designated subimage and MIP level, storing the pixel values
    /// beginning at the address specified by `result` and with the given
    /// strides.  The pixel values will be converted to the data type
    /// specified by `format`. The rectangular region to be retrieved
    /// includes `begin` but does not include `end` (much like STL begin/end
    /// usage). Requested pixels that are not part of the valid pixel data
    /// region of the image file will be filled with zero values.
    ///
    /// @param  filename
    ///             The name of the image.
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to retrieve pixels from.
    /// @param  xbegin/xend/ybegin/yend/zbegin/zend
    ///             The range of pixels to retrieve. The pixels retrieved
    ///             include the begin value but not the end value (much like
    ///             STL begin/end usage).
    /// @param  chbegin/chend
    ///             Channel range to retrieve. To retrieve all channels, use
    ///             `chbegin = 0`, `chend = nchannels`.
    /// @param  format
    ///             TypeDesc describing the data type of the values you want
    ///             to retrieve into `result`. The pixel values will be
    ///             converted to this type regardless of how they were
    ///             stored in the file.
    /// @param  result
    ///             Pointer to the memory where the pixel values should be
    ///             stored.  It is up to the caller to ensure that `result`
    ///             points to an area of memory big enough to accommodate
    ///             the requested rectangle (taking into consideration its
    ///             dimensions, number of channels, and data format).
    /// @param  xstride/ystride/zstride
    ///             The number of bytes between the beginning of successive
    ///             pixels, scanlines, and image planes, respectively. Any
    ///             stride values set to `AutoStride` will be assumed to
    ///             indicate a contiguous data layout in that dimension.
    /// @param  cache_chbegin/cache_chend These parameters can be used to
    ///             tell the ImageCache to read and cache a subset of
    ///             channels (if not specified or if they denote a
    ///             non-positive range, all the channels of the file will be
    ///             stored in the cached tile).
    ///
    /// @returns
    ///             `true` for success, `false` for failure.
    virtual bool get_pixels (ustring filename,
                    int subimage, int miplevel, int xbegin, int xend,
                    int ybegin, int yend, int zbegin, int zend,
                    int chbegin, int chend, TypeDesc format, void *result,
                    stride_t xstride=AutoStride, stride_t ystride=AutoStride,
                    stride_t zstride=AutoStride,
                    int cache_chbegin = 0, int cache_chend = -1) = 0;
    /// A more efficient variety of `get_pixels()` for cases where you can
    /// use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    virtual bool get_pixels (ImageHandle *file, Perthread *thread_info,
                    int subimage, int miplevel, int xbegin, int xend,
                    int ybegin, int yend, int zbegin, int zend,
                    int chbegin, int chend, TypeDesc format, void *result,
                    stride_t xstride=AutoStride, stride_t ystride=AutoStride,
                    stride_t zstride=AutoStride,
                    int cache_chbegin = 0, int cache_chend = -1) = 0;

    /// A simplified `get_pixels()` where all channels are retrieved,
    /// strides are assumed to be contiguous.
    virtual bool get_pixels (ustring filename, int subimage, int miplevel,
                             int xbegin, int xend, int ybegin, int yend,
                             int zbegin, int zend,
                             TypeDesc format, void *result) = 0;
    /// A more efficient variety of `get_pixels()` for cases where you can
    /// use an `ImageHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    virtual bool get_pixels (ImageHandle *file, Perthread *thread_info,
                             int subimage, int miplevel,
                             int xbegin, int xend, int ybegin, int yend,
                             int zbegin, int zend,
                             TypeDesc format, void *result) = 0;
    /// @}

    /// @{
    /// @name Controlling the cache
    ///

    /// Invalidate any loaded tiles or open file handles associated with the
    /// filename, so that any subsequent queries will be forced to re-open
    /// the file or re-load any tiles (even those that were previously
    /// loaded and would ordinarily be reused).  A client might do this if,
    /// for example, they are aware that an image being held in the cache
    /// has been updated on disk.  This is safe to do even if other
    /// procedures are currently holding reference-counted tile pointers
    /// from the named image, but those procedures will not get updated
    /// pixels until they release the tiles they are holding.
    ///
    /// If `force` is true, this invalidation will happen unconditionally;
    /// if false, the file will only be invalidated if it has been changed
    /// since it was first opened by the ImageCache.
    virtual void invalidate(ustring filename, bool force = true) = 0;

    /// Invalidate all loaded tiles and close open file handles.  This is
    /// safe to do even if other procedures are currently holding
    /// reference-counted tile pointers from the named image, but those
    /// procedures will not get updated pixels (if the images change) until
    /// they release the tiles they are holding.
    ///
    /// If `force` is true, everything will be invalidated, no matter how
    /// wasteful it is, but if `force` is false, in actuality files will
    /// only be invalidated if their modification times have been changed
    /// since they were first opened.
    virtual void invalidate_all(bool force = false) = 0;

    /// Close any open file handles associated with a named file, but do not
    /// invalidate any image spec information or pixels associated with the
    /// files.  A client might do this in order to release OS file handle
    /// resources, or to make it safe for other processes to modify image
    /// files on disk.
    virtual void close (ustring filename) = 0;

    /// `close()` all files known to the cache.
    virtual void close_all () = 0;

    /// An opaque data type that allows us to have a pointer to a tile but
    /// without exposing any internals.
    class Tile;

    /// Find the tile specified by an image filename, subimage & miplevel,
    /// the coordinates of a pixel, and optionally a channel range.   An
    /// opaque pointer to the tile will be returned, or `nullptr` if no such
    /// file (or tile within the file) exists or can be read.  The tile will
    /// not be purged from the cache until after `release_tile()` is called
    /// on the tile pointer the same number of times that `get_tile()` was
    /// called (reference counting). This is thread-safe! If `chend <
    /// chbegin`, it will retrieve a tile containing all channels in the
    /// file.
    virtual Tile * get_tile (ustring filename, int subimage, int miplevel,
                             int x, int y, int z,
                             int chbegin = 0, int chend = -1) = 0;
    /// A slightly more efficient variety of `get_tile()` for cases where
    /// you can use an `ImageHandle*` to specify the image and optionally
    /// have a `Perthread*` for the calling thread.
    ///
    /// @see `get_pixels()`
    virtual Tile * get_tile (ImageHandle *file, Perthread *thread_info,
                             int subimage, int miplevel,
                             int x, int y, int z,
                             int chbegin = 0, int chend = -1) = 0;

    /// After finishing with a tile, release_tile will allow it to
    /// once again be purged from the tile cache if required.
    virtual void release_tile(Tile* tile) const = 0;

    /// Retrieve the data type of the pixels stored in the tile, which may
    /// be different than the type of the pixels in the disk file.
    virtual TypeDesc tile_format(const Tile* tile) const = 0;

    /// Retrieve the ROI describing the pixels and channels stored in the
    /// tile.
    virtual ROI tile_roi(const Tile* tile) const = 0;

    /// For a tile retrived by `get_tile()`, return a pointer to the pixel
    /// data itself, and also store in `format` the data type that the
    /// pixels are internally stored in (which may be different than the
    /// data type of the pixels in the disk file).   This method should only
    /// be called on a tile that has been requested by `get_tile()` but has
    /// not yet been released with `release_tile()`.
    virtual const void* tile_pixels(Tile* tile, TypeDesc& format) const = 0;

    /// The add_file() call causes a file to be opened or added to the
    /// cache. There is no reason to use this method unless you are
    /// supplying a custom creator, or configuration, or both.
    ///
    /// If creator is not NULL, it points to an ImageInput::Creator that
    /// will be used rather than the default ImageInput::create(), thus
    /// instead of reading from disk, creates and uses a custom ImageInput
    /// to generate the image. The 'creator' is a factory that creates the
    /// custom ImageInput and will be called like this:
    ///
    ///      std::unique_ptr<ImageInput> in (creator());
    ///
    /// Once created, the ImageCache owns the ImageInput and is responsible
    /// for destroying it when done. Custom ImageInputs allow "procedural"
    /// images, among other things.  Also, this is the method you use to set
    /// up a "writeable" ImageCache images (perhaps with a type of
    /// ImageInput that's just a stub that does as little as possible).
    ///
    /// If `config` is not NULL, it points to an ImageSpec with configuration
    /// options/hints that will be passed to the underlying
    /// ImageInput::open() call. Thus, this can be used to ensure that the
    /// ImageCache opens a call with special configuration options.
    ///
    /// This call (including any custom creator or configuration hints) will
    /// have no effect if there's already an image by the same name in the
    /// cache. Custom creators or configurations only "work" the FIRST time
    /// a particular filename is referenced in the lifetime of the
    /// ImageCache. But if replace is true, any existing entry will be
    /// invalidated, closed and overwritten. So any subsequent access will
    /// see the new file. Existing texture handles will still be valid.
    virtual bool add_file (ustring filename, ImageInput::Creator creator=nullptr,
                           const ImageSpec *config=nullptr,
                           bool replace = false) = 0;

    /// Preemptively add a tile corresponding to the named image, at the
    /// given subimage, MIP level, and channel range.  The tile added is the
    /// one whose corner is (x,y,z), and buffer points to the pixels (in the
    /// given format, with supplied strides) which will be copied and
    /// inserted into the cache and made available for future lookups.
    /// If chend < chbegin, it will add a tile containing the full set of
    /// channels for the image. Note that if the 'copy' flag is false, the
    /// data is assumed to be in some kind of persistent storage and will
    /// not be copied, nor will its pixels take up additional memory in the
    /// cache.
    virtual bool add_tile (ustring filename, int subimage, int miplevel,
                     int x, int y, int z, int chbegin, int chend,
                     TypeDesc format, const void *buffer,
                     stride_t xstride=AutoStride, stride_t ystride=AutoStride,
                     stride_t zstride=AutoStride, bool copy = true) = 0;

    /// @}

    /// @{
    /// @name Errors and statistics

    /// If any of the API routines returned `false` indicating an error,
    /// this routine will return the error string (and clear any error
    /// flags).  If no error has occurred since the last time `geterror()`
    /// was called, it will return an empty string.
    virtual std::string geterror() const = 0;

    /// Returns a big string containing useful statistics about the
    /// ImageCache operations, suitable for saving to a file or outputting
    /// to the terminal. The `level` indicates the amount of detail in
    /// the statistics, with higher numbers (up to a maximum of 5) yielding
    /// more and more esoteric information.
    virtual std::string getstats(int level = 1) const = 0;

    /// Reset most statistics to be as they were with a fresh ImageCache.
    /// Caveat emptor: this does not flush the cache itelf, so the resulting
    /// statistics from the next set of texture requests will not match the
    /// number of tile reads, etc., that would have resulted from a new
    /// ImageCache.
    virtual void reset_stats() = 0;

    /// @}

    virtual ~ImageCache() {}

protected:
    // User code should never directly construct or destruct an ImageCache.
    // Always use ImageCache::create() and ImageCache::destroy().
    ImageCache(void) {}
private:
    // Make delete private and unimplemented in order to prevent apps
    // from calling it.  Instead, they should call ImageCache::destroy().
    void operator delete(void* /*todel*/) {}
};


OIIO_NAMESPACE_END
