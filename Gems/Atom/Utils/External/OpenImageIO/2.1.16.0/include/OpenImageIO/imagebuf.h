// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once
#define OPENIMAGEIO_IMAGEBUF_H

#if defined(_MSC_VER)
// Ignore warnings about DLL exported classes with member variables that are template classes.
// This happens with the std::vector and std::string protected members of ImageBuf below.
#    pragma warning(disable : 4251)
#endif

#include <OpenImageIO/dassert.h>
#include <OpenImageIO/fmath.h>
#include <OpenImageIO/imagecache.h>
#include <OpenImageIO/imageio.h>

#include <limits>
#include <memory>


OIIO_NAMESPACE_BEGIN

class ImageBuf;
class ImageBufImpl;  // Opaque type for the unique_ptr.



/// Return pixel data window for this ImageSpec as a ROI.
OIIO_API ROI
get_roi(const ImageSpec& spec);

/// Return full/display window for this ImageSpec as a ROI.
OIIO_API ROI
get_roi_full(const ImageSpec& spec);

/// Set pixel data window for this ImageSpec to a ROI.
/// Does NOT change the channels of the spec, regardless of newroi.
OIIO_API void
set_roi(ImageSpec& spec, const ROI& newroi);

/// Set full/display window for this ImageSpec to a ROI.
/// Does NOT change the channels of the spec, regardless of newroi.
OIIO_API void
set_roi_full(ImageSpec& spec, const ROI& newroi);


enum class InitializePixels { No = 0, Yes = 1 };



/// An ImageBuf is a simple in-memory representation of a 2D image.  It uses
/// ImageInput and ImageOutput underneath for its file I/O, and has simple
/// routines for setting and getting individual pixels, that hides most of
/// the details of memory layout and data representation (translating
/// to/from float automatically).
class OIIO_API ImageBuf {
public:
    /// An ImageBuf can store its pixels in one of several ways (each
    /// identified by an `IBStorage` enumerated value):
    enum IBStorage {
        // clang-format off
        UNINITIALIZED,
            ///< An ImageBuf that doesn't represent any image at all
            /// (either because it is newly constructed with the default
            /// constructor, or had an error during construction).
        LOCALBUFFER,
            ///< "Local storage" is allocated to hold the image pixels
            /// internal to the ImageBuf.  This memory will be freed when
            /// the ImageBuf is destroyed.
        APPBUFFER,
            ///< The ImageBuf "wraps" pixel memory already allocated and
            /// owned by the calling application. The caller will continue
            /// to own that memory and be responsible for freeing it after
            /// the ImageBuf is destroyed.
        IMAGECACHE
            ///< The ImageBuf is "backed" by an ImageCache, which will
            /// automatically be used to retrieve pixels when requested, but
            /// the ImageBuf will not allocate separate storage for it. 
            /// This brings all the advantages of the ImageCache, but can
            /// only be used for read-only ImageBuf's that reference a
            /// stored image file.
        // clang-format on
    };

    /// @{
    /// @name Constructing and destructing an ImageBuf.

    /// Default constructor makes an empty/uninitialized ImageBuf.  There
    /// isn't much you can do with an uninitialized buffer until you call
    /// `reset()`. The storage type of a default-constructed ImageBuf is
    /// `IBStorage::UNINITIALIZED`.
    ImageBuf();

    /// Destructor for an ImageBuf.
    ~ImageBuf();

    /// Construct a read-only ImageBuf that will be used to read the named
    /// file (at the given subimage and MIP-level, defaulting to the first
    /// in the file).  But don't read it yet!  The image will actually be
    /// read lazily, only when other methods need to access the spec and/or
    /// pixels, or when an explicit call to `init_spec()` or `read()` is
    /// made, whichever comes first.
    ///
    /// The implementation may end up either reading the entire image
    /// internally owned memory (if so, the storage will be `LOCALBUFFER`),
    /// or it may rely on being backed by an ImageCache (in this case, the
    /// storage will be `IMAGECACHE`) -- depending on the image size and
    /// other factors.
    ///
    /// @param name
    ///             The image to read.
    /// @param subimage/miplevel
    ///             The subimage and MIP level to read (defaults to the
    ///             first subimage of the file, highest-res MIP level).
    /// @param imagecache
    ///             Optionally, a particular ImageCache to use. If nullptr,
    ///             the default global/shared image cache will be used.
    /// @param config
    ///             Optionally, a pointer to an ImageSpec whose metadata
    ///             contains configuration hints that set options related
    ///             to the opening and reading of the file.
    ///
    explicit ImageBuf(string_view name, int subimage = 0, int miplevel = 0,
                      ImageCache* imagecache  = nullptr,
                      const ImageSpec* config = nullptr);

    // Deprecated synonym for `ImageBuf(name, 0, 0, imagecache, nullptr)`.
    ImageBuf(string_view name, ImageCache* imagecache);

    /// Construct a writable ImageBuf with the given specification
    /// (including resolution, data type, metadata, etc.). The ImageBuf will
    /// allocate and own its own pixel memory and will free that memory
    /// automatically upon destruction, clear(), or reset(). Upon successful
    /// initialization, the storage will be reported as `LOCALBUFFER`.
    ///
    /// @param spec
    ///             An ImageSpec describing the image and its metadata. If
    ///             not enough information is given to know how much memory
    ///             to allocate (width, height, depth, channels, and data
    ///             format), the ImageBuf will remain in an UNINITIALIZED
    ///             state and will have no local pixel storage.
    /// @param zero
    ///             After a successful allocation of the local pixel
    ///             storage, this parameter controls whether the pixels
    ///             will be initialized to hold zero (black) values
    ///             (`InitializePixels::Yes`) or if the pixel memory will
    ///             remain uninitialized (`InitializePixels::No`) and thus
    ///             may hold nonsensical values. Choosing `No` may save the
    ///             time of writing to the pixel memory if you know for sure
    ///             that you are about to overwrite it completely before you
    ///             will need to read any pixel values.
    ///
    explicit ImageBuf(const ImageSpec& spec,
                      InitializePixels zero = InitializePixels::Yes);

    // Deprecated/useless synonym for `ImageBuf(spec,zero)` but also gives
    // it an internal name.
    ImageBuf(string_view name, const ImageSpec& spec,
             InitializePixels zero = InitializePixels::Yes);

    /// Construct a writable ImageBuf that "wraps" existing pixel memory
    /// owned by the calling application. The ImageBuf does not own the
    /// pixel storage and will will not free/delete that memory, even when
    /// the ImageBuf is destroyed. Upon successful initialization, the
    /// storage will be reported as `APPBUFFER`.
    ///
    /// @param spec
    ///             An ImageSpec describing the image and its metadata. If
    ///             not enough information is given to know the "shape" of
    ///             the image (width, height, depth, channels, and data
    ///             format), the ImageBuf will remain in an UNINITIALIZED
    ///             state.
    /// @param buffer
    ///             A pointer to the caller-owned memory containing the
    ///             storage for the pixels. It must be already allocated
    ///             with enough space to hold a full image as described by
    ///             `spec`.
    ///
    ImageBuf(const ImageSpec& spec, void* buffer);

    // Deprecated/useless synonym for `ImageBuf(spec,buffer)` but also gives
    // it an internal name.
    ImageBuf(string_view name, const ImageSpec& spec, void* buffer);

    /// Construct a copy of an ImageBuf.
    ImageBuf(const ImageBuf& src);

    /// Move the contents of an ImageBuf to another ImageBuf.
    ImageBuf(ImageBuf&& src);

    // Old name for reset().
    void clear();

    /// Destroy any previous contents of the ImageBuf and re-initialize it
    /// to resemble a freshly constructed ImageBuf using the default
    /// constructor (holding no image, with storage
    /// `IBStorage::UNINITIALIZED`).
    void reset() { clear(); }

    // Deprecated/useless synonym for `reset(name, 0, 0, imagecache, nullptr)`
    void reset(string_view name, ImageCache* imagecache = nullptr);

    /// Destroy any previous contents of the ImageBuf and re-initialize it
    /// as if newly constructed with the same arguments, as a read-only
    /// representation of an existing image file.
    void reset(string_view name, int subimage, int miplevel,
               ImageCache* imagecache  = nullptr,
               const ImageSpec* config = nullptr);

    /// Destroy any previous contents of the ImageBuf and re-initialize it
    /// as if newly constructed with the same arguments, as a read/write
    /// image with locally allocated storage that can hold an image as
    /// described by `spec`. The optional `zero` parameter controls whether
    /// the pixel values are filled with black/empty, or are left
    /// uninitialized after being allocated.
    ///
    /// Note that if the ImageSpec does not contain enough information to
    /// specify how much memory to allocate (width, height, channels, and
    /// data format), the ImageBuf will remain uninitialized (regardless of
    /// how `zero` is set).
    void reset(const ImageSpec& spec,
               InitializePixels zero = InitializePixels::Yes);

    // Deprecated/useless synonym for `reset(spec, spec, zero)` and also
    // give it an internal name.
    void reset(string_view name, const ImageSpec& spec,
               InitializePixels zero = InitializePixels::Yes);

    /// Destroy any previous contents of the ImageBuf and re-initialize it
    /// as if newly constructed with the same arguments, to "wrap" existing
    /// pixel memory owned by the calling application.
    void reset(const ImageSpec& spec, void* buffer);

    /// Make the ImageBuf be writable. That means that if it was previously
    /// backed by an ImageCache (storage was `IMAGECACHE`), it will force a
    /// full read so that the whole image is in local memory. This will
    /// invalidate any current iterators on the image. It has no effect if
    /// the image storage is not `IMAGECACHE`.
    ///
    /// @param keep_cache_type
    ///             If true, preserve any ImageCache-forced data types (you
    ///             might want to do this if it is critical that the
    ///             apparent data type doesn't change, for example if you
    ///             are calling `make_writeable()` from within a
    ///             type-specialized function).
    /// @returns
    ///             Return `true` if it works (including if no read was
    ///             necessary), `false` if something went horribly wrong.
    bool make_writeable(bool keep_cache_type = false);

    /// @}


    /// Wrap mode describes what happens when an iterator points to
    /// a value outside the usual data range of an image.
    enum WrapMode {
        WrapDefault,
        WrapBlack,
        WrapClamp,
        WrapPeriodic,
        WrapMirror,
        _WrapLast
    };


    /// @{
    /// @name  Reading and Writing disk images

    /// Read the particular subimage and MIP level of the image.  Generally,
    /// this will skip the expensive read if the file has already been read
    /// into the ImageBuf (at the specified subimage and MIP level).  It
    /// will clear and re-allocate memory if the previously allocated space
    /// was not appropriate for the size or data type of the image being
    /// read.
    ///
    /// In general, `read()` will try not to do any I/O at the time of the
    /// `read()` call, but rather to have the ImageBuf "backed" by an
    /// ImageCache, which will do the file I/O on demand, as pixel values
    /// are needed, and in that case the ImageBuf doesn't actually allocate
    /// memory for the pixels (the data lives in the ImageCache).  However,
    /// there are several conditions for which the ImageCache will be
    /// bypassed, the ImageBuf will allocate "local" memory, and the disk
    /// file will be read directly into allocated buffer at the time of the
    /// `read()` call: (a) if the `force` parameter is `true`; (b) if the
    /// `convert` parameter requests a data format conversion to a type that
    /// is not the native file type and also is not one of the internal
    /// types supported by the ImageCache (specifically, `float` and
    /// `UINT8`); (c) if the ImageBuf already has local pixel memory
    /// allocated, or "wraps" an application buffer.
    ///
    /// Note that `read()` is not strictly necessary. If you are happy with
    /// the filename, subimage and MIP level specified by the ImageBuf
    /// constructor (or the last call to `reset()`), and you want the
    /// storage to be backed by the ImageCache (including storing the
    /// pixels in whatever data format that implies), then the file contents
    /// will be automatically read the first time you make any other
    /// ImageBuf API call that requires the spec or pixel values.  The only
    /// reason to call `read()` yourself is if you are changing the
    /// filename, subimage, or MIP level, or if you want to use `force =
    /// true` or a specific `convert` value to force data format conversion.
    ///
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to read.
    /// @param  force
    ///             If `true`, will force an immediate full read into
    ///             ImageBuf-owned local pixel memory (yielding a
    ///             `LOCALPIXELS` storage buffer). Otherwise, it is up to
    ///             the implementation whether to immediately read or have
    ///             the image backed by an ImageCache (storage
    ///             `IMAGECACHE`.)
    /// @param  convert
    ///             If set to a specific type (not`UNKNOWN`), the ImageBuf
    ///             memory will be allocated for that type specifically and
    ///             converted upon read.
    /// @param  progress_callback/progress_callback_data
    ///             If `progress_callback` is non-NULL, the underlying
    ///             read, if expensive, may make several calls to
    ///                 `progress_callback(progress_callback_data, portion_done)`
    ///             which allows you to implement some sort of progress
    ///             meter. Note that if the ImageBuf is backed by an
    ///             ImageCache, the progress callback will never be called,
    ///             since no actual file I/O will occur at this time
    ///             (ImageCache will load tiles or scanlines on demand, as
    ///             individual pixel values are needed).
    ///
    /// @returns
    ///             `true` upon success, or `false` if the read failed (in
    ///             which case, you should be able to retrieve an error
    ///             message via `geterror()`).
    ///
    bool read(int subimage = 0, int miplevel = 0, bool force = false,
              TypeDesc convert                   = TypeDesc::UNKNOWN,
              ProgressCallback progress_callback = nullptr,
              void* progress_callback_data       = nullptr);

    /// Read the file, if possible only allocating and reading a subset of
    /// channels, `[chbegin..chend-1]`. This can be a performance and memory
    /// improvement for some image file formats, if you know that any use of
    /// the ImageBuf will only access a subset of channels from a
    /// many-channel file.
    ///
    /// Additional parameters:
    ///
    /// @param  chbegin/chend
    ///             The subset (a range with "exclusive end") of channels to
    ///             read, if the implementation is able to read only a
    ///             subset of channels and have a performance advantage by
    ///             doing so. If `chbegin` is 0 and `chend` is either
    ///             negative or greater than the number of channels in the
    ///             file, all channels will be read. Please note that it is
    ///             "advisory" and not guaranteed to be honored by the
    ///             underlying implementation.
    bool read(int subimage, int miplevel, int chbegin, int chend, bool force,
              TypeDesc convert, ProgressCallback progress_callback = nullptr,
              void* progress_callback_data = nullptr);

    /// Read the ImageSpec for the given file, subimage, and MIP level into
    /// the ImageBuf, but will not read the pixels or allocate any local
    /// storage (until a subsequent call to `read()`).  This is helpful if
    /// you have an ImageBuf and you need to know information about the
    /// image, but don't want to do a full read yet, and maybe won't need to
    /// do the full read, depending on what's found in the spec.
    ///
    /// Note that `init_spec()` is not strictly necessary. If you are happy
    /// with the filename, subimage and MIP level specified by the ImageBuf
    /// constructor (or the last call to `reset()`), then the spec will be
    /// automatically read the first time you make any other ImageBuf API
    /// call that requires it. The only reason to call `read()` yourself is
    /// if you are changing the filename, subimage, or MIP level, or if you
    /// want to use `force=true` or a specific `convert` value to force
    /// data format conversion.
    ///
    /// @param  filename
    ///             The filename to read from (should be the same as the
    ///             filename used when the ImageBuf was constructed or
    ///             reset.)
    /// @param  subimage/miplevel
    ///             The subimage and MIP level to read.
    ///
    /// @returns
    ///             `true` upon success, or `false` if the read failed (in
    ///             which case, you should be able to retrieve an error
    ///             message via `geterror()`).
    ///
    bool init_spec(string_view filename, int subimage, int miplevel);

    /// Write the image to the named file, converted to the specified pixel
    /// data type `dtype` (`TypeUnknown` signifies to use the data type of
    /// the buffer), and file format (an empty `fileformat` means to infer
    /// the type from the filename extension).
    ///
    /// By default, it will always try to write a scanline-oriented file,
    /// unless the `set_write_tiles()` method has been used to override
    /// this.
    ///
    /// @param  filename
    ///             The filename to write to.
    /// @param  dtype
    ///             Optional override of the pixel data format to use in the
    ///             file being written. The default (`UNKNOWN`) means to try
    ///             writing the same data format that as pixels are stored
    ///             within the ImageBuf memory (or whatever type was
    ///             specified by a prior call to `set_write_format()`). In
    ///             either case, if the file format does not support that
    ///             data type, another will be automatically chosen that is
    ///             supported by the file type and loses as little precision
    ///             as possible.
    /// @param  fileformat
    ///             Optional override of the file format to write. The
    ///             default (empty string) means to infer the file format
    ///             from the extension of the `filename` (for
    ///             example, "foo.tif" will write a TIFF file).
    /// @param  progress_callback/progress_callback_data
    ///             If `progress_callback` is non-NULL, the underlying
    ///             write, if expensive, may make several calls to
    ///                 `progress_callback(progress_callback_data, portion_done)`
    ///             which allows you to implement some sort of progress
    ///             meter.
    ///
    /// @returns
    ///             `true` upon success, or `false` if the write failed (in
    ///             which case, you should be able to retrieve an error
    ///             message via `geterror()`).
    ///

    bool write(string_view filename, TypeDesc dtype = TypeUnknown,
               string_view fileformat             = string_view(),
               ProgressCallback progress_callback = nullptr,
               void* progress_callback_data       = nullptr) const;

    // DEPRECATED(1.9): old version did not have the data type
    bool write(string_view filename, string_view fileformat,
               ProgressCallback progress_callback = nullptr,
               void* progress_callback_data       = nullptr) const
    {
        return write(filename, TypeUnknown, fileformat, progress_callback,
                     progress_callback_data);
    }

    /// Set the pixel data format that will be used for subsequent `write()`
    /// calls that do not themselves request a specific data type request.
    ///
    /// Note that this does not affect the variety of `write()` that takes
    /// an open `ImageOutput*` as a parameter.
    ///
    /// @param  format
    ///             The data type to be used for all channels.
    void set_write_format(TypeDesc format);

    /// Set the per-channel pixel data format that will be used for
    /// subsequent `write()` calls that do not themselves request a specific
    /// data type request.
    ///
    /// @param  format
    ///             The type of each channel (in order). Any channel's
    ///             format specified as `TypeUnknown` will default to be
    ///             whatever type is described in the ImageSpec of the
    ///             buffer.
    void set_write_format(cspan<TypeDesc> format);

    /// Override the tile sizing for subsequent calls to the `write()`
    /// method (the variety that does not take an open `ImageOutput*`).
    /// Setting all three dimensions to 0 indicates that the output should
    /// be a scanline-oriented file.
    ///
    /// This lets you write a tiled file from an ImageBuf that may have been
    /// read originally from a scanline file, or change the dimensions of a
    /// tiled file, or to force the file written to be scanline even if it
    /// was originally read from a tiled file.
    ///
    /// In all cases, if the file format ultimately written does not support
    /// tiling, or the tile dimensions requested, a suitable supported
    /// tiling choice will be made automatically.
    void set_write_tiles(int width = 0, int height = 0, int depth = 0);

    /// Write the pixels of the ImageBuf to an open ImageOutput. The
    /// ImageOutput must have already been opened with a spec that indicates
    /// a resolution identical to that of this ImageBuf (but it may have
    /// specified a different pixel data type, in which case data
    /// conversions will happen automatically).
    ///
    /// Note that since this uses an already-opened `ImageOutput`, which is
    /// too late to change how it was opened, it does not honor any prior
    /// calls to `set_write_format` or `set_write_tiles`.
    ///
    /// @param  out
    ///             A pointer to an already-opened `ImageOutput` to which
    ///             the pixels of the ImageBuf will be written.
    /// @param  progress_callback/progress_callback_data
    ///             If `progress_callback` is non-NULL, the underlying
    ///             write, if expensive, may make several calls to
    ///                 `progress_callback(progress_callback_data, portion_done)`
    ///             which allows you to implement some sort of progress
    ///             meter.
    /// @returns  `true` if all went ok, `false` if there were errors
    ///           writing.  It does NOT close the file when it's done (and
    ///           so may be called in a loop to write a multi-image file).
    bool write(ImageOutput* out, ProgressCallback progress_callback = nullptr,
               void* progress_callback_data = nullptr) const;

    /// @}

    /// @{
    /// @name  Copying ImageBuf's and blocks of pixels

    /// Copy assignment.
    const ImageBuf& operator=(const ImageBuf& src);

    /// Move assignment.
    const ImageBuf& operator=(ImageBuf&& src);

    /// Copy all the metadata from `src` to `*this` (except for pixel data
    /// resolution, channel types and names, and data format).
    void copy_metadata(const ImageBuf& src);

    /// Copy the pixel data from `src` to `*this`, automatically converting
    /// to the existing data format of `*this`.  It only copies pixels in
    /// the overlap regions (and channels) of the two images; pixel data in
    /// `*this` that do exist in `src` will be set to 0, and pixel data in
    /// `src` that do not exist in `*this` will not be copied.
    bool copy_pixels(const ImageBuf& src);

    /// Try to copy the pixels and metadata from `src` to `*this`
    /// (optionally with an explicit data format conversion).
    ///
    /// If the previous state of `*this` was uninitialized, owning its own
    /// local pixel memory, or referring to a read-only image backed by
    /// ImageCache, then local pixel memory will be allocated to hold the
    /// new pixels and the call always succeeds unless the memory cannot be
    /// allocated. In this case, the `format` parameter may request a pixel
    /// data type that is different from that of the source buffer.
    ///
    /// If `*this` previously referred to an app-owned memory buffer, the
    /// memory cannot be re-allocated, so the call will only succeed if the
    /// app-owned buffer is already the correct resolution and number of
    /// channels.  The data type of the pixels will be converted
    /// automatically to the data type of the app buffer.
    ///
    /// @param  src
    ///             Another ImageBuf from which to copy the pixels and
    ///             metadata.
    /// @param  format
    ///             Optionally request the pixel data type to be used. The
    ///             default of `TypeUnknown` means to use whatever data type
    ///             is used by the `src`. If `*this` is already initialized
    ///             and has `APPBUFFER` storage ("wrapping" an application
    ///             buffer), this parameter is ignored.
    /// @returns
    ///             `true` upon success or `false` upon error/failure.
    bool copy(const ImageBuf& src, TypeDesc format = TypeUnknown);

    /// Return a full copy of `this` ImageBuf (optionally with an explicit
    /// data format conversion).
    ImageBuf copy(TypeDesc format /*= TypeDesc::UNKNOWN*/) const;

    /// Swap the entire contents with another ImageBuf.
    void swap(ImageBuf& other) { std::swap(m_impl, other.m_impl); }

    /// @}


    /// @{
    /// @name  Getting and setting pixel values

    /// Retrieve a single channel of one pixel.
    ///
    /// @param x/y/z
    ///             The pixel coordinates.
    /// @param c
    ///             The channel index to retrieve.
    /// @param wrap
    ///             WrapMode that determines the behavior if the pixel
    ///             coordinates are outside the data window: `WrapBlack`,
    ///             `WrapClamp`, `WrapPeriodic`, `WrapMirror`.
    /// @returns
    ///            The data value, converted to a `float`.
    float getchannel(int x, int y, int z, int c,
                     WrapMode wrap = WrapBlack) const;

    /// Retrieve the pixel value by x, y, z pixel indices, placing its
    /// contents in `pixel[0..n-1]` where *n* is the smaller of
    /// `maxchannels` the actual number of channels stored in the buffer.
    ///
    /// @param x/y/z
    ///             The pixel coordinates.
    /// @param pixel
    ///             The results are stored in `pixel[0..nchannels-1]`. It is
    ///             up to the caller to ensure that `pixel` points to enough
    ///             memory to hold the required number of channels.
    /// @param maxchannels
    ///             Optional clamp to the number of channels retrieved.
    /// @param wrap
    ///             WrapMode that determines the behavior if the pixel
    ///             coordinates are outside the data window: `WrapBlack`,
    ///             `WrapClamp`, `WrapPeriodic`, `WrapMirror`.
    void getpixel(int x, int y, int z, float* pixel, int maxchannels = 1000,
                  WrapMode wrap = WrapBlack) const;

    // Simplified version: 2D, black wrap.
    void getpixel(int x, int y, float* pixel, int maxchannels = 1000) const
    {
        getpixel(x, y, 0, pixel, maxchannels);
    }

    /// Sample the image plane at pixel coordinates (x,y), using linear
    /// interpolation between pixels, placing the result in `pixel[]`.
    ///
    /// @param x/y
    ///             The pixel coordinates. Note that pixel data values
    ///             themselves are at the pixel centers, so pixel (i,j) is
    ///             at image plane coordinate (i+0.5, j+0.5).
    /// @param pixel
    ///             The results are stored in `pixel[0..nchannels-1]`. It is
    ///             up to the caller to ensure that `pixel` points to enough
    ///             memory to hold the number of channels in the image.
    /// @param wrap
    ///             WrapMode that determines the behavior if the pixel
    ///             coordinates are outside the data window: `WrapBlack`,
    ///             `WrapClamp`, `WrapPeriodic`, `WrapMirror`.
    void interppixel(float x, float y, float* pixel,
                     WrapMode wrap = WrapBlack) const;

    /// Linearly interpolate at NDC coordinates (s,t), where (0,0) is
    /// the upper left corner of the display window, (1,1) the lower
    /// right corner of the display window.
    ///
    /// @note `interppixel()` uses pixel coordinates (ranging 0..resolution)
    /// whereas `interppixel_NDC()` uses NDC coordinates (ranging 0..1).
    void interppixel_NDC(float s, float t, float* pixel,
                         WrapMode wrap = WrapBlack) const;

    // DEPCRECATED (1.5) synonym for interppixel_NDC.
    void interppixel_NDC_full(float s, float t, float* pixel,
                              WrapMode wrap = WrapBlack) const;

    /// Bicubic interpolation at pixel coordinates (x,y).
    void interppixel_bicubic(float x, float y, float* pixel,
                             WrapMode wrap = WrapBlack) const;

    /// Bicubic interpolation at NDC space coordinates (s,t), where (0,0)
    /// is the upper left corner of the display (a.k.a. "full") window,
    /// (1,1) the lower right corner of the display window.
    void interppixel_bicubic_NDC(float s, float t, float* pixel,
                                 WrapMode wrap = WrapBlack) const;


    /// Set the pixel with coordinates (x,y,0) to have the values in span
    /// `pixel[]`.  The number of channels copied is the minimum of the span
    /// length and the actual number of channels in the image.
    void setpixel(int x, int y, cspan<float> pixel)
    {
        setpixel(x, y, 0, pixel);
    }

    /// Set the pixel with coordinates (x,y,z) to have the values in span
    /// `pixel[]`.  The number of channels copied is the minimum of the span
    /// length and the actual number of channels in the image.
    void setpixel(int x, int y, int z, cspan<float> pixel)
    {
        setpixel(x, y, z, pixel.data(), int(pixel.size()));
    }

    /// Set the `i`-th pixel value of the image (out of width*height*depth),
    /// from floating-point values in span `pixel[]`.  The number of
    /// channels copied is the minimum of the span length and the actual
    /// number of channels in the image.
    void setpixel(int i, cspan<float> pixel)
    {
        setpixel(i, pixel.data(), int(pixel.size()));
    }

    /// Set the pixel with coordinates (x,y,0) to have the values in
    /// pixel[0..n-1].  The number of channels copied, n, is the minimum
    /// of maxchannels and the actual number of channels in the image.
    void setpixel(int x, int y, const float* pixel, int maxchannels = 1000)
    {
        setpixel(x, y, 0, pixel, maxchannels);
    }

    /// Set the pixel with coordinates (x,y,z) to have the values in
    /// `pixel[0..n-1]`.  The number of channels copied, n, is the minimum
    /// of `maxchannels` and the actual number of channels in the image.
    void setpixel(int x, int y, int z, const float* pixel,
                  int maxchannels = 1000);

    /// Set the `i`-th pixel value of the image (out of width*height*depth),
    /// from floating-point values in `pixel[]`.  Set at most
    /// `maxchannels` (will be clamped to the actual number of channels).
    void setpixel(int i, const float* pixel, int maxchannels = 1000);

    /// Retrieve the rectangle of pixels spanning the ROI (including
    /// channels) at the current subimage and MIP-map level, storing the
    /// pixel values beginning at the address specified by result and with
    /// the given strides (by default, AutoStride means the usual contiguous
    /// packing of pixels) and converting into the data type described by
    /// `format`.  It is up to the caller to ensure that result points to an
    /// area of memory big enough to accommodate the requested rectangle.
    /// Return true if the operation could be completed, otherwise return
    /// false.
    bool get_pixels(ROI roi, TypeDesc format, void* result,
                    stride_t xstride = AutoStride,
                    stride_t ystride = AutoStride,
                    stride_t zstride = AutoStride) const;

    /// Copy the data into the given ROI of the ImageBuf. The data points to
    /// values specified by `format`, with layout detailed by the stride
    /// values (in bytes, with AutoStride indicating "contiguous" layout).
    /// It is up to the caller to ensure that data points to an area of
    /// memory big enough to account for the ROI. Return true if the
    /// operation could be completed, otherwise return false.
    bool set_pixels(ROI roi, TypeDesc format, const void* data,
                    stride_t xstride = AutoStride,
                    stride_t ystride = AutoStride,
                    stride_t zstride = AutoStride);

    /// @}

    /// @{
    /// @name  Getting and setting information about an ImageBuf

    /// Returns `true` if the ImageBuf is initialized, `false` if not yet
    /// initialized.
    bool initialized() const;

    /// Which type of storage is being used for the pixels? Returns an
    /// enumerated type describing the type of storage currently employed by
    /// the ImageBuf: `UNINITIALIZED` (no storage), `LOCALBUFFER` (the
    /// ImageBuf has allocated and owns the pixel memory), `APPBUFFER` (the
    /// ImageBuf "wraps" memory owned by the calling application), or
    /// `IMAGECACHE` (the image is backed by an ImageCache).
    IBStorage storage() const;

    /// Return a read-only (const) reference to the image spec that
    /// describes the buffer.
    const ImageSpec& spec() const;

    /// Return a writable reference to the ImageSpec that describes the
    /// buffer.  It's ok to modify most of the metadata, but if you modify
    /// the spec's `format`, `width`, `height`, or `depth` fields, you get
    /// the pain you deserve, as the ImageBuf will no longer have correct
    /// knowledge of its pixel memory layout.  USE WITH EXTREME CAUTION.
    ImageSpec& specmod();

    /// Return a read-only (const) reference to the "native" image spec
    /// (that describes the file, which may be slightly different than
    /// the spec of the ImageBuf, particularly if the IB is backed by an
    /// ImageCache that is imposing some particular data format or tile
    /// size).
    ///
    /// This may differ from `spec()` --- for example, if a data format
    /// conversion was requested, if the buffer is backed by an ImageCache
    /// which stores the pixels internally in a different data format than
    /// that of the file, or if the file had differing per-channel data
    /// formats (ImageBuf must contain a single data format for all
    /// channels).
    const ImageSpec& nativespec() const;

    /// Returns the name of the buffer (name of the file, for an ImageBuf
    /// read from disk).
    string_view name(void) const;

    /// Return the name of the image file format of the disk file we read
    /// into this image, for example `"openexr"`.  Returns an empty string
    /// if this image was not the result of a read().
    string_view file_format_name(void) const;

    /// Return the index of the subimage the ImageBuf is currently holding.
    int subimage() const;

    /// Return the number of subimages in the file.
    int nsubimages() const;

    /// Return the index of the miplevel the ImageBuf is currently holding.
    int miplevel() const;

    /// Return the number of miplevels of the current subimage.
    int nmiplevels() const;

    /// Return the number of color channels in the image. This is equivalent
    /// to `spec().nchannels`.
    int nchannels() const;

    /// Return the beginning (minimum) x coordinate of the defined image.
    int xbegin() const;
    /// Return the end (one past maximum) x coordinate of the defined image.
    int xend() const;
    /// Return the beginning (minimum) y coordinate of the defined image.
    int ybegin() const;
    /// Return the end (one past maximum) y coordinate of the defined image.
    int yend() const;
    /// Return the beginning (minimum) z coordinate of the defined image.
    int zbegin() const;
    /// Return the end (one past maximum) z coordinate of the defined image.
    int zend() const;
    /// Return the minimum x coordinate of the defined image.
    int xmin() const;
    /// Return the maximum x coordinate of the defined image.
    int xmax() const;
    /// Return the minimum y coordinate of the defined image.
    int ymin() const;
    /// Return the maximum y coordinate of the defined image.
    int ymax() const;
    /// Return the minimum z coordinate of the defined image.
    int zmin() const;
    /// Return the maximum z coordinate of the defined image.
    int zmax() const;

    /// Return the current `"Orientation"` metadata for the image, per the
    /// table in `sec-metadata-orientation`_
    int orientation() const;

    /// Set the `"Orientation"` metadata for the image.
    void set_orientation(int orient);

    // Return the width, height, or full versions, if the image were
    // positioned for display in its designated orientation.
    int oriented_width() const;
    int oriented_height() const;
    int oriented_x() const;
    int oriented_y() const;
    int oriented_full_width() const;
    int oriented_full_height() const;
    int oriented_full_x() const;
    int oriented_full_y() const;

    /// Alters the metadata of the spec in the ImageBuf to reset the
    /// "origin" of the pixel data window to be the specified coordinates.
    /// This does not affect the size of the pixel data window, only its
    /// position.
    void set_origin(int x, int y, int z = 0);

    /// Set the "full" (a.k.a. display) window to Alters the metadata of the
    /// spec in the ImageBuf to reset the "full" image size (a.k.a.
    /// "display window") to
    ///
    ///     [xbegin,xend) x [ybegin,yend) x [zbegin,zend)`
    ///
    /// This does not affect the size of the pixel data window.
    void set_full(int xbegin, int xend, int ybegin, int yend, int zbegin,
                  int zend);

    /// Return pixel data window for this ImageBuf as a ROI.
    ROI roi() const;

    /// Return full/display window for this ImageBuf as a ROI.
    ROI roi_full() const;

    /// Set full/display window for this ImageBuf to a ROI.
    /// Does NOT change the channels of the spec, regardless of `newroi`.
    void set_roi_full(const ROI& newroi);

    /// Is the specified roi completely contained in the data window of
    /// this ImageBuf?
    bool contains_roi(ROI roi) const;

    bool pixels_valid(void) const;

    /// The data type of the pixels stored in the buffer (equivalent to
    /// `spec().format`).
    TypeDesc pixeltype() const;

    /// Return a raw pointer to "local" pixel memory, if they are fully in
    /// RAM and not backed by an ImageCache, or `nullptr` otherwise.  You
    /// can also test it like a bool to find out if pixels are local.
    void* localpixels();
    const void* localpixels() const;

    /// Pixel-to-pixel stride within the localpixels memory.
    stride_t pixel_stride() const;
    /// Scanline-to-scanline stride within the localpixels memory.
    stride_t scanline_stride() const;
    /// Z plane stride within the localpixels memory.
    stride_t z_stride() const;

    /// Are the pixels backed by an ImageCache, rather than the whole
    /// image being in RAM somewhere?
    bool cachedpixels() const;

    /// A pointer to the underlying ImageCache.
    ImageCache* imagecache() const;

    /// Return the address where pixel `(x,y,z)`, channel `ch`, is stored in
    /// the image buffer.  Use with extreme caution!  Will return `nullptr`
    /// if the pixel values aren't local in RAM.
    const void* pixeladdr(int x, int y, int z = 0, int ch = 0) const;
    void* pixeladdr(int x, int y, int z = 0, int ch = 0);

    /// Return the index of pixel (x,y,z). If check_range is true, return
    /// -1 for an invalid coordinate that is not within the data window.
    int pixelindex(int x, int y, int z, bool check_range = false) const;

    /// Set the threading policy for this ImageBuf, controlling the maximum
    /// amount of parallelizing thread "fan-out" that might occur during
    /// expensive operations. The default of 0 means that the global
    /// `attribute("threads")` value should be used (which itself defaults
    /// to using as many threads as cores).
    ///
    /// The main reason to change this value is to set it to 1 to indicate
    /// that the calling thread should do all the work rather than spawning
    /// new threads. That is probably the desired behavior in situations
    /// where the calling application has already spawned multiple worker
    /// threads.
    void threads(int n) const;

    /// Retrieve the current thread-spawning policy of this ImageBuf.
    int threads() const;

    /// @}

    /// @{
    /// @name Error handling

    /// Add simple string to the error message list for this IB.
    void error(const std::string& message) const;

    /// Error reporting for ImageBuf: call this with Python / {fmt} /
    /// std::format style formatting specification.
    template<typename... Args>
    void fmterror(const char* fmt, const Args&... args) const
    {
        error(Strutil::fmt::format(fmt, args...));
    }

    /// Error reporting for ImageBuf: call this with printf-like arguments
    /// to report an error.
    template<typename... Args>
    void errorf(const char* fmt, const Args&... args) const
    {
        error(Strutil::sprintf(fmt, args...));
    }

    /// Error reporting for ImageBuf: call this with Strutil::format
    /// formatting conventions. Beware, this is in transition, is currently
    /// printf-like but will someday change to python-like!
    template<typename... Args>
    void error(const char* fmt, const Args&... args) const
    {
        error(Strutil::format(fmt, args...));
    }

    /// Returns `true` if the ImageBuf has had an error and has an error
    /// message ready to retrieve via `geterror()`.
    bool has_error(void) const;

    /// Return the text of all error messages issued since `geterror()` was
    /// called (or an empty string if no errors are pending).  This also
    /// clears the error message for next time.
    std::string geterror(void) const;

    /// @}

    /// @{
    /// @name  Deep data in an ImageBuf

    /// Does this ImageBuf store deep data? Returns `true` if the ImageBuf
    /// holds a "deep" image, `false` if the ImageBuf holds an ordinary
    /// pixel-based image.
    bool deep() const;

    /// Retrieve the number of deep data samples corresponding to pixel
    /// (x,y,z).  Return 0 if not a deep image, or if the pixel is outside
    /// of the data window, or if the designated pixel has no deep samples.
    int deep_samples(int x, int y, int z = 0) const;

    /// Return a pointer to the raw data of pixel `(x,y,z)`, channel `c`,
    /// sample `s`. Return `nullptr` if the pixel coordinates or channel
    /// number are out of range, if the pixel/channel has no deep samples,
    /// or if the image is not deep. Use with caution --- these pointers may
    /// be invalidated by calls that adjust the number of samples in any
    /// pixel.
    const void* deep_pixel_ptr(int x, int y, int z, int c, int s = 0) const;

    /// Return the value (as a `float`) of sample `s` of channel `c` of
    /// pixel `(x,y,z)`.  Return 0 if not a deep image or if the pixel
    /// coordinates or channel number are out of range or if that pixel has
    /// no deep samples.
    float deep_value(int x, int y, int z, int c, int s) const;

    /// Return the value (as a `uint32_t`) of sample `s` of channel `c` of
    /// pixel `(x,y,z)`.  Return 0 if not a deep image or if the pixel
    /// coordinates or channel number are out of range or if that pixel has
    /// no deep samples.
    uint32_t deep_value_uint(int x, int y, int z, int c, int s) const;

    /// Set the number of deep samples for pixel (x,y,z).  If data has
    /// already been allocated, this is equivalent to inserting or erasing
    /// samples.
    void set_deep_samples(int x, int y, int z, int nsamples);

    /// Insert `nsamples` new samples, starting at position `samplepos` of
    /// pixel (x,y,z).
    void deep_insert_samples(int x, int y, int z, int samplepos, int nsamples);

    /// Remove `nsamples` samples, starting at position `samplepos` of pixel
    /// (x,y,z).
    void deep_erase_samples(int x, int y, int z, int samplepos, int nsamples);

    /// Set the value of sample `s` of channel `c` of pixel `(x,y,z)` to a
    /// `float` value (it is expected that channel `c` is a floating point
    /// type).
    void set_deep_value(int x, int y, int z, int c, int s, float value);

    /// Set the value of sample `s` of channel `c` of pixel `(x,y,z)` to a
    /// `uint32_t` value (it is expected that channel `c` is an integer
    /// type).
    void set_deep_value(int x, int y, int z, int c, int s, uint32_t value);

    /// Copy a deep pixel from another ImageBuf -- it is required to have
    /// the same channels.
    bool copy_deep_pixel(int x, int y, int z, const ImageBuf& src, int srcx,
                         int srcy, int srcz);

    /// Retrieve the "deep" data.
    DeepData* deepdata();
    const DeepData* deepdata() const;

    /// @}

    /// Return the `WrapMode` corresponding to the name (`"default"`,
    /// `"black"`, `"clamp"`, `"periodic"`, `"mirror"`). For an unknown
    /// name, this will return `WrapDefault`.
    static WrapMode WrapMode_from_string(string_view name);


    friend class IteratorBase;

    class IteratorBase {
    public:
        IteratorBase(const ImageBuf& ib, WrapMode wrap)
            : m_ib(&ib)
        {
            init_ib(wrap);
            range_is_image();
        }

        /// Construct valid iteration region from ImageBuf and ROI.
        IteratorBase(const ImageBuf& ib, const ROI& roi, WrapMode wrap)
            : m_ib(&ib)
        {
            init_ib(wrap);
            if (roi.defined()) {
                m_rng_xbegin = roi.xbegin;
                m_rng_xend   = roi.xend;
                m_rng_ybegin = roi.ybegin;
                m_rng_yend   = roi.yend;
                m_rng_zbegin = roi.zbegin;
                m_rng_zend   = roi.zend;
            } else {
                range_is_image();
            }
        }

        /// Construct from an ImageBuf and designated region -- iterate
        /// over region, starting with the upper left pixel.
        IteratorBase(const ImageBuf& ib, int xbegin, int xend, int ybegin,
                     int yend, int zbegin, int zend, WrapMode wrap)
            : m_ib(&ib)
        {
            init_ib(wrap);
            m_rng_xbegin = xbegin;
            m_rng_xend   = xend;
            m_rng_ybegin = ybegin;
            m_rng_yend   = yend;
            m_rng_zbegin = zbegin;
            m_rng_zend   = zend;
        }

        IteratorBase(const IteratorBase& i)
            : m_ib(i.m_ib)
            , m_rng_xbegin(i.m_rng_xbegin)
            , m_rng_xend(i.m_rng_xend)
            , m_rng_ybegin(i.m_rng_ybegin)
            , m_rng_yend(i.m_rng_yend)
            , m_rng_zbegin(i.m_rng_zbegin)
            , m_rng_zend(i.m_rng_zend)
            , m_proxydata(i.m_proxydata)
        {
            init_ib(i.m_wrap);
        }

        ~IteratorBase()
        {
            if (m_tile)
                m_ib->imagecache()->release_tile(m_tile);
        }

        /// Assign one IteratorBase to another
        ///
        const IteratorBase& assign_base(const IteratorBase& i)
        {
            if (m_tile)
                m_ib->imagecache()->release_tile(m_tile);
            m_tile      = nullptr;
            m_proxydata = i.m_proxydata;
            m_ib        = i.m_ib;
            init_ib(i.m_wrap);
            m_rng_xbegin = i.m_rng_xbegin;
            m_rng_xend   = i.m_rng_xend;
            m_rng_ybegin = i.m_rng_ybegin;
            m_rng_yend   = i.m_rng_yend;
            m_rng_zbegin = i.m_rng_zbegin;
            m_rng_zend   = i.m_rng_zend;
            return *this;
        }

        /// Retrieve the current x location of the iterator.
        ///
        int x() const { return m_x; }
        /// Retrieve the current y location of the iterator.
        ///
        int y() const { return m_y; }
        /// Retrieve the current z location of the iterator.
        ///
        int z() const { return m_z; }

        /// Is the current location within the designated iteration range?
        bool valid() const { return m_valid; }

        /// Is the location (x,y[,z]) within the designated iteration
        /// range?
        bool valid(int x_, int y_, int z_ = 0) const
        {
            return (x_ >= m_rng_xbegin && x_ < m_rng_xend && y_ >= m_rng_ybegin
                    && y_ < m_rng_yend && z_ >= m_rng_zbegin
                    && z_ < m_rng_zend);
        }

        /// Is the location (x,y[,z]) within the region of the ImageBuf
        /// that contains pixel values (sometimes called the "data window")?
        bool exists(int x_, int y_, int z_ = 0) const
        {
            return (x_ >= m_img_xbegin && x_ < m_img_xend && y_ >= m_img_ybegin
                    && y_ < m_img_yend && z_ >= m_img_zbegin
                    && z_ < m_img_zend);
        }
        /// Does the current location exist within the ImageBuf's
        /// data window?
        bool exists() const { return m_exists; }

        /// Are we finished iterating over the region?
        //
        bool done() const
        {
            // We're "done" if we are both invalid and in exactly the
            // spot that we would end up after iterating off of the last
            // pixel in the range.  (The m_valid test is just a quick
            // early-out for when we're in the correct pixel range.)
            return (m_valid == false && m_x == m_rng_xbegin
                    && m_y == m_rng_ybegin && m_z == m_rng_zend);
        }

        /// Retrieve the number of deep data samples at this pixel.
        int deep_samples() const { return m_ib->deep_samples(m_x, m_y, m_z); }

        /// Return the wrap mode
        WrapMode wrap() const { return m_wrap; }

        /// Explicitly point the iterator.  This results in an invalid
        /// iterator if outside the previously-designated region.
        void pos(int x_, int y_, int z_ = 0)
        {
            if (x_ == m_x + 1 && x_ < m_rng_xend && y_ == m_y && z_ == m_z
                && m_valid && m_exists) {
                // Special case for what is in effect just incrementing x
                // within the iteration region.
                m_x = x_;
                pos_xincr();
                // Not necessary? m_exists = (x_ < m_img_xend);
                OIIO_DASSERT((x_ < m_img_xend) == m_exists);
                return;
            }
            bool v = valid(x_, y_, z_);
            bool e = exists(x_, y_, z_);
            if (m_localpixels) {
                if (e)
                    m_proxydata = (char*)m_ib->pixeladdr(x_, y_, z_);
                else {  // pixel not in data window
                    m_x = x_;
                    m_y = y_;
                    m_z = z_;
                    if (m_wrap == WrapBlack) {
                        m_proxydata = (char*)m_ib->blackpixel();
                    } else {
                        if (m_ib->do_wrap(x_, y_, z_, m_wrap))
                            m_proxydata = (char*)m_ib->pixeladdr(x_, y_, z_);
                        else
                            m_proxydata = (char*)m_ib->blackpixel();
                    }
                    m_valid  = v;
                    m_exists = e;
                    return;
                }
            } else if (!m_deep)
                m_proxydata = (char*)m_ib->retile(x_, y_, z_, m_tile,
                                                  m_tilexbegin, m_tileybegin,
                                                  m_tilezbegin, m_tilexend, e,
                                                  m_wrap);
            m_x      = x_;
            m_y      = y_;
            m_z      = z_;
            m_valid  = v;
            m_exists = e;
        }

        /// Increment to the next pixel in the region.
        ///
        OIIO_FORCEINLINE void operator++()
        {
            if (++m_x < m_rng_xend) {
                // Special case: we only incremented x, didn't change y
                // or z, and the previous position was within the data
                // window.  Call a shortcut version of pos.
                if (m_exists) {
                    pos_xincr();
                    return;
                }
            } else {
                // Wrap to the next scanline
                m_x = m_rng_xbegin;
                if (++m_y >= m_rng_yend) {
                    m_y = m_rng_ybegin;
                    if (++m_z >= m_rng_zend) {
                        m_valid = false;  // shortcut -- finished iterating
                        return;
                    }
                }
            }
            pos(m_x, m_y, m_z);
        }
        /// Increment to the next pixel in the region.
        ///
        void operator++(int) { ++(*this); }

        /// Return the iteration range
        ROI range() const
        {
            return ROI(m_rng_xbegin, m_rng_xend, m_rng_ybegin, m_rng_yend,
                       m_rng_zbegin, m_rng_zend, 0, m_ib->nchannels());
        }

        /// Reset the iteration range for this iterator and reposition to
        /// the beginning of the range, but keep referring to the same
        /// image.
        void rerange(int xbegin, int xend, int ybegin, int yend, int zbegin,
                     int zend, WrapMode wrap = WrapDefault)
        {
            m_x          = 1 << 31;
            m_y          = 1 << 31;
            m_z          = 1 << 31;
            m_wrap       = (wrap == WrapDefault ? WrapBlack : wrap);
            m_rng_xbegin = xbegin;
            m_rng_xend   = xend;
            m_rng_ybegin = ybegin;
            m_rng_yend   = yend;
            m_rng_zbegin = zbegin;
            m_rng_zend   = zend;
            pos(xbegin, ybegin, zbegin);
        }

    protected:
        friend class ImageBuf;
        friend class ImageBufImpl;
        const ImageBuf* m_ib = nullptr;
        bool m_valid = false, m_exists = false;
        bool m_deep        = false;
        bool m_localpixels = false;
        // Image boundaries
        int m_img_xbegin, m_img_xend, m_img_ybegin, m_img_yend, m_img_zbegin,
            m_img_zend;
        // Iteration range
        int m_rng_xbegin, m_rng_xend, m_rng_ybegin, m_rng_yend, m_rng_zbegin,
            m_rng_zend;
        int m_x, m_y, m_z;
        ImageCache::Tile* m_tile = nullptr;
        int m_tilexbegin, m_tileybegin, m_tilezbegin;
        int m_tilexend;
        int m_nchannels;
        size_t m_pixel_bytes;
        char* m_proxydata = nullptr;
        WrapMode m_wrap   = WrapBlack;

        // Helper called by ctrs -- set up some locally cached values
        // that are copied or derived from the ImageBuf.
        void init_ib(WrapMode wrap)
        {
            const ImageSpec& spec(m_ib->spec());
            m_deep        = spec.deep;
            m_localpixels = (m_ib->localpixels() != nullptr);
            m_img_xbegin  = spec.x;
            m_img_xend    = spec.x + spec.width;
            m_img_ybegin  = spec.y;
            m_img_yend    = spec.y + spec.height;
            m_img_zbegin  = spec.z;
            m_img_zend    = spec.z + spec.depth;
            m_nchannels   = spec.nchannels;
            //            m_tilewidth = spec.tile_width;
            m_pixel_bytes = spec.pixel_bytes();
            m_x           = 1 << 31;
            m_y           = 1 << 31;
            m_z           = 1 << 31;
            m_wrap        = (wrap == WrapDefault ? WrapBlack : wrap);
        }

        // Helper called by ctrs -- make the iteration range the full
        // image data window.
        void range_is_image()
        {
            m_rng_xbegin = m_img_xbegin;
            m_rng_xend   = m_img_xend;
            m_rng_ybegin = m_img_ybegin;
            m_rng_yend   = m_img_yend;
            m_rng_zbegin = m_img_zbegin;
            m_rng_zend   = m_img_zend;
        }

        // Helper called by pos(), but ONLY for the case where we are
        // moving from an existing pixel to the next spot in +x.
        // Note: called *after* m_x was incremented!
        OIIO_FORCEINLINE void pos_xincr()
        {
            OIIO_DASSERT(m_exists && m_valid);   // precondition
            OIIO_DASSERT(valid(m_x, m_y, m_z));  // should be true by definition
            m_proxydata += m_pixel_bytes;
            if (m_localpixels) {
                if (OIIO_UNLIKELY(m_x >= m_img_xend)) {
                    // Ran off the end of the row
                    m_exists = false;
                    if (m_wrap == WrapBlack) {
                        m_proxydata = (char*)m_ib->blackpixel();
                    } else {
                        int x = m_x, y = m_y, z = m_z;
                        if (m_ib->do_wrap(x, y, z, m_wrap))
                            m_proxydata = (char*)m_ib->pixeladdr(x, y, z);
                        else
                            m_proxydata = (char*)m_ib->blackpixel();
                    }
                }
            } else if (m_deep) {
                m_proxydata = nullptr;
            } else {
                // Cached image
                bool e = m_x < m_img_xend;
                if (OIIO_UNLIKELY(!(e && m_x < m_tilexend && m_tile))) {
                    // Crossed a tile boundary
                    m_proxydata = (char*)m_ib->retile(m_x, m_y, m_z, m_tile,
                                                      m_tilexbegin,
                                                      m_tileybegin,
                                                      m_tilezbegin, m_tilexend,
                                                      e, m_wrap);
                    m_exists    = e;
                }
            }
        }

        // Set to the "done" position
        void pos_done()
        {
            m_valid = false;
            m_x     = m_rng_xbegin;
            m_y     = m_rng_ybegin;
            m_z     = m_rng_zend;
        }

        // Make sure it's writable. Use with caution!
        void make_writeable()
        {
            if (!m_localpixels) {
                const_cast<ImageBuf*>(m_ib)->make_writeable(true);
                OIIO_DASSERT(m_ib->storage() != IMAGECACHE);
                m_tile      = nullptr;
                m_proxydata = nullptr;
                init_ib(m_wrap);
            }
        }
    };

    /// Templated class for referring to an individual pixel in an
    /// ImageBuf, iterating over the pixels of an ImageBuf, or iterating
    /// over the pixels of a specified region of the ImageBuf
    /// [xbegin..xend) X [ybegin..yend).  It is templated on BUFT, the
    /// type known to be in the internal representation of the ImageBuf,
    /// and USERT, the type that the user wants to retrieve or set the
    /// data (defaulting to float).  the whole idea is to allow this:
    /// \code
    ///   ImageBuf img (...);
    ///   ImageBuf::Iterator<float> pixel (img, 0, 512, 0, 512);
    ///   for (  ;  ! pixel.done();  ++pixel) {
    ///       for (int c = 0;  c < img.nchannels();  ++c) {
    ///           float x = pixel[c];
    ///           pixel[c] = ...;
    ///       }
    ///   }
    /// \endcode
    ///
    template<typename BUFT, typename USERT = float>
    class Iterator : public IteratorBase {
    public:
        /// Construct from just an ImageBuf -- iterate over the whole
        /// region, starting with the upper left pixel of the region.
        Iterator(ImageBuf& ib, WrapMode wrap = WrapDefault)
            : IteratorBase(ib, wrap)
        {
            make_writeable();
            pos(m_rng_xbegin, m_rng_ybegin, m_rng_zbegin);
            if (m_rng_xbegin == m_rng_xend || m_rng_ybegin == m_rng_yend
                || m_rng_zbegin == m_rng_zend)
                pos_done();  // make empty range look "done"
        }
        /// Construct from an ImageBuf and a specific pixel index.
        /// The iteration range is the full image.
        Iterator(ImageBuf& ib, int x, int y, int z = 0,
                 WrapMode wrap = WrapDefault)
            : IteratorBase(ib, wrap)
        {
            make_writeable();
            pos(x, y, z);
        }
        /// Construct read-write iteration region from ImageBuf and ROI.
        Iterator(ImageBuf& ib, const ROI& roi, WrapMode wrap = WrapDefault)
            : IteratorBase(ib, roi, wrap)
        {
            make_writeable();
            pos(m_rng_xbegin, m_rng_ybegin, m_rng_zbegin);
            if (m_rng_xbegin == m_rng_xend || m_rng_ybegin == m_rng_yend
                || m_rng_zbegin == m_rng_zend)
                pos_done();  // make empty range look "done"
        }
        /// Construct from an ImageBuf and designated region -- iterate
        /// over region, starting with the upper left pixel.
        Iterator(ImageBuf& ib, int xbegin, int xend, int ybegin, int yend,
                 int zbegin = 0, int zend = 1, WrapMode wrap = WrapDefault)
            : IteratorBase(ib, xbegin, xend, ybegin, yend, zbegin, zend, wrap)
        {
            make_writeable();
            pos(m_rng_xbegin, m_rng_ybegin, m_rng_zbegin);
            if (m_rng_xbegin == m_rng_xend || m_rng_ybegin == m_rng_yend
                || m_rng_zbegin == m_rng_zend)
                pos_done();  // make empty range look "done"
        }
        /// Copy constructor.
        ///
        Iterator(Iterator& i)
            : IteratorBase(i.m_ib, i.m_wrap)
        {
            make_writeable();
            pos(i.m_x, i.m_y, i.m_z);
        }

        ~Iterator() {}

        /// Assign one Iterator to another
        ///
        const Iterator& operator=(const Iterator& i)
        {
            assign_base(i);
            pos(i.m_x, i.m_y, i.m_z);
            return *this;
        }

        /// Dereferencing the iterator gives us a proxy for the pixel,
        /// which we can index for reading or assignment.
        DataArrayProxy<BUFT, USERT>& operator*()
        {
            return *(DataArrayProxy<BUFT, USERT>*)(void*)&m_proxydata;
        }

        /// Array indexing retrieves the value of the i-th channel of
        /// the current pixel.
        USERT operator[](int i) const
        {
            DataArrayProxy<BUFT, USERT> proxy((BUFT*)m_proxydata);
            return proxy[i];
        }

        /// Array referencing retrieve a proxy (which may be "assigned
        /// to") of i-th channel of the current pixel, so that this
        /// works: me[i] = val;
        DataProxy<BUFT, USERT> operator[](int i)
        {
            DataArrayProxy<BUFT, USERT> proxy((BUFT*)m_proxydata);
            return proxy[i];
        }

        void* rawptr() const { return m_proxydata; }

        /// Set the number of deep data samples at this pixel. (Only use
        /// this if deep_alloc() has not yet been called on the buffer.)
        void set_deep_samples(int n)
        {
            return const_cast<ImageBuf*>(m_ib)->set_deep_samples(m_x, m_y, m_z,
                                                                 n);
        }

        /// Retrieve the deep data value of sample s of channel c.
        USERT deep_value(int c, int s) const
        {
            return convert_type<float, USERT>(
                m_ib->deep_value(m_x, m_y, m_z, c, s));
        }
        uint32_t deep_value_uint(int c, int s) const
        {
            return m_ib->deep_value_uint(m_x, m_y, m_z, c, s);
        }

        /// Set the deep data value of sample s of channel c. (Only use this
        /// if deep_alloc() has been called.)
        void set_deep_value(int c, int s, float value)
        {
            return const_cast<ImageBuf*>(m_ib)->set_deep_value(m_x, m_y, m_z, c,
                                                               s, value);
        }
        void set_deep_value(int c, int s, uint32_t value)
        {
            return const_cast<ImageBuf*>(m_ib)->set_deep_value(m_x, m_y, m_z, c,
                                                               s, value);
        }
    };


    /// Just like an ImageBuf::Iterator, except that it refers to a
    /// const ImageBuf.
    template<typename BUFT, typename USERT = float>
    class ConstIterator : public IteratorBase {
    public:
        /// Construct from just an ImageBuf -- iterate over the whole
        /// region, starting with the upper left pixel of the region.
        ConstIterator(const ImageBuf& ib, WrapMode wrap = WrapDefault)
            : IteratorBase(ib, wrap)
        {
            pos(m_rng_xbegin, m_rng_ybegin, m_rng_zbegin);
            if (m_rng_xbegin == m_rng_xend || m_rng_ybegin == m_rng_yend
                || m_rng_zbegin == m_rng_zend)
                pos_done();  // make empty range look "done"
        }
        /// Construct from an ImageBuf and a specific pixel index.
        /// The iteration range is the full image.
        ConstIterator(const ImageBuf& ib, int x_, int y_, int z_ = 0,
                      WrapMode wrap = WrapDefault)
            : IteratorBase(ib, wrap)
        {
            pos(x_, y_, z_);
        }
        /// Construct read-only iteration region from ImageBuf and ROI.
        ConstIterator(const ImageBuf& ib, const ROI& roi,
                      WrapMode wrap = WrapDefault)
            : IteratorBase(ib, roi, wrap)
        {
            pos(m_rng_xbegin, m_rng_ybegin, m_rng_zbegin);
            if (m_rng_xbegin == m_rng_xend || m_rng_ybegin == m_rng_yend
                || m_rng_zbegin == m_rng_zend)
                pos_done();  // make empty range look "done"
        }
        /// Construct from an ImageBuf and designated region -- iterate
        /// over region, starting with the upper left pixel.
        ConstIterator(const ImageBuf& ib, int xbegin, int xend, int ybegin,
                      int yend, int zbegin = 0, int zend = 1,
                      WrapMode wrap = WrapDefault)
            : IteratorBase(ib, xbegin, xend, ybegin, yend, zbegin, zend, wrap)
        {
            pos(m_rng_xbegin, m_rng_ybegin, m_rng_zbegin);
            if (m_rng_xbegin == m_rng_xend || m_rng_ybegin == m_rng_yend
                || m_rng_zbegin == m_rng_zend)
                pos_done();  // make empty range look "done"
        }
        /// Copy constructor.
        ///
        ConstIterator(const ConstIterator& i)
            : IteratorBase(i)
        {
            pos(i.m_x, i.m_y, i.m_z);
        }

        ~ConstIterator() {}

        /// Assign one ConstIterator to another
        ///
        const ConstIterator& operator=(const ConstIterator& i)
        {
            assign_base(i);
            pos(i.m_x, i.m_y, i.m_z);
            return *this;
        }

        /// Dereferencing the iterator gives us a proxy for the pixel,
        /// which we can index for reading or assignment.
        ConstDataArrayProxy<BUFT, USERT>& operator*() const
        {
            return *(ConstDataArrayProxy<BUFT, USERT>*)&m_proxydata;
        }

        /// Array indexing retrieves the value of the i-th channel of
        /// the current pixel.
        USERT operator[](int i) const
        {
            ConstDataArrayProxy<BUFT, USERT> proxy((BUFT*)m_proxydata);
            return proxy[i];
        }

        const void* rawptr() const { return m_proxydata; }

        /// Retrieve the deep data value of sample s of channel c.
        USERT deep_value(int c, int s) const
        {
            return convert_type<float, USERT>(
                m_ib->deep_value(m_x, m_y, m_z, c, s));
        }
        uint32_t deep_value_uint(int c, int s) const
        {
            return m_ib->deep_value_uint(m_x, m_y, m_z, c, s);
        }
    };


protected:
    // PIMPL idiom
    static void impl_deleter(ImageBufImpl*);
    std::unique_ptr<ImageBufImpl, decltype(&impl_deleter)> m_impl;

    // Reset the ImageCache::Tile * to reserve and point to the correct
    // tile for the given pixel, and return the ptr to the actual pixel
    // within the tile.
    const void* retile(int x, int y, int z, ImageCache::Tile*& tile,
                       int& tilexbegin, int& tileybegin, int& tilezbegin,
                       int& tilexend, bool exists,
                       WrapMode wrap = WrapDefault) const;

    const void* blackpixel() const;

    // Given x,y,z known to be outside the pixel data range, and a wrap
    // mode, alter xyz to implement the wrap. Return true if the resulting
    // x,y,z is within the valid pixel data window, false if it still is
    // not.
    bool do_wrap(int& x, int& y, int& z, WrapMode wrap) const;
};


OIIO_NAMESPACE_END
