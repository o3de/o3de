// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/////////////////////////////////////////////////////////////////////////////
/// \file
///
/// Provides a simple API that abstracts the reading and writing of
/// images.  Subclasses, which may be found in DSO/DLL's, implement
/// particular formats.
///
/////////////////////////////////////////////////////////////////////////////

// clang-format off

#pragma once
#define OPENIMAGEIO_IMAGEIO_H

#if defined(_MSC_VER)
// Ignore warnings about DLL exported classes with member variables that are template classes.
// This happens with the std::vector<T> and std::string members of the classes below.
#    pragma warning(disable : 4251)
#endif

#include <cmath>
#include <limits>
#include <string>
#include <vector>

#include <OpenImageIO/span.h>
#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/paramlist.h>
#include <OpenImageIO/platform.h>
#include <OpenImageIO/strutil.h>
#include <OpenImageIO/thread.h>
#include <OpenImageIO/typedesc.h>

OIIO_NAMESPACE_BEGIN

class DeepData;


/// Type we use for stride lengths between pixels, scanlines, or image
/// planes.
using stride_t = int64_t;

/// Type we use to express how many pixels (or bytes) constitute an image,
/// tile, or scanline.
using imagesize_t = uint64_t;

/// Special value to indicate a stride length that should be
/// auto-computed.
const stride_t AutoStride = std::numeric_limits<stride_t>::min();



/// Pointer to a function called periodically by read_image and
/// write_image.  This can be used to implement progress feedback, etc.
/// It takes an opaque data pointer (passed to read_image/write_image)
/// and a float giving the portion of work done so far.  It returns a
/// bool, which if 'true' will STOP the read or write.
typedef bool (*ProgressCallback)(void *opaque_data, float portion_done);



// Deprecated typedefs. Just use ParamValue and ParamValueList directly.
typedef ParamValue ImageIOParameter;
typedef ParamValueList ImageIOParameterList;



/// ROI is a small helper struct describing a rectangular region of interest
/// in an image. The region is [xbegin,xend) x [begin,yend) x [zbegin,zend),
/// with the "end" designators signifying one past the last pixel in each
/// dimension, a la STL style.
///
struct ROI {
    ///@{
    /// @name ROI data members
    /// The data members are:
    /// 
    ///     int xbegin, xend, ybegin, yend, zbegin, zend;
    ///     int chbegin, chend;
    /// 
    /// These describe the spatial extent
    ///     [xbegin,xend) x [ybegin,yend) x [zbegin,zend)
    /// And the channel extent:
    ///     [chbegin,chend)]
    int xbegin, xend;
    int ybegin, yend;
    int zbegin, zend;
    int chbegin, chend;
    ///@}

    /// Default constructor is an undefined region. Note that this is also
    /// interpreted as All().
    constexpr ROI () noexcept : xbegin(std::numeric_limits<int>::min()), xend(0),
             ybegin(0), yend(0), zbegin(0), zend(0), chbegin(0), chend(0)
    { }

    /// Constructor with an explicitly defined region.
    ///
    constexpr ROI (int xbegin, int xend, int ybegin, int yend,
         int zbegin=0, int zend=1, int chbegin=0, int chend=10000) noexcept
        : xbegin(xbegin), xend(xend), ybegin(ybegin), yend(yend),
          zbegin(zbegin), zend(zend), chbegin(chbegin), chend(chend)
    { }

    /// Is a region defined?
    constexpr bool defined () const noexcept { return (xbegin != std::numeric_limits<int>::min()); }

    ///@{
    /// @name Spatial size functions.
    /// The width, height, and depth of the region.
    constexpr int width () const noexcept { return xend - xbegin; }  ///< Height
    constexpr int height () const noexcept { return yend - ybegin; } ///< Width
    constexpr int depth () const noexcept { return zend - zbegin; }  ///< Depth
    ///@}

    /// Number of channels in the region.  Beware -- this defaults to a
    /// huge number, and to be meaningful you must consider
    /// std::min (imagebuf.nchannels(), roi.nchannels()).
    constexpr int nchannels () const noexcept { return chend - chbegin; }

    /// Total number of pixels in the region.
    constexpr imagesize_t npixels () const noexcept {
        return defined()
            ? imagesize_t(width()) * imagesize_t(height()) * imagesize_t(depth())
            : 0;
    }

    /// All() is an alias for the default constructor, which indicates that
    /// it means "all" of the image, or no region restriction.  For example,
    ///     float myfunc (ImageBuf &buf, ROI roi = ROI::All());
    /// Note that this is equivalent to:
    ///     float myfunc (ImageBuf &buf, ROI roi = {});
    static constexpr ROI All () noexcept { return ROI(); }

    /// Test equality of two ROIs
    friend constexpr bool operator== (const ROI &a, const ROI &b) noexcept {
        return (a.xbegin == b.xbegin && a.xend == b.xend &&
                a.ybegin == b.ybegin && a.yend == b.yend &&
                a.zbegin == b.zbegin && a.zend == b.zend &&
                a.chbegin == b.chbegin && a.chend == b.chend);
    }
    /// Test inequality of two ROIs
    friend constexpr bool operator!= (const ROI &a, const ROI &b) noexcept {
        return (a.xbegin != b.xbegin || a.xend != b.xend ||
                a.ybegin != b.ybegin || a.yend != b.yend ||
                a.zbegin != b.zbegin || a.zend != b.zend ||
                a.chbegin != b.chbegin || a.chend != b.chend);
    }

    /// Test if the coordinate is within the ROI.
    constexpr bool contains (int x, int y, int z=0, int ch=0) const noexcept {
        return x >= xbegin && x < xend && y >= ybegin && y < yend
            && z >= zbegin && z < zend && ch >= chbegin && ch < chend;
    }

    /// Test if another ROI is entirely within our ROI.
    constexpr bool contains (const ROI& other) const noexcept {
        return (other.xbegin >= xbegin && other.xend <= xend &&
                other.ybegin >= ybegin && other.yend <= yend &&
                other.zbegin >= zbegin && other.zend <= zend &&
                other.chbegin >= chbegin && other.chend <= chend);
    }

    /// Stream output of the range
    friend std::ostream & operator<< (std::ostream &out, const ROI &roi) {
        out << roi.xbegin << ' ' << roi.xend << ' ' << roi.ybegin << ' '
            << roi.yend << ' ' << roi.zbegin << ' ' << roi.zend << ' '
            << roi.chbegin << ' ' << roi.chend;
        return out;
    }
};



/// Union of two regions, the smallest region containing both.
inline constexpr ROI roi_union (const ROI &A, const ROI &B) noexcept {
    return (A.defined() && B.defined())
        ? ROI (std::min (A.xbegin,  B.xbegin),  std::max (A.xend,  B.xend),
               std::min (A.ybegin,  B.ybegin),  std::max (A.yend,  B.yend),
               std::min (A.zbegin,  B.zbegin),  std::max (A.zend,  B.zend),
               std::min (A.chbegin, B.chbegin), std::max (A.chend, B.chend))
        : (A.defined() ? A : B);
}

/// Intersection of two regions.
inline constexpr ROI roi_intersection (const ROI &A, const ROI &B) noexcept {
    return (A.defined() && B.defined())
        ? ROI (std::max (A.xbegin,  B.xbegin),  std::min (A.xend,  B.xend),
               std::max (A.ybegin,  B.ybegin),  std::min (A.yend,  B.yend),
               std::max (A.zbegin,  B.zbegin),  std::min (A.zend,  B.zend),
               std::max (A.chbegin, B.chbegin), std::min (A.chend, B.chend))
        : (A.defined() ? A : B);
}




/// ImageSpec describes the data format of an image -- dimensions, layout,
/// number and meanings of image channels.
///
/// The `width, height, depth` are the size of the data of this image, i.e.,
/// the number of pixels in each dimension.  A ``depth`` greater than 1
/// indicates a 3D "volumetric" image. The `x, y, z` fields indicate the
/// *origin* of the pixel data of the image. These default to (0,0,0), but
/// setting them differently may indicate that this image is offset from the
/// usual origin.
/// Therefore the pixel data are defined over pixel coordinates
///    [`x` ... `x+width-1`] horizontally,
///    [`y` ... `y+height-1`] vertically,
///    and [`z` ... `z+depth-1`] in depth.
///
/// The analogous `full_width`, `full_height`, `full_depth` and `full_x`,
/// `full_y`, `full_z` fields define a "full" or "display" image window over
/// the region [`full_x` ... `full_x+full_width-1`] horizontally, [`full_y`
/// ... `full_y+full_height-1`] vertically, and [`full_z`...
/// `full_z+full_depth-1`] in depth.
///
/// Having the full display window different from the pixel data window can
/// be helpful in cases where you want to indicate that your image is a
/// *crop window* of a larger image (if the pixel data window is a subset of
/// the full display window), or that the pixels include *overscan* (if the
/// pixel data is a superset of the full display window), or may simply
/// indicate how different non-overlapping images piece together.
///
/// For tiled images, `tile_width`, `tile_height`, and `tile_depth` specify
/// that the image is stored in a file organized into rectangular *tiles*
/// of these dimensions. The default of 0 value for these fields indicates
/// that the image is stored in scanline order, rather than as tiles.
///

class OIIO_API ImageSpec {
public:
    ///@{
    /// @name ImageSpec data members
    ///
    /// The `ImageSpec` contains data fields for the values that are
    /// required to describe nearly any image, and an extensible list of
    /// arbitrary attributes that can hold metadata that may be user-defined
    /// or specific to individual file formats.
    ///
    /// Here are the hard-coded data fields:

    int x;                    ///< origin (upper left corner) of pixel data
    int y;                    ///< origin (upper left corner) of pixel data
    int z;                    ///< origin (upper left corner) of pixel data
    int width;                ///< width of the pixel data window
    int height;               ///< height of the pixel data window
    int depth;                ///< depth of pixel data, >1 indicates a "volume"
    int full_x;               ///< origin of the full (display) window
    int full_y;               ///< origin of the full (display) window
    int full_z;               ///< origin of the full (display) window
    int full_width;           ///< width of the full (display) window
    int full_height;          ///< height of the full (display) window
    int full_depth;           ///< depth of the full (display) window
    int tile_width;           ///< tile width (0 for a non-tiled image)
    int tile_height;          ///< tile height (0 for a non-tiled image)
    int tile_depth;           ///< tile depth (0 for a non-tiled image,
                              ///<             1 for a non-volume image)
    int nchannels;            ///< number of image channels, e.g., 4 for RGBA

    TypeDesc format;          ///< Data format of the channels.
        ///< Describes the native format of the pixel data values
        /// themselves, as a `TypeDesc`.  Typical values would be
        /// `TypeDesc::UINT8` for 8-bit unsigned values, `TypeDesc::FLOAT`
        /// for 32-bit floating-point values, etc.
    std::vector<TypeDesc> channelformats;
        ///< Optional per-channel data formats. If all channels of the image
        /// have the same data format, that will be described by `format`
        /// and `channelformats` will be empty (zero length). If there are
        /// different data formats for each channel, they will be described
        /// in the `channelformats` vector, and the `format` field will
        /// indicate a single default data format for applications that
        /// don't wish to support per-channel formats (usually this will be
        /// the format of the channel that has the most precision).

    std::vector<std::string> channelnames;
        ///< The names of each channel, in order. Typically this will be "R",
        ///< "G", "B", "A" (alpha), "Z" (depth), or other arbitrary names.
    int alpha_channel;
        ///< The index of the channel that represents *alpha* (pixel
        ///< coverage and/or transparency).  It defaults to -1 if no alpha
        ///< channel is present, or if it is not known which channel
        ///< represents alpha.
    int z_channel;
        ///< The index of the channel that represents *z* or *depth* (from
        ///< the camera).  It defaults to -1 if no depth channel is present,
        ///< or if it is not know which channel represents depth.
    bool deep;                ///< True if the image contains deep data.
        ///< If `true`, this indicates that the image describes contains
        ///< "deep" data consisting of multiple samples per pixel.  If
        ///< `false`, it's an ordinary image with one data value (per
        ///< channel) per pixel.
    ParamValueList extra_attribs;
        ///< A list of arbitrarily-named and arbitrarily-typed additional
        /// attributes of the image, for any metadata not described by the
        /// hard-coded fields described above.  This list may be manipulated
        /// with the `attribute()` and `find_attribute()` methods.

    ///@}

    /// Constructor: given just the data format, set all other fields to
    /// something reasonable.
    ImageSpec (TypeDesc format = TypeDesc::UNKNOWN) noexcept;

    /// Constructs an `ImageSpec` with the given x and y resolution, number
    /// of channels, and pixel data format.
    ///
    /// All other fields are set to the obvious defaults -- the image is an
    /// ordinary 2D image (not a volume), the image is not offset or a crop
    /// of a bigger image, the image is scanline-oriented (not tiled),
    /// channel names are "R", "G", "B"' and "A" (up to and including 4
    /// channels, beyond that they are named "channel *n*"), the fourth
    /// channel (if it exists) is assumed to be alpha.
    ImageSpec (int xres, int yres, int nchans, TypeDesc fmt = TypeUInt8) noexcept;

    /// Construct an `ImageSpec` whose dimensions (both data and "full") and
    /// number of channels are given by the `ROI`, pixel data type by `fmt`,
    /// and other fields are set to their default values.
    explicit ImageSpec (const ROI &roi, TypeDesc fmt = TypeUInt8) noexcept;

    /// Set the data format, and clear any per-channel format information
    /// in `channelformats`.
    void set_format (TypeDesc fmt) noexcept;

    /// Sets the `channelnames` to reasonable defaults for the number of
    /// channels.  Specifically, channel names are set to "R", "G", "B,"
    /// and "A" (up to and including 4 channels, beyond that they are named
    /// "channel*n*".
    void default_channel_names () noexcept;

    /// Returns the number of bytes comprising each channel of each pixel
    /// (i.e., the size of a single value of the type described by the
    /// `format` field).
    size_t channel_bytes() const noexcept { return format.size(); }

    /// Return the number of bytes needed for the single specified
    /// channel.  If native is false (default), compute the size of one
    /// channel of `this->format`, but if native is true, compute the size
    /// of the channel in terms of the "native" data format of that
    /// channel as stored in the file.
    size_t channel_bytes (int chan, bool native=false) const noexcept;

    /// Return the number of bytes for each pixel (counting all channels).
    /// If `native` is false (default), assume all channels are in
    /// `this->format`, but if `native` is true, compute the size of a pixel
    /// in the "native" data format of the file (these may differ in
    /// the case of per-channel formats).
    size_t pixel_bytes (bool native=false) const noexcept;

    /// Return the number of bytes for just the subset of channels in each
    /// pixel described by [chbegin,chend). If native is false (default),
    /// assume all channels are in this->format, but if native is true,
    /// compute the size of a pixel in the "native" data format of the file
    /// (these may differ in the case of per-channel formats).
    size_t pixel_bytes (int chbegin, int chend, bool native=false) const noexcept;

    /// Returns the number of bytes comprising each scanline, i.e.,
    /// `pixel_bytes(native) * width` This will return
    /// `std::numeric_limits<imagesize_t>::max()` in the event of an
    /// overflow where it's not representable in an `imagesize_t`.
    imagesize_t scanline_bytes (bool native=false) const noexcept;

    /// Return the number of pixels comprising a tile (or 0 if it is not a
    /// tiled image).  This will return
    /// `std::numeric_limits<imagesize_t>::max()` in the event of an
    /// overflow where it's not representable in an `imagesize_t`.
    imagesize_t tile_pixels () const noexcept;

    /// Returns the number of bytes comprising an image tile, i.e.,
    ///     `pixel_bytes(native) * tile_width * tile_height * tile_depth`
    /// If native is false (default), assume all channels are in
    /// `this->format`, but if `native` is true, compute the size of a pixel
    /// in the "native" data format of the file (these may differ in the
    /// case of per-channel formats).
    imagesize_t tile_bytes (bool native=false) const noexcept;

    /// Return the number of pixels for an entire image.  This will
    /// return `std::numeric_limits<imagesize_t>::max()` in the event of
    /// an overflow where it's not representable in an `imagesize_t`.
    imagesize_t image_pixels () const noexcept;

    /// Returns the number of bytes comprising an entire image of these
    /// dimensions, i.e.,
    ///     `pixel_bytes(native) * width * height * depth`
    /// This will return `std::numeric_limits<image size_t>::max()` in the
    /// event of an overflow where it's not representable in an
    /// `imagesize_t`. If `native` is false (default), assume all channels
    /// are in `this->format`, but if `native` is true, compute the size of
    /// a pixel in the "native" data format of the file (these may differ in
    /// the case of per-channel formats).
    imagesize_t image_bytes (bool native=false) const noexcept;

    /// Verify that on this platform, a `size_t` is big enough to hold the
    /// number of bytes (and pixels) in a scanline, a tile, and the
    /// whole image.  If this returns false, the image is much too big
    /// to allocate and read all at once, so client apps beware and check
    /// these routines for overflows!
    bool size_t_safe() const noexcept {
        const imagesize_t big = std::numeric_limits<size_t>::max();
        return image_bytes() < big && scanline_bytes() < big &&
            tile_bytes() < big;
    }

    /// Adjust the stride values, if set to AutoStride, to be the right
    /// sizes for contiguous data with the given format, channels,
    /// width, height.
    static void auto_stride (stride_t &xstride, stride_t &ystride,
                             stride_t &zstride, stride_t channelsize,
                             int nchannels, int width, int height) noexcept {
        if (xstride == AutoStride)
            xstride = nchannels * channelsize;
        if (ystride == AutoStride)
            ystride = xstride * width;
        if (zstride == AutoStride)
            zstride = ystride * height;
    }

    /// Adjust the stride values, if set to AutoStride, to be the right
    /// sizes for contiguous data with the given format, channels,
    /// width, height.
    static void auto_stride (stride_t &xstride, stride_t &ystride,
                             stride_t &zstride, TypeDesc format,
                             int nchannels, int width, int height) noexcept {
        auto_stride (xstride, ystride, zstride, format.size(),
                     nchannels, width, height);
    }

    /// Adjust xstride, if set to AutoStride, to be the right size for
    /// contiguous data with the given format and channels.
    static void auto_stride (stride_t &xstride, TypeDesc format, int nchannels) noexcept {
        if (xstride == AutoStride)
            xstride = nchannels * format.size();
    }

    /// Add a metadata attribute to `extra_attribs`, with the given name and
    /// data type. The `value` pointer specifies the address of the data to
    /// be copied.
    void attribute (string_view name, TypeDesc type, const void *value);

    /// Add an `unsigned int` attribute to `extra_attribs`.
    void attribute (string_view name, unsigned int value) {
        attribute (name, TypeDesc::UINT, &value);
    }

    /// Add an `int` attribute to `extra_attribs`.
    void attribute (string_view name, int value) {
        attribute (name, TypeDesc::INT, &value);
    }

    /// Add a `float` attribute to `extra_attribs`.
    void attribute (string_view name, float value) {
        attribute (name, TypeDesc::FLOAT, &value);
    }

    /// Add a string attribute to `extra_attribs`.
    void attribute (string_view name, string_view value) {
        const char *s = value.c_str();
        attribute (name, TypeDesc::STRING, &s);
    }

    /// Parse a string containing a textual representation of a value of
    /// the given `type`, and add that as an attribute to `extra_attribs`.
    /// Example:
    ///
    ///     spec.attribute ("temperature", TypeString, "-273.15");
    ///
    void attribute (string_view name, TypeDesc type, string_view value);

    /// Searches `extra_attribs` for any attributes matching `name` (as a
    /// regular expression), removing them entirely from `extra_attribs`. If
    /// `searchtype` is anything other than `TypeDesc::UNKNOWN`, matches
    /// will be restricted only to attributes with the given type. The name
    /// comparison will be case-sensitive if `casesensitive` is true,
    /// otherwise in a case-insensitive manner.
    void erase_attribute (string_view name,
                          TypeDesc searchtype=TypeDesc::UNKNOWN,
                          bool casesensitive=false);

    /// Searches `extra_attribs` for an attribute matching `name`, returning
    /// a pointer to the attribute record, or NULL if there was no match.
    /// If `searchtype` is anything other than `TypeDesc::UNKNOWN`, matches
    /// will be restricted only to attributes with the given type. The name
    /// comparison will be exact if `casesensitive` is true, otherwise in a
    /// case-insensitive manner if `caseinsensitive` is false.
    ParamValue * find_attribute (string_view name,
                                 TypeDesc searchtype=TypeDesc::UNKNOWN,
                                 bool casesensitive=false);
    const ParamValue *find_attribute (string_view name,
                                      TypeDesc searchtype=TypeDesc::UNKNOWN,
                                      bool casesensitive=false) const;

    /// Search for the named attribute and return the pointer to its
    /// `ParamValue` record, or NULL if not found.  This variety of
    /// `find_attribute(}` can retrieve items such as "width", which are
    /// data members of the `ImageSpec`, but not in `extra_attribs`. The
    /// `tmpparam` is a storage area owned by the caller, which is used as
    /// temporary buffer in cases where the information does not correspond
    /// to an actual `extra_attribs` (in this case, the return value will be
    /// `&tmpparam`). The extra names it understands are:
    ///
    /// - `"x"` `"y"` `"z"` `"width"` `"height"` `"depth"`
    ///   `"full_x"` `"full_y"` `"full_z"` `"full_width"` `"full_height"` `"full_depth"`
    ///
    ///   Returns the `ImageSpec` fields of those names (despite the
    ///   fact that they are technically not arbitrary named attributes
    ///   in `extra_attribs`). All are of type `int`.
    ///
    /// - `"datawindow"`
    ///
    ///   Without a type, or if requested explicitly as an `int[4]`,
    ///   returns the OpenEXR-like pixel data min and max coordinates,
    ///   as a 4-element integer array: `{ x, y, x+width-1, y+height-1
    ///   }`. If instead you specifically request as an `int[6]`, it
    ///   will return the volumetric data window, `{ x, y, z, x+width-1,
    ///   y+height-1, z+depth-1 }`.
    ///
    /// - `"displaywindow"`
    ///
    ///   Without a type, or if requested explicitly as an `int[4]`,
    ///   returns the OpenEXR-like pixel display min and max
    ///   coordinates, as a 4-element integer array: `{ full_x, full_y,
    ///   full_x+full_width-1, full_y+full_height-1 }`. If instead you
    ///   specifically request as an `int[6]`, it will return the
    ///   volumetric display window, `{ full_x, full_y, full_z,
    ///   full_x+full_width-1, full_y+full_height-1, full_z+full_depth-1 }`.
    ///
    /// EXAMPLES
    ///
    ///     ImageSpec spec;           // has the info
    ///     Imath::Box2i dw;          // we want the displaywindow here
    ///     ParamValue tmp;           // so we can retrieve pseudo-values
    ///     TypeDesc int4("int[4]");  // Equivalent: TypeDesc int4(TypeDesc::INT,4);
    ///     const ParamValue* p = spec.find_attribute ("displaywindow", int4);
    ///     if (p)
    ///         dw = Imath::Box2i(p->get<int>(0), p->get<int>(1),
    ///                           p->get<int>(2), p->get<int>(3));
    ///
    ///     p = spec.find_attribute("temperature", TypeFloat);
    ///     if (p)
    ///         float temperature = p->get<float>();
    ///
    const ParamValue * find_attribute (string_view name,
                         ParamValue &tmpparam,
                         TypeDesc searchtype=TypeDesc::UNKNOWN,
                         bool casesensitive=false) const;

    /// If the named attribute can be found in the `ImageSpec`, return its
    /// data type. If no such attribute exists, return `TypeUnknown`.
    ///
    /// This was added in version 2.1.
    TypeDesc getattributetype (string_view name,
                               bool casesensitive = false) const;

    /// If the `ImageSpec` contains the named attribute and its type matches
    /// `type`, copy the attribute value into the memory pointed to by `val`
    /// (it is up to the caller to ensure there is enough space) and return
    /// `true`. If no such attribute is found, or if it doesn't match the
    /// type, return `false` and do not modify `val`.
    /// 
    /// EXAMPLES:
    /// 
    ///     ImageSpec spec;
    ///     ...
    ///     // Retrieving an integer attribute:
    ///     int orientation = 0;
    ///     spec.getattribute ("orientation", TypeInt, &orientation);
    /// 
    ///     // Retrieving a string attribute with a char*:
    ///     const char* compression = nullptr;
    ///     spec.getattribute ("compression", TypeString, &compression);
    /// 
    ///     // Alternately, retrieving a string with a ustring:
    ///     ustring compression;
    ///     spec.getattribute ("compression", TypeString, &compression);
    /// 
    /// Note that when passing a string, you need to pass a pointer to the
    /// `char*`, not a pointer to the first character.  Also, the `char*`
    /// will end up pointing to characters owned by the `ImageSpec`; the
    /// caller does not need to ever free the memory that contains the
    /// characters.
    ///
    /// This was added in version 2.1.
    bool getattribute (string_view name, TypeDesc type, void* value,
                       bool casesensitive = false) const;

    /// Retrieve the named metadata attribute and return its value as an
    /// `int`. Any integer type will convert to `int` by truncation or
    /// expansion, string data will parsed into an `int` if its contents
    /// consist of of the text representation of one integer. Floating point
    /// data will not succeed in converting to an `int`. If no such metadata
    /// exists, or are of a type that cannot be converted, the `defaultval`
    /// will be returned.
    int get_int_attribute (string_view name, int defaultval=0) const;

    /// Retrieve the named metadata attribute and return its value as a
    /// `float`. Any integer or floating point type will convert to `float`
    /// in the obvious way (like a C cast), and so will string metadata if
    /// its contents consist of of the text representation of one floating
    /// point value. If no such metadata exists, or are of a type that cannot
    /// be converted, the `defaultval` will be returned.
    float get_float_attribute (string_view name, float defaultval=0) const;

    /// Retrieve any metadata attribute, converted to a string.
    /// If no such metadata exists, the `defaultval` will be returned.
    string_view get_string_attribute (string_view name,
                           string_view defaultval = string_view()) const;

    /// For a given parameter `p`, format the value nicely as a string.  If
    /// `human` is true, use especially human-readable explanations (units,
    /// or decoding of values) for certain known metadata.
    static std::string metadata_val (const ParamValue &p, bool human=false);

    enum SerialFormat  { SerialText, SerialXML };
    enum SerialVerbose { SerialBrief, SerialDetailed, SerialDetailedHuman };

    /// Returns, as a string, a serialized version of the `ImageSpec`. The
    /// `format` may be either `ImageSpec::SerialText` or
    /// `ImageSpec::SerialXML`. The `verbose` argument may be one of:
    /// `ImageSpec::SerialBrief` (just resolution and other vital
    /// statistics, one line for `SerialText`, `ImageSpec::SerialDetailed`
    /// (contains all metadata in orginal form), or
    /// `ImageSpec::SerialDetailedHuman` (contains all metadata, in many
    /// cases with human-readable explanation).
    std::string serialize (SerialFormat format,
                           SerialVerbose verbose = SerialDetailed) const;

    /// Converts the contents of the `ImageSpec` as an XML string.
    std::string to_xml () const;

    /// Populates the fields of the `ImageSpec` based on the XML passed in.
    void from_xml (const char *xml);

    /// Hunt for the "Compression" and "CompressionQuality" settings in the
    /// spec and turn them into the compression name and quality. This
    /// handles compression name/qual combos of the form "name:quality".
    std::pair<string_view, int>
    decode_compression_metadata(string_view defaultcomp = "",
                                int defaultqual = -1) const;

    /// Helper function to verify that the given pixel range exactly covers
    /// a set of tiles.  Also returns false if the spec indicates that the
    /// image isn't tiled at all.
    bool valid_tile_range (int xbegin, int xend, int ybegin, int yend,
                           int zbegin, int zend) noexcept {
        return (tile_width &&
                ((xbegin-x) % tile_width)  == 0 &&
                ((ybegin-y) % tile_height) == 0 &&
                ((zbegin-z) % tile_depth)  == 0 &&
                (((xend-x) % tile_width)  == 0 || (xend-x) == width) &&
                (((yend-y) % tile_height) == 0 || (yend-y) == height) &&
                (((zend-z) % tile_depth)  == 0 || (zend-z) == depth));
    }

    /// Return the channelformat of the given channel. This is safe even
    /// if channelformats is not filled out.
    TypeDesc channelformat (int chan) const {
        return chan >= 0 && chan < (int)channelformats.size()
            ? channelformats[chan] : format;
    }

    /// Return the channel name of the given channel. This is safe even if
    /// channelnames is not filled out.
    string_view channel_name (int chan) const {
        return chan >= 0 && chan < (int)channelnames.size()
            ? string_view(channelnames[chan]) : "";
    }

    /// Fill in an array of channel formats describing all channels in
    /// the image.  (Note that this differs slightly from the member
    /// data channelformats, which is empty if there are not separate
    /// per-channel formats.)
    void get_channelformats (std::vector<TypeDesc> &formats) const {
        formats = channelformats;
        if ((int)formats.size() < nchannels)
            formats.resize (nchannels, format);
    }

    /// Return the index of the channel with the given name, or -1 if no
    /// such channel is present in `channelnames`.
    int channelindex (string_view name) const;

    /// Return pixel data window for this ImageSpec expressed as a ROI.
    ROI roi () const noexcept {
        return ROI (x, x+width, y, y+height, z, z+depth, 0, nchannels);
    }

    /// Return full/display window for this ImageSpec expressed as a ROI.
    ROI roi_full () const noexcept {
        return ROI (full_x, full_x+full_width, full_y, full_y+full_height,
                    full_z, full_z+full_depth, 0, nchannels);
    }

    /// Set pixel data window parameters (x, y, z, width, height, depth)
    /// for this ImageSpec from an ROI.
    /// Does NOT change the channels of the spec, regardless of r.
    void set_roi (const ROI &r) noexcept {
        x = r.xbegin;
        y = r.ybegin;
        z = r.zbegin;
        width = r.width();
        height = r.height();
        depth = r.depth();
    }

    /// Set full/display window parameters (full_x, full_y, full_z,
    /// full_width, full_height, full_depth) for this ImageSpec from an ROI.
    /// Does NOT change the channels of the spec, regardless of r.
    void set_roi_full (const ROI &r) noexcept {
        full_x = r.xbegin;
        full_y = r.ybegin;
        full_z = r.zbegin;
        full_width = r.width();
        full_height = r.height();
        full_depth = r.depth();
    }

    /// Copy from `other` the image dimensions (x, y, z, width, height,
    /// depth, full*, nchannels, format) and data types. It does *not* copy
    /// arbitrary named metadata or channel names (thus, for an `ImageSpec`
    /// with lots of metadata, it is much less expensive than copying the
    /// whole thing with `operator=()`).
    void copy_dimensions (const ImageSpec &other) {
        x = other.x;
        y = other.y;
        z = other.z;
        width = other.width;
        height = other.height;
        depth = other.depth;
        full_x = other.full_x;
        full_y = other.full_y;
        full_z = other.full_z;
        full_width = other.full_width;
        full_height = other.full_height;
        full_depth = other.full_depth;
        tile_width = other.tile_width;
        tile_height = other.tile_height;
        tile_depth = other.tile_depth;
        nchannels = other.nchannels;
        format = other.format;
        channelformats = other.channelformats;
        alpha_channel = other.alpha_channel;
        z_channel = other.z_channel;
        deep = other.deep;
    }

    /// Returns `true` for a newly initialized (undefined) `ImageSpec`.
    /// (Designated by no channels and undefined data type -- true of the
    /// uninitialized state of an ImageSpec, and presumably not for any
    /// ImageSpec that is useful or purposefully made.)
    bool undefined () const noexcept {
        return nchannels == 0 && format == TypeUnknown;
    }

    /// Array indexing by string will create an AttrDelegate that enables a
    /// convenient shorthand for adding and retrieving values from the spec:
    ///
    /// 1. Assigning to the delegate adds a metadata attribute:
    ///
    ///        ImageSpec spec;
    ///        spec["foo"] = 42;                   // int
    ///        spec["pi"] = float(M_PI);           // float
    ///        spec["oiio:ColorSpace"] = "sRGB";   // string
    ///        spec["cameratoworld"] = Imath::Matrix44(...);  // matrix
    ///
    ///    Be very careful, the attribute's type will be implied by the C++
    ///    type of what you assign.
    ///
    /// 2. String data may be retrieved directly, and for other types, the
    ///    delegate supports a get<T>() that retrieves an item of type T:
    ///
    ///         std::string colorspace = spec["oiio:ColorSpace"];
    ///         int dither = spec["oiio:dither"].get<int>();
    ///
    /// This was added in version 2.1.
    AttrDelegate<ImageSpec> operator[](string_view name)
    {
        return { this, name };
    }
    AttrDelegate<const ImageSpec> operator[](string_view name) const
    {
        return { this, name };
    }
};




/// ImageInput abstracts the reading of an image file in a file
/// format-agnostic manner.
class OIIO_API ImageInput {
public:
    /// unique_ptr to an ImageInput
    using unique_ptr = std::unique_ptr<ImageInput>;

    /// @{
    /// @name Creating an ImageIntput

    /// Create an ImageInput subclass instance that is able to read the
    /// given file and open it, returning a `unique_ptr` to the ImageInput
    /// if successful.  The `unique_ptr` is set up with an appropriate
    /// deleter so the ImageInput will be properly closed and deleted when
    /// the `unique_ptr` goes out of scope or is reset. If the open fails,
    /// return an empty `unique_ptr` and set an error that can be retrieved
    /// by `OIIO::geterror()`.
    ///
    /// The `config`, if not nullptr, points to an ImageSpec giving hints,
    /// requests, or special instructions.  ImageInput implementations are
    /// free to not respond to any such requests, so the default
    /// implementation is just to ignore `config`.
    ///
    /// `open()` will first try to make an ImageInput corresponding to
    /// the format implied by the file extension (for example, `"foo.tif"`
    /// will try the TIFF plugin), but if one is not found or if the
    /// inferred one does not open the file, every known ImageInput type
    /// will be tried until one is found that will open the file.
    ///
    /// @param filename
    ///         The name of the file to open.
    ///
    /// @param config
    ///         Optional pointer to an ImageSpec whose metadata contains
    ///         "configuration hints."
    ///
    /// @returns
    ///         A `unique_ptr` that will close and free the ImageInput when
    ///         it exits scope or is reset. The pointer will be empty if the
    ///         required writer was not able to be created.
    static unique_ptr open (const std::string& filename,
                            const ImageSpec *config = nullptr);

    /// Create and return an ImageInput implementation that is able to read
    /// the given file.  If `do_open` is true, fully open it if possible
    /// (using the optional `config` configuration spec, if supplied),
    /// otherwise just create the ImageInput but don't open it. The
    /// plugin_searchpath parameter is an override of the searchpath.
    /// colon-separated list of directories to search for ImageIO plugin
    /// DSO/DLL's (not a searchpath for the image itself!).  This will
    /// actually just try every imageio plugin it can locate, until it finds
    /// one that's able to open the file without error.
    ///
    /// If the caller intends to immediately open the file, then it is often
    /// simpler to call static `ImageInput::open()`.
    ///
    /// @param filename
    ///         The name of the file to open.
    ///
    /// @param do_open
    ///         If `true`, not only create but also open the file.
    ///
    /// @param config
    ///         Optional pointer to an ImageSpec whose metadata contains
    ///         "configuration hints" for the ImageInput implementation.
    ///
    /// @param plugin_searchpath
    ///                 An optional colon-separated list of directories to
    ///                 search for OpenImageIO plugin DSO/DLL's.
    ///
    /// @returns
    ///         A `unique_ptr` that will close and free the ImageInput when
    ///         it exits scope or is reset. The pointer will be empty if the
    ///         required writer was not able to be created.
    static unique_ptr create (const std::string& filename, bool do_open=false,
                              const ImageSpec *config=nullptr,
                              string_view plugin_searchpath="");

    // DEPRECATED(2.1) This method should no longer be used, it is redundant.
    static unique_ptr create (const std::string& filename,
                              const std::string& plugin_searchpath);

    /// @}

    // DEPRECATED(2.1)
    static void destroy (ImageInput *x);

protected:
    ImageInput ();
public:
    virtual ~ImageInput ();

    typedef std::recursive_mutex mutex;
    typedef std::lock_guard<mutex> lock_guard;

    /// Return the name of the format implemented by this class.
    virtual const char *format_name (void) const = 0;

    /// Given the name of a "feature", return whether this ImageInput
    /// supports output of images with the given properties. Most queries
    /// will simply return 0 for "doesn't support" and 1 for "supports it,"
    /// but it is acceptable to have queries return other nonzero integers
    /// to indicate varying degrees of support or limits (but should be
    /// clearly documented as such).
    ///
    /// Feature names that ImageInput implementations are expected to
    /// recognize include:
    ///
    /// - `"arbitrary_metadata"` : Does this format allow metadata with
    ///       arbitrary names and types?
    ///
    /// - `"exif"` :
    ///       Can this format store Exif camera data?
    ///
    /// - `"iptc"` :
    ///       Can this format store IPTC data?
    ///
    /// - `"procedural"` :
    ///       Can this format create images without reading from a disk
    ///       file?
    ///
    /// - `"ioproxy"` :
    ///       Does this format reader support reading from an `IOProxy`?
    ///
    /// This list of queries may be extended in future releases. Since this
    /// can be done simply by recognizing new query strings, and does not
    /// require any new API entry points, addition of support for new
    /// queries does not break ``link compatibility'' with
    /// previously-compiled plugins.
    virtual int supports (string_view feature) const { return false; }

    /// Return true if the `filename` names a file of the type for this
    /// ImageInput.  The implementation will try to determine this as
    /// efficiently as possible, in most cases much less expensively than
    /// doing a full `open()`.  Note that there can be false positives: a
    /// file can appear to be of the right type (i.e., `valid_file()`
    /// returning `true`) but still fail a subsequent call to `open()`, such
    /// as if the contents of the file are truncated, nonsensical, or
    /// otherwise corrupted.
    ///
    /// @returns
    ///         `true` upon success, or `false` upon failure.
    virtual bool valid_file (const std::string& filename) const;

    /// Opens the file with given name and seek to the first subimage in the
    /// file.  Various file attributes are put in `newspec` and a copy
    /// is also saved internally to the `ImageInput` (retrievable via
    /// `spec()`.  From examining `newspec` or `spec()`, you can
    /// discern the resolution, if it's tiled, number of channels, native
    /// data format, and other metadata about the image.
    ///
    /// @param name
    ///         Filename to open.
    ///
    /// @param newspec
    ///         Reference to an ImageSpec in which to deposit a full
    ///         description of the contents of the first subimage of the
    ///         file.
    ///
    /// @returns
    ///         `true` if the file was found and opened successfully.
    virtual bool open (const std::string& name, ImageSpec &newspec) = 0;

    /// Open file with given name, similar to `open(name,newspec)`. The
    /// `config` is an ImageSpec giving requests or special instructions.
    /// ImageInput implementations are free to not respond to any such
    /// requests, so the default implementation is just to ignore config and
    /// call regular `open(name,newspec)`.
    ///
    /// @param name
    ///         Filename to open.
    ///
    /// @param newspec
    ///         Reference to an ImageSpec in which to deposit a full
    ///         description of the contents of the first subimage of the
    ///         file.
    ///
    /// @param config
    ///         An ImageSpec whose metadata contains "configuration hints"
    ///         for the ImageInput implementation.
    ///
    /// @returns
    ///         `true` if the file was found and opened successfully.
    virtual bool open (const std::string& name, ImageSpec &newspec,
                       const ImageSpec& config) { return open(name,newspec); }

    /// Return a reference to the image specification of the current
    /// subimage/MIPlevel.  Note that the contents of the spec are invalid
    /// before `open()` or after `close()`, and may change with a call to
    /// `seek_subimage()`. It is thus not thread-safe, since the spec may
    /// change if another thread calls `seek_subimage`, or any of the
    /// `read_*()` functions that take explicit subimage/miplevel.
    virtual const ImageSpec &spec (void) const { return m_spec; }

    /// Return a full copy of the ImageSpec of the designated subimage and
    /// MIPlevel. This method is thread-safe, but it is potentially
    /// expensive, due to the work that needs to be done to fully copy an
    /// ImageSpec if there is lots of named metadata to allocate and copy.
    /// See also the less expensive `spec_dimensions()`. Errors (such as
    /// having requested a nonexistent subimage) are indicated by returning
    /// an ImageSpec with `format==TypeUnknown`.
    virtual ImageSpec spec (int subimage, int miplevel=0);

    /// Return a copy of the ImageSpec of the designated subimage and
    /// miplevel, but only the dimension and type fields. Just as with a
    /// call to `ImageSpec::copy_dimensions()`, neither the channel names
    /// nor any of the arbitrary named metadata will be copied, thus this is
    /// a relatively inexpensive operation if you don't need that
    /// information. It is guaranteed to be thread-safe. Errors (such as
    /// having requested a nonexistent subimage) are indicated by returning
    /// an ImageSpec with `format==TypeUnknown`.
    virtual ImageSpec spec_dimensions (int subimage, int miplevel=0);

    /// Close an open ImageInput. The call to close() is not strictly
    /// necessary if the ImageInput is destroyed immediately afterwards,
    /// since it is required for the destructor to close if the file is
    /// still open.
    ///
    /// @returns
    ///         `true` upon success, or `false` upon failure.
    virtual bool close () = 0;

    /// Returns the index of the subimage that is currently being read.
    /// The first subimage (or the only subimage, if there is just one)
    /// is number 0.
    virtual int current_subimage (void) const { return 0; }

    /// Returns the index of the MIPmap image that is currently being read.
    /// The highest-res MIP level (or the only level, if there is just
    /// one) is number 0.
    virtual int current_miplevel (void) const { return 0; }

    /// Seek to the given subimage and MIP-map level within the open image
    /// file.  The first subimage of the file has index 0, the highest-
    /// resolution MIP level has index 0.  The new subimage's vital
    /// statistics=may be retrieved by `this->spec()`.  The reader is
    /// expected to give the appearance of random access to subimages and
    /// MIP levels -- in other words, if it can't randomly seek to the given
    /// subimage/level, it should transparently close, reopen, and
    /// sequentially read through prior subimages and levels.
    ///
    /// @returns
    ///         `true` upon success, or `false` upon failure. A failure may
    ///         indicate that no such subimage or MIP level exists in the
    ///         file.
    virtual bool seek_subimage (int subimage, int miplevel) {
        // Default implementation assumes no support for subimages or
        // mipmaps, so there is no work to do.
        return subimage == current_subimage() && miplevel == current_miplevel();
    }

    // Old version for backwards-compatibility: pass reference to newspec.
    // Some day this will be deprecated.
    bool seek_subimage (int subimage, int miplevel, ImageSpec &newspec) {
        bool ok = seek_subimage (subimage, miplevel);
        if (ok)
            newspec = spec();
        return ok;
    }

    // DEPRECATED(2.1)
    // Seek to the given subimage -- backwards-compatible call that
    // doesn't worry about MIP-map levels at all.
    bool seek_subimage (int subimage, ImageSpec &newspec) {
        return seek_subimage (subimage, 0 /* miplevel */, newspec);
    }

    /// @{
    /// @name Reading pixels
    ///
    /// Common features of all the `read` methods:
    ///
    /// * The `format` parameter describes the data type of the `data[]`
    ///   buffer. The read methods automatically convert the data from the
    ///   data type it is stored in the file into the `format` of the `data`
    ///   buffer.  If `format` is `TypeUnknown` it will just copy pixels of
    ///   file's native data layout (including, possibly, per-channel data
    ///   formats as specified by the ImageSpec's `channelfomats` field).
    ///
    /// * The `stride` values describe the layout of the `data` buffer:
    ///   `xstride` is the distance in bytes between successive pixels
    ///   within each scanline. `ystride` is the distance in bytes between
    ///   successive scanlines. For volumetric images `zstride` is the
    ///   distance in bytes between successive "volumetric planes".  Strides
    ///   set to the special value `AutoStride` imply contiguous data, i.e.,
    ///
    ///       xstride = format.size() * nchannels
    ///       ystride = xstride * width
    ///       zstride = ystride * height
    ///
    /// * Any *range* parameters (such as `ybegin` and `yend`) describe a
    ///   "half open interval", meaning that `begin` is the first item and
    ///   `end` is *one past the last item*. That means that the number of
    ///   items is `end - begin`.
    ///
    /// * For ordinary 2D (non-volumetric) imaages, any `z` or `zbegin`
    ///   coordinates should be 0 and any `zend` should be 1, indicating
    ///   that only a single image "plane" exists.
    ///
    /// * Some read methods take a channel range [chbegin,chend) to allow
    ///   reading of a contiguous subset of channels (chbegin=0,
    ///   chend=spec.nchannels reads all channels).
    ///
    /// * ImageInput readers are expected to give the appearance of random
    ///   access -- in other words, if it can't randomly seek to the given
    ///   scanline or tile, it should transparently close, reopen, and
    ///   sequentially read through prior scanlines.
    ///
    /// * All read functions return `true` for success, `false` for failure
    ///   (after which a call to `geterror()` may retrieve a specific error
    ///   message).
    ///

    /// Read the scanline that includes pixels (*,y,z) from the "current"
    /// subimage and MIP level.  The `xstride` value gives the distance
    /// between successive pixels (in bytes).  Strides set to `AutoStride`
    /// imply "contiguous" data.
    ///
    /// @note This variety of `read_scanline` is not re-entrant nor
    /// thread-safe. If you require concurrent reads to the same open
    /// ImageInput, you should use `read_scanlines` that has the `subimage`
    /// and `miplevel` passed explicitly.
    ///
    /// @param  y/z         The y & z coordinates of the scanline. For 2D
    ///                     images, z should be 0.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data buffer.
    /// @param  xstride     The distance in bytes between successive
    ///                     pixels in `data` (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_scanline (int y, int z, TypeDesc format, void *data,
                                stride_t xstride=AutoStride);

    /// Simple read_scanline reads into contiguous float pixels.
    bool read_scanline (int y, int z, float *data) {
        return read_scanline (y, z, TypeFloat, data);
    }

    /// Read multiple scanlines that include pixels (*,y,z) for all ybegin
    /// <= y < yend in the specified subimage and mip level, into `data`,
    /// using the strides given and converting to the requested data
    /// `format` (TypeUnknown indicates no conversion, just copy native data
    /// types). Only channels [chbegin,chend) will be read/copied
    /// (chbegin=0, chend=spec.nchannels reads all channels, yielding
    /// equivalent behavior to the simpler variant of `read_scanlines`).
    ///
    /// This version of read_scanlines, because it passes explicit
    /// subimage/miplevel, does not require a separate call to
    /// seek_subimage, and is guaranteed to be thread-safe against other
    /// concurrent calls to any of the read_* methods that take an explicit
    /// subimage/miplevel (but not against any other ImageInput methods).
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride
    ///                     The distance in bytes between successive pixels
    ///                     and scanlines (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note This call was changed for OpenImageIO 2.0 to include the
    ///     explicit subimage and miplevel parameters. The previous
    ///     versions, which lacked subimage and miplevel parameters (thus
    ///     were dependent on a prior call to `seek_subimage`) are
    ///     considered deprecated.
    virtual bool read_scanlines (int subimage, int miplevel,
                                 int ybegin, int yend, int z,
                                 int chbegin, int chend,
                                 TypeDesc format, void *data,
                                 stride_t xstride=AutoStride,
                                 stride_t ystride=AutoStride);

    // DEPRECATED versions of read_scanlines (pre-1.9 OIIO). These will
    // eventually be removed. Try to replace these calls with ones to the
    // new variety of read_scanlines that takes an explicit subimage and
    // miplevel. These old versions are NOT THREAD-SAFE.
    bool read_scanlines (int ybegin, int yend, int z,
                         TypeDesc format, void *data,
                         stride_t xstride=AutoStride,
                         stride_t ystride=AutoStride);
    bool read_scanlines (int ybegin, int yend, int z,
                         int chbegin, int chend,
                         TypeDesc format, void *data,
                         stride_t xstride=AutoStride,
                         stride_t ystride=AutoStride);

    /// Read the tile whose upper-left origin is (x,y,z) into `data[]`,
    /// converting if necessary from the native data format of the file into
    /// the `format` specified. The stride values give the data spacing of
    /// adjacent pixels, scanlines, and volumetric slices (measured in
    /// bytes). Strides set to AutoStride imply 'contiguous' data in the
    /// shape of a full tile, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * spec.tile_width
    ///     zstride = ystride * spec.tile_height
    ///
    /// @note This variety of `read_tile` is not re-entrant nor thread-safe.
    /// If you require concurrent reads to the same open ImageInput, you
    /// should use `read_tiles()` that has the `subimage` and `miplevel`
    /// passed explicitly.
    ///
    /// @param  x/y/z       The upper left coordinate of the tile being passed.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride` to
    ///                     indicate a "contiguous" single tile).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note This call will fail if the image is not tiled, or if (x,y,z)
    /// is not the upper left corner coordinates of a tile.
    virtual bool read_tile (int x, int y, int z, TypeDesc format,
                            void *data, stride_t xstride=AutoStride,
                            stride_t ystride=AutoStride,
                            stride_t zstride=AutoStride);

    /// Simple read_tile reads into contiguous float pixels.
    bool read_tile (int x, int y, int z, float *data) {
        return read_tile (x, y, z, TypeDesc::FLOAT, data,
                          AutoStride, AutoStride, AutoStride);
    }

    /// Read the block of multiple tiles that include all pixels in
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// This is analogous to calling `read_tile(x,y,z,...)` for each tile
    /// in turn (but for some file formats, reading multiple tiles may allow
    /// it to read more efficiently or in parallel).
    ///
    /// The begin/end pairs must correctly delineate tile boundaries, with
    /// the exception that it may also be the end of the image data if the
    /// image resolution is not a whole multiple of the tile size. The
    /// stride values give the data spacing of adjacent pixels, scanlines,
    /// and volumetric slices (measured in bytes). Strides set to AutoStride
    /// imply contiguous data in the shape of the [begin,end) region, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * (xend-xbegin)
    ///     zstride = ystride * (yend-ybegin)
    ///
    /// This version of read_tiles, because it passes explicit subimage and
    /// miplevel, does not require a separate call to seek_subimage, and is
    /// guaranteed to be thread-safe against other concurrent calls to any
    /// of the read_* methods that take an explicit subimage/miplevel (but
    /// not against any other ImageInput methods).
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles being read.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool read_tiles (int subimage, int miplevel, int xbegin, int xend,
                             int ybegin, int yend, int zbegin, int zend,
                             int chbegin, int chend, TypeDesc format, void *data,
                             stride_t xstride=AutoStride, stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride);

    // DEPRECATED versions of read_tiles (pre-1.9 OIIO). These will
    // eventually be removed. Try to replace these calls with ones to the
    // new variety of read_tiles that takes an explicit subimage and
    // miplevel. These old versions are NOT THREAD-SAFE.
    bool read_tiles (int xbegin, int xend, int ybegin, int yend,
                     int zbegin, int zend, TypeDesc format, void *data,
                     stride_t xstride=AutoStride, stride_t ystride=AutoStride,
                     stride_t zstride=AutoStride);
    bool read_tiles (int xbegin, int xend, int ybegin, int yend,
                     int zbegin, int zend, int chbegin, int chend,
                     TypeDesc format, void *data, stride_t xstride=AutoStride,
                     stride_t ystride=AutoStride, stride_t zstride=AutoStride);

    /// Read the entire image of `spec.width x spec.height x spec.depth`
    /// pixels into a buffer with the given strides and in the desired
    /// data format.
    ///
    /// Depending on the spec, this will read either all tiles or all
    /// scanlines. Assume that data points to a layout in row-major order.
    ///
    /// This version of read_image, because it passes explicit subimage and
    /// miplevel, does not require a separate call to seek_subimage, and is
    /// guaranteed to be thread-safe against other concurrent calls to any
    /// of the read_* methods that take an explicit subimage/miplevel (but
    /// not against any other ImageInput methods).
    ///
    /// Because this may be an expensive operation, a progress callback
    /// may be passed.  Periodically, it will be called as follows:
    ///
    ///     progress_callback (progress_callback_data, float done);
    ///
    /// where `done` gives the portion of the image (between 0.0 and 1.0)
    /// that has been written thus far.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @param  progress_callback/progress_callback_data
    ///                     Optional progress callback.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_image (int subimage, int miplevel,
                             int chbegin, int chend,
                             TypeDesc format, void *data,
                             stride_t xstride=AutoStride,
                             stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride,
                             ProgressCallback progress_callback=NULL,
                             void *progress_callback_data=NULL);

    // DEPRECATED versions of read_image (pre-1.9 OIIO). These will
    // eventually be removed. Try to replace these calls with ones to the
    // new variety of read_image that takes an explicit subimage and
    // miplevel. These old versions are NOT THREAD-SAFE.
    virtual bool read_image (TypeDesc format, void *data,
                             stride_t xstride=AutoStride,
                             stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride,
                             ProgressCallback progress_callback=NULL,
                             void *progress_callback_data=NULL);
    virtual bool read_image (int chbegin, int chend,
                             TypeDesc format, void *data,
                             stride_t xstride=AutoStride,
                             stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride,
                             ProgressCallback progress_callback=NULL,
                             void *progress_callback_data=NULL);
    bool read_image (float *data) {
        return read_image (TypeDesc::FLOAT, data);
    }

    /// Read deep scanlines containing pixels (*,y,z), for all y in the
    /// range [ybegin,yend) into `deepdata`. This will fail if it is not a
    /// deep file.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  deepdata    A `DeepData` object into which the data for
    ///                     these scanlines will be placed.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_native_deep_scanlines (int subimage, int miplevel,
                                             int ybegin, int yend, int z,
                                             int chbegin, int chend,
                                             DeepData &deepdata);

    /// Read into `deepdata` the block of native deep data tiles that
    /// include all pixels and channels specified by pixel range.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles being read.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  chbegin/chend
    ///                     The channel range to read.
    /// @param  deepdata    A `DeepData` object into which the data for
    ///                     these tiles will be placed.
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool read_native_deep_tiles (int subimage, int miplevel,
                                         int xbegin, int xend,
                                         int ybegin, int yend,
                                         int zbegin, int zend,
                                         int chbegin, int chend,
                                         DeepData &deepdata);

    /// Read the entire deep data image of spec.width x spec.height x
    /// spec.depth pixels, all channels, into `deepdata`.
    ///
    /// @param  subimage    The subimage to read from (starting with 0).
    /// @param  miplevel    The MIP level to read (0 is the highest
    ///                     resolution level).
    /// @param  deepdata    A `DeepData` object into which the data for
    ///                     the image will be placed.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool read_native_deep_image (int subimage, int miplevel,
                                         DeepData &deepdata);

    // DEPRECATED(1.9), Now just used for back compatibility:
    bool read_native_deep_scanlines (int ybegin, int yend, int z,
                             int chbegin, int chend, DeepData &deepdata) {
        return read_native_deep_scanlines (current_subimage(), current_miplevel(),
                                           ybegin, yend, z,
                                           chbegin, chend, deepdata);
    }
    bool read_native_deep_tiles (int xbegin, int xend, int ybegin, int yend,
                                 int zbegin, int zend, int chbegin, int chend,
                                 DeepData &deepdata) {
        return read_native_deep_tiles (current_subimage(), current_miplevel(),
                                       xbegin, xend, ybegin, yend,
                                       zbegin, zend, chbegin, chend, deepdata);
    }
    bool read_native_deep_image (DeepData &deepdata) {
        return read_native_deep_image (current_subimage(), current_miplevel(),
                                       deepdata);
    }

    /// @}

    /// @{
    /// @name Reading native pixels -- implementation overloads
    ///
    /// @note read_native_* methods are usually not directly called by user
    ///     code (except for read_native_deep_* varieties). These are the
    ///     methods that are overloaded by the ImageInput subclasses that
    ///     implement the individual file format readers.
    //
    // The read_native_* methods always read the "native" data types
    // (including per-channel data types) and assume that `data` points to
    // contiguous memory (no non-default strides). In contrast, the
    // read_scanline/scanlines/tile/tiles handle data type translation and
    // arbitrary strides.
    //
    // The read_native_* methods take an explicit subimage and miplevel, and
    // thus do not require a prior call to seek_subimage (and therefore no
    // saved state). They are all required to be thread-safe when called
    // concurrently with any other read_native_* call or with the varieties
    // of read_tiles() that also takes an explicit subimage and miplevel
    // parameter.
    //
    // As far as format-reading ImageInput subclasses are concerned, the
    // only truly required overloads are read_native_scanline (always) and
    // read_native_tile (only for formats that support tiles). The other
    // varieties are special cases, for example if the particular format is
    // able to efficiently read multiple scanlines or tiles at once, and if
    // the subclass does not provide overloads, the base class
    // implementaiton will be used instead, which is implemented by reducing
    // the operation to multiple calls to read_scanline or read_tile.

    /// Read a single scanline (all channels) of native data into contiguous
    /// memory.
    virtual bool read_native_scanline (int subimage, int miplevel,
                                       int y, int z, void *data) = 0;
    /// Read a range of scanlines (all channels) of native data into
    /// contiguous memory.
    virtual bool read_native_scanlines (int subimage, int miplevel,
                                        int ybegin, int yend, int z,
                                        void *data);
    /// Read a range of scanlines (with optionally a subset of channels) of
    /// native data into contiguous memory.
    virtual bool read_native_scanlines (int subimage, int miplevel,
                                        int ybegin, int yend, int z,
                                        int chbegin, int chend, void *data);

    /// Read a single tile (all channels) of native data into contiguous
    /// mamory. The base class read_native_tile fails. A format reader that
    /// supports tiles MUST overload this virtual method that reads a single
    /// tile (all channels).
    virtual bool read_native_tile (int subimage, int miplevel,
                                   int x, int y, int z, void *data);

    /// Read multiple tiles (all channels) of native data into contigious
    /// memory. A format reader that supports reading multiple tiles at once
    /// (in a way that's more efficient than reading the tiles one at a
    /// time) is advised (but not required) to overload this virtual method.
    /// If an ImageInput subclass does not overload this, the default
    /// implementation here is simply to loop over the tiles, calling the
    /// single-tile read_native_tile() for each one.
    virtual bool read_native_tiles (int subimage, int miplevel,
                                    int xbegin, int xend, int ybegin, int yend,
                                    int zbegin, int zend, void *data);

    /// Read multiple tiles (potentially a subset of channels) of native
    /// data into contigious memory. A format reader that supports reading
    /// multiple tiles at once, and can handle a channel subset while doing
    /// so, is advised (but not required) to overload this virtual method.
    /// If an ImageInput subclass does not overload this, the default
    /// implementation here is simply to loop over the tiles, calling the
    /// single-tile read_native_tile() for each one (and copying carefully
    /// to handle the channel subset issues).
    virtual bool read_native_tiles (int subimage, int miplevel,
                                    int xbegin, int xend, int ybegin, int yend,
                                    int zbegin, int zend,
                                    int chbegin, int chend, void *data);
    /// @}


    // General message passing between client and image input server. This
    // is currently undefined and is reserved for future use.
    virtual int send_to_input (const char *format, ...);
    int send_to_client (const char *format, ...);

    /// If any of the API routines returned false indicating an error, this
    /// method will return the error string (and clear any error flags).  If
    /// no error has occurred since the last time `geterror()` was called,
    /// it will return an empty string.
    std::string geterror () const {
        lock_guard lock (m_mutex);
        std::string e = m_errmessage;
        m_errmessage.clear ();
        return e;
    }

    /// Error reporting for the plugin implementation: call this with
    /// Strutil::format-like arguments.
    /// Use with caution! Some day this will change to be fmt-like rather
    /// than printf-like.
    template<typename... Args>
    void error(const char* fmt, const Args&... args) const {
        append_error(Strutil::format (fmt, args...));
    }

    /// Error reporting for the plugin implementation: call this with
    /// printf-like arguments.
    template<typename... Args>
    void errorf(const char* fmt, const Args&... args) const {
        append_error(Strutil::sprintf (fmt, args...));
    }

    /// Error reporting for the plugin implementation: call this with
    /// fmt::format-like arguments.
    template<typename... Args>
    void fmterror(const char* fmt, const Args&... args) const {
        append_error(Strutil::fmt::format (fmt, args...));
    }

    /// Set the threading policy for this ImageInput, controlling the
    /// maximum amount of parallelizing thread "fan-out" that might occur
    /// during large read operations. The default of 0 means that the global
    /// `attribute("threads")` value should be used (which itself defaults
    /// to using as many threads as cores; see Section `Global Attributes`_).
    ///
    /// The main reason to change this value is to set it to 1 to indicate
    /// that the calling thread should do all the work rather than spawning
    /// new threads. That is probably the desired behavior in situations
    /// where the calling application has already spawned multiple worker
    /// threads.
    void threads (int n) { m_threads = n; }

    /// Retrieve the current thread-spawning policy.
    /// @see  `threads(int)`
    int threads () const { return m_threads; }

    /// Lock the internal mutex, block until the lock is acquired.
    void lock () { m_mutex.lock(); }
    /// Try to loak the internal mutex, returning true if successful, or
    /// false if the lock could not be immediately acquired.
    bool try_lock () { return m_mutex.try_lock(); }
    /// Ulock the internal mutex.
    void unlock () { m_mutex.unlock(); }

    // Custom new and delete to ensure that allocations & frees happen in
    // the OpenImageIO library, not in the app or plugins (because Windows).
    void* operator new (size_t size);
    void operator delete (void *ptr);

    /// Call signature of a function that creates and returns an
    /// `ImageInput*`.
    typedef ImageInput* (*Creator)();

protected:
    mutable mutex m_mutex;   // lock of the thread-safe methods
    ImageSpec m_spec;  // format spec of the current open subimage/MIPlevel
                       // BEWARE using m_spec directly -- not thread-safe

private:
    mutable std::string m_errmessage;  // private storage of error message
    int m_threads;    // Thread policy
    void append_error (const std::string& message) const; // add to m_errmessage
    // Deprecated:
    static unique_ptr create (const std::string& filename, bool do_open,
                              const std::string& plugin_searchpath);
};




/// ImageOutput abstracts the writing of an image file in a file
/// format-agnostic manner.
///
/// Users don't directly declare these. Instead, you call the `create()`
/// static method, which will return a `unique_ptr` holding a subclass of
/// ImageOutput that implements writing the particular format.
///
class OIIO_API ImageOutput {
public:
    /// unique_ptr to an ImageOutput.
    using unique_ptr = std::unique_ptr<ImageOutput>;

    /// @{
    /// @name Creating an ImageOutput

    /// Create an `ImageOutput` that can be used to write an image file.
    /// The type of image file (and hence, the particular subclass of
    /// `ImageOutput` returned, and the plugin that contains its methods) is
    /// inferred from the name.
    ///
    /// @param filename The name of the file format (e.g., "openexr"), a
    ///                 file extension (e.g., "exr"), or a filename from
    ///                 which the the file format can be inferred from its
    ///                 extension (e.g., "hawaii.exr").
    /// @param plugin_searchpath
    ///                 An optional colon-separated list of directories to
    ///                 search for OpenImageIO plugin DSO/DLL's.
    /// @returns        A `unique_ptr` that will close and free the
    ///                 ImageOutput when it exits scope or is reset.
    ///                 The pointer will be empty if the required writer
    ///                 was not able to be created.
    static unique_ptr create (const std::string &filename,
                              const std::string &plugin_searchpath="");

    /// @}

    // @deprecated
    static void destroy (ImageOutput *x);

protected:
    ImageOutput ();
public:
    virtual ~ImageOutput ();

    /// Return the name of the format implemented by this class.
    virtual const char *format_name (void) const = 0;

    // Overrride these functions in your derived output class
    // to inform the client which formats are supported

    /// @{
    /// @name Opening and closing files for output

    /// Given the name of a "feature", return whether this ImageOutput
    /// supports output of images with the given properties. Most queries
    /// will simply return 0 for "doesn't support" and 1 for "supports it,"
    /// but it is acceptable to have queries return other nonzero integers
    /// to indicate varying degrees of support or limits (but should be
    /// clearly documented as such).
    ///
    /// Feature names that ImageOutput implementations are expected to
    /// recognize include:
    ///
    ///  - `"tiles"` :
    ///         Is this format writer able to write tiled images?
    ///
    ///  - `"rectangles"` :
    ///         Does this writer accept arbitrary rectangular pixel regions
    ///         (via `write_rectangle()`)?  Returning 0 indicates that
    ///         pixels must be transmitted via `write_scanline()` (if
    ///         scanline-oriented) or `write_tile()` (if tile-oriented, and
    ///         only if `supports("tiles")` returns true).
    ///
    ///  - `"random_access"` :
    ///         May tiles or scanlines be written in any order (0 indicates
    ///         that they *must* be in successive order)?
    ///
    ///  - `"multiimage"` :
    ///         Does this format support multiple subimages within a file?
    ///
    ///  - `"appendsubimage"` :
    ///         Does this format support multiple subimages that can be
    ///         successively appended at will via
    ///         `open(name,spec,AppendSubimage)`? A value of 0 means that
    ///         the format requires pre-declaring the number and
    ///         specifications of the subimages when the file is first
    ///         opened, with `open(name,subimages,specs)`.
    ///
    ///  - `"mipmap"` :
    ///         Does this format support multiple resolutions for an
    ///         image/subimage?
    ///
    ///  - `"volumes"` :
    ///         Does this format support "3D" pixel arrays (a.k.a. volume
    ///         images)?
    ///
    ///  - `"alpha"` :
    ///         Can this format support an alpha channel?
    ///
    ///  - `"nchannels"` :
    ///         Can this format support arbitrary number of channels (beyond RGBA)?
    ///
    ///  - `"rewrite"` :
    ///         May the same scanline or tile be sent more than once?
    ///         Generally, this is true for plugins that implement
    ///         interactive display, rather than a saved image file.
    ///
    ///  - `"empty"` :
    ///         Does this plugin support passing a NULL data pointer to the
    ///         various `write` routines to indicate that the entire data
    ///         block is composed of pixels with value zero?  Plugins that
    ///         support this achieve a speedup when passing blank scanlines
    ///         or tiles (since no actual data needs to be transmitted or
    ///         converted).
    ///
    ///  - `"channelformats"` :
    ///         Does this format writer support per-channel data formats,
    ///         respecting the ImageSpec's `channelformats` field? If not,
    ///         it only accepts a single data format for all channels and
    ///         will ignore the `channelformats` field of the spec.
    ///
    ///  - `"displaywindow"` :
    ///         Does the format support display ("full") windows distinct
    ///         from the pixel data window?
    ///
    ///  - `"origin"` :
    ///         Does the image format support specifying a pixel window
    ///         origin (i.e., nonzero ImageSpec `x`, `y`, `z`)?
    ///
    ///  - `"negativeorigin"` :
    ///         Does the image format allow data and display window origins
    ///         (i.e., ImageSpec `x`, `y`, `z`, `full_x`, `full_y`, `full_z`)
    ///         to have negative values?
    ///
    ///  - `"deepdata"` :
    ///        Does the image format allow "deep" data consisting of
    ///        multiple values per pixel (and potentially a differing number
    ///        of values from pixel to pixel)?
    ///
    ///  - `"arbitrary_metadata"` :
    ///         Does the image file format allow metadata with arbitrary
    ///         names (and either arbitrary, or a reasonable set of, data
    ///         types)? (Versus the file format supporting only a fixed list
    ///         of specific metadata names/values.)
    ///
    ///  - `"exif"`
    ///         Does the image file format support Exif camera data (either
    ///         specifically, or via arbitrary named metadata)?
    ///
    ///  - `"iptc"`
    ///         Does the image file format support IPTC data (either
    ///         specifically, or via arbitrary named metadata)?
    ///
    ///  - `"ioproxy"`
    ///         Does the image file format support writing to an `IOProxy`?
    ///
    /// This list of queries may be extended in future releases. Since this
    /// can be done simply by recognizing new query strings, and does not
    /// require any new API entry points, addition of support for new
    /// queries does not break ``link compatibility'' with
    /// previously-compiled plugins.
    virtual int supports (string_view feature) const { return false; }

    /// Modes passed to the `open()` call.
    enum OpenMode { Create, AppendSubimage, AppendMIPLevel };

    /// Open the file with given name, with resolution and other format
    /// data as given in newspec. It is legal to call open multiple times
    /// on the same file without a call to `close()`, if it supports
    /// multiimage and mode is AppendSubimage, or if it supports
    /// MIP-maps and mode is AppendMIPLevel -- this is interpreted as
    /// appending a subimage, or a MIP level to the current subimage,
    /// respectively.
    ///
    /// @param  name        The name of the image file to open.
    /// @param  newspec     The ImageSpec describing the resolution, data
    ///                     types, etc.
    /// @param  mode        Specifies whether the purpose of the `open` is
    ///                     to create/truncate the file (default: `Create`),
    ///                     append another subimage (`AppendSubimage`), or
    ///                     append another MIP level (`AppendMIPLevel`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool open (const std::string &name, const ImageSpec &newspec,
                       OpenMode mode=Create) = 0;

    /// Open a multi-subimage file with given name and specifications for
    /// each of the subimages.  Upon success, the first subimage will be
    /// open and ready for transmission of pixels.  Subsequent subimages
    /// will be denoted with the usual call of
    /// `open(name,spec,AppendSubimage)` (and MIP levels by
    /// `open(name,spec,AppendMIPLevel)`).
    ///
    /// The purpose of this call is to accommodate format-writing
    /// libraries that fmust know the number and specifications of the
    /// subimages upon first opening the file; such formats can be
    /// detected by::
    ///     supports("multiimage") && !supports("appendsubimage")
    /// The individual specs passed to the appending open() calls for
    /// subsequent subimages *must* match the ones originally passed.
    ///
    /// @param  name        The name of the image file to open.
    /// @param  subimages   The number of subimages (and therefore the
    ///                     length of the `specs[]` array.
    /// @param  specs[]
    ///                      Pointer to an array of `ImageSpec` objects
    ///                      describing each of the expected subimages.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool open (const std::string &name, int subimages,
                       const ImageSpec *specs) {
        // Default implementation: just a regular open, assume that
        // appending will work.
        return open (name, specs[0]);
    }

    /// Return a reference to the image format specification of the current
    /// subimage.  Note that the contents of the spec are invalid before
    /// `open()` or after `close()`.
    const ImageSpec &spec (void) const { return m_spec; }

    /// Closes the currently open file associated with this ImageOutput and
    /// frees any memory or resources associated with it.
    virtual bool close () = 0;
    /// @}

    /// @{
    /// @name Writing pixels
    ///
    /// Common features of all the `write` methods:
    ///
    /// * The `format` parameter describes the data type of the `data[]`. The
    ///   write methods automatically convert the data from the specified
    ///   `format` to the actual output data type of the file (as was
    ///   specified by the ImageSpec passed to `open()`).  If `format` is
    ///   `TypeUnknown`, then rather than converting from `format`, it will
    ///   just copy pixels assumed to already be in the file's native data
    ///   layout (including, possibly, per-channel data formats as specified
    ///   by the ImageSpec's `channelfomats` field).
    ///
    /// * The `stride` values describe the layout of the `data` buffer:
    ///   `xstride` is the distance in bytes between successive pixels
    ///   within each scanline. `ystride` is the distance in bytes between
    ///   successive scanlines. For volumetric images `zstride` is the
    ///   distance in bytes between successive "volumetric planes".  Strides
    ///   set to the special value `AutoStride` imply contiguous data, i.e.,
    ///
    ///       xstride = format.size() * nchannels
    ///       ystride = xstride * width
    ///       zstride = ystride * height
    ///
    /// * Any *range* parameters (such as `ybegin` and `yend`) describe a
    ///   "half open interval", meaning that `begin` is the first item and
    ///   `end` is *one past the last item*. That means that the number of
    ///   items is `end - begin`.
    ///
    /// * For ordinary 2D (non-volumetric) imaages, any `z` or `zbegin`
    ///   coordinates should be 0 and any `zend` should be 1, indicating
    ///   that only a single image "plane" exists.
    ///
    /// * Scanlines or tiles must be written in successive increasing
    ///   coordinate order, unless the particular output file driver allows
    ///   random access (indicated by `supports("random_access")`).
    ///
    /// * All write functions return `true` for success, `false` for failure
    ///   (after which a call to `geterror()` may retrieve a specific error
    ///   message).
    ///

    /// Write the full scanline that includes pixels (*,y,z).  For 2D
    /// non-volume images, `z` should be 0. The `xstride` value gives the
    /// distance between successive pixels (in bytes).  Strides set to
    /// `AutoStride` imply "contiguous" data.
    ///
    /// @param  y/z         The y & z coordinates of the scanline.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride     The distance in bytes between successive
    ///                     pixels in `data` (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_scanline (int y, int z, TypeDesc format,
                                 const void *data, stride_t xstride=AutoStride);

    /// Write multiple scanlines that include pixels (*,y,z) for all ybegin
    /// <= y < yend, from data.  This is analogous to
    /// `write_scanline(y,z,format,data,xstride)` repeatedly for each of the
    /// scanlines in turn (the advantage, though, is that some image file
    /// types may be able to write multiple scanlines more efficiently or
    /// in parallel, than it could with one scanline at a time).
    ///
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride
    ///                     The distance in bytes between successive pixels
    ///                     and scanlines (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_scanlines (int ybegin, int yend, int z,
                                  TypeDesc format, const void *data,
                                  stride_t xstride=AutoStride,
                                  stride_t ystride=AutoStride);

    /// Write the tile with (x,y,z) as the upper left corner.  The three
    /// stride values give the distance (in bytes) between successive
    /// pixels, scanlines, and volumetric slices, respectively.  Strides set
    /// to AutoStride imply 'contiguous' data in the shape of a full tile,
    /// i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * spec.tile_width
    ///     zstride = ystride * spec.tile_height
    ///
    /// @param  x/y/z       The upper left coordinate of the tile being passed.
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride` to
    ///                     indicate a "contiguous" single tile).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note This call will fail if the image is not tiled, or if (x,y,z)
    /// is not the upper left corner coordinates of a tile.
    virtual bool write_tile (int x, int y, int z, TypeDesc format,
                             const void *data, stride_t xstride=AutoStride,
                             stride_t ystride=AutoStride,
                             stride_t zstride=AutoStride);

    /// Write the block of multiple tiles that include all pixels in
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// This is analogous to calling `write_tile(x,y,z,...)` for each tile
    /// in turn (but for some file formats, passing multiple tiles may allow
    /// it to write more efficiently or in parallel).
    ///
    /// The begin/end pairs must correctly delineate tile boundaries, with
    /// the exception that it may also be the end of the image data if the
    /// image resolution is not a whole multiple of the tile size. The
    /// stride values give the data spacing of adjacent pixels, scanlines,
    /// and volumetric slices (measured in bytes). Strides set to AutoStride
    /// imply contiguous data in the shape of the [begin,end) region, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * (xend-xbegin)
    ///     zstride = ystride * (yend-ybegin)
    ///
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles passed.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool write_tiles (int xbegin, int xend, int ybegin, int yend,
                              int zbegin, int zend, TypeDesc format,
                              const void *data, stride_t xstride=AutoStride,
                              stride_t ystride=AutoStride,
                              stride_t zstride=AutoStride);

    /// Write a rectangle of pixels given by the range
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// The stride values give the data spacing of adjacent pixels,
    /// scanlines, and volumetric slices (measured in bytes). Strides set to
    /// AutoStride imply contiguous data in the shape of the [begin,end)
    /// region, i.e.,
    ///
    ///     xstride = format.size() * spec.nchannels
    ///     ystride = xstride * (xend-xbegin)
    ///     zstride = ystride * (yend-ybegin)
    ///
    /// @param  xbegin/xend The x range of the pixels being passed.
    /// @param  ybegin/yend The y range of the pixels being passed.
    /// @param  zbegin/zend The z range of the pixels being passed
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail for a format plugin that does not return
    /// true for `supports("rectangles")`.
    virtual bool write_rectangle (int xbegin, int xend, int ybegin, int yend,
                                  int zbegin, int zend, TypeDesc format,
                                  const void *data, stride_t xstride=AutoStride,
                                  stride_t ystride=AutoStride,
                                  stride_t zstride=AutoStride);

    /// Write the entire image of `spec.width x spec.height x spec.depth`
    /// pixels, from a buffer with the given strides and in the desired
    /// format.
    ///
    /// Depending on the spec, this will write either all tiles or all
    /// scanlines. Assume that data points to a layout in row-major order.
    ///
    /// Because this may be an expensive operation, a progress callback
    /// may be passed.  Periodically, it will be called as follows:
    ///
    ///     progress_callback (progress_callback_data, float done);
    ///
    /// where `done` gives the portion of the image (between 0.0 and 1.0)
    /// that has been written thus far.
    ///
    /// @param  format      A TypeDesc describing the type of `data`.
    /// @param  data        Pointer to the pixel data.
    /// @param  xstride/ystride/zstride
    ///                     The distance in bytes between successive pixels,
    ///                     scanlines, and image planes (or `AutoStride`).
    /// @param  progress_callback/progress_callback_data
    ///                     Optional progress callback.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_image (TypeDesc format, const void *data,
                              stride_t xstride=AutoStride,
                              stride_t ystride=AutoStride,
                              stride_t zstride=AutoStride,
                              ProgressCallback progress_callback=nullptr,
                              void *progress_callback_data=nullptr);

    /// Write deep scanlines containing pixels (*,y,z), for all y in the
    /// range [ybegin,yend), to a deep file. This will fail if it is not a
    /// deep file.
    ///
    /// @param  ybegin/yend The y range of the scanlines being passed.
    /// @param  z           The z coordinate of the scanline.
    /// @param  deepdata    A `DeepData` object with the data for these
    ///                     scanlines.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_deep_scanlines (int ybegin, int yend, int z,
                                       const DeepData &deepdata);

    /// Write the block of deep tiles that include all pixels in
    /// the range
    ///
    ///     [xbegin,xend) X [ybegin,yend) X [zbegin,zend)
    ///
    /// The begin/end pairs must correctly delineate tile boundaries, with
    /// the exception that it may also be the end of the image data if the
    /// image resolution is not a whole multiple of the tile size.
    ///
    /// @param  xbegin/xend The x range of the pixels covered by the group
    ///                     of tiles passed.
    /// @param  ybegin/yend The y range of the pixels covered by the tiles.
    /// @param  zbegin/zend The z range of the pixels covered by the tiles
    ///                     (for a 2D image, zbegin=0 and zend=1).
    /// @param  deepdata    A `DeepData` object with the data for the tiles.
    /// @returns            `true` upon success, or `false` upon failure.
    ///
    /// @note The call will fail if the image is not tiled, or if the pixel
    /// ranges do not fall along tile (or image) boundaries, or if it is not
    /// a valid tile range.
    virtual bool write_deep_tiles (int xbegin, int xend, int ybegin, int yend,
                                   int zbegin, int zend,
                                   const DeepData &deepdata);

    /// Write the entire deep image described by `deepdata`. Depending on
    /// the spec, this will write either all tiles or all scanlines.
    ///
    /// @param  deepdata    A `DeepData` object with the data for the image.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool write_deep_image (const DeepData &deepdata);

    /// @}

    /// Read the current subimage of `in`, and write it as the next
    /// subimage of `*this`, in a way that is efficient and does not alter
    /// pixel values, if at all possible.  Both `in` and `this` must be a
    /// properly-opened `ImageInput` and `ImageOutput`, respectively, and
    /// their current images must match in size and number of channels.
    ///
    /// If a particular ImageOutput implementation does not supply a
    /// `copy_image` method, it will inherit the default implementation,
    /// which is to simply read scanlines or tiles from `in` and write them
    /// to `*this`.  However, some file format implementations may have a
    /// special technique for directly copying raw pixel data from the input
    /// to the output, when both are the same file type and the same pixel
    /// data type.  This can be more efficient than `in->read_image()`
    /// followed by `out->write_image()`, and avoids any unintended pixel
    /// alterations, especially for formats that use lossy compression.
    ///
    /// @param  in          A pointer to the open `ImageInput` to read from.
    /// @returns            `true` upon success, or `false` upon failure.
    virtual bool copy_image (ImageInput *in);

    // General message passing between client and image output server. This
    // is currently undefined and is reserved for future use.
    virtual int send_to_output (const char *format, ...);
    int send_to_client (const char *format, ...);

    /// If any of the API routines returned false indicating an error, this
    /// method will return the error string (and clear any error flags).  If
    /// no error has occurred since the last time `geterror()` was called,
    /// it will return an empty string.
    std::string geterror () const {
        std::string e = m_errmessage;
        m_errmessage.clear ();
        return e;
    }

    /// Error reporting for the plugin implementation: call this with
    /// `Strutil::format`-like arguments.
    /// Use with caution! Some day this will change to be fmt-like rather
    /// than printf-like.
    template<typename... Args>
    void error(const char* fmt, const Args&... args) const {
        append_error(Strutil::format (fmt, args...));
    }

    /// Error reporting for the plugin implementation: call this with
    /// printf-like arguments.
    template<typename... Args>
    void errorf(const char* fmt, const Args&... args) const {
        append_error(Strutil::sprintf (fmt, args...));
    }

    /// Error reporting for the plugin implementation: call this with
    /// fmt::format-like arguments.
    template<typename... Args>
    void fmterror(const char* fmt, const Args&... args) const {
        append_error(Strutil::fmt::format (fmt, args...));
    }

    /// Set the threading policy for this ImageOutput, controlling the
    /// maximum amount of parallelizing thread "fan-out" that might occur
    /// during large write operations. The default of 0 means that the
    /// global `attribute("threads")` value should be used (which itself
    /// defaults to using as many threads as cores; see Section
    /// `Global Attributes`_).
    ///
    /// The main reason to change this value is to set it to 1 to indicate
    /// that the calling thread should do all the work rather than spawning
    /// new threads. That is probably the desired behavior in situations
    /// where the calling application has already spawned multiple worker
    /// threads.
    void threads (int n) { m_threads = n; }

    /// Retrieve the current thread-spawning policy.
    /// @see  `threads(int)`
    int threads () const { return m_threads; }

    // Custom new and delete to ensure that allocations & frees happen in
    // the OpenImageIO library, not in the app or plugins (because Windows).
    void* operator new (size_t size);
    void operator delete (void *ptr);

    /// Call signature of a function that creates and returns an
    /// `ImageOutput*`.
    typedef ImageOutput* (*Creator)();

protected:
    /// Helper routines used by write_* implementations: convert data (in
    /// the given format and stride) to the "native" format of the file
    /// (described by the 'spec' member variable), in contiguous order. This
    /// requires a scratch space to be passed in so that there are no memory
    /// leaks.  Returns a pointer to the native data, which may be the
    /// original data if it was already in native format and contiguous, or
    /// it may point to the scratch space if it needed to make a copy or do
    /// conversions. For float->uint8 conversions only, if dither is
    /// nonzero, random dither will be added to reduce quantization banding
    /// artifacts; in this case, the specific nonzero dither value is used
    /// as a seed for the hash function that produces the per-pixel dither
    /// amounts, and the optional [xyz]origin parameters help it to align
    /// the pixels to the right position in the dither pattern.
    const void *to_native_scanline (TypeDesc format,
                                    const void *data, stride_t xstride,
                                    std::vector<unsigned char> &scratch,
                                    unsigned int dither=0,
                                    int yorigin=0, int zorigin=0);
    const void *to_native_tile (TypeDesc format, const void *data,
                                stride_t xstride, stride_t ystride,
                                stride_t zstride,
                                std::vector<unsigned char> &scratch,
                                unsigned int dither=0,
                                int xorigin=0, int yorigin=0, int zorigin=0);
    const void *to_native_rectangle (int xbegin, int xend, int ybegin, int yend,
                                     int zbegin, int zend,
                                     TypeDesc format, const void *data,
                                     stride_t xstride, stride_t ystride,
                                     stride_t zstride,
                                     std::vector<unsigned char> &scratch,
                                     unsigned int dither=0,
                                     int xorigin=0, int yorigin=0, int zorigin=0);

    /// Helper function to copy a rectangle of data into the right spot in
    /// an image-sized buffer. In addition to copying to the right place,
    /// this handles data format conversion and dither (if the spec's
    /// "oiio:dither" is nonzero, and if it's converting from a float-like
    /// type to UINT8). The buf_format describes the type of image_buffer,
    /// if it's TypeDesc::UNKNOWN it will be assumed to be spec.format.
    bool copy_to_image_buffer (int xbegin, int xend, int ybegin, int yend,
                               int zbegin, int zend, TypeDesc format,
                               const void *data, stride_t xstride,
                               stride_t ystride, stride_t zstride,
                               void *image_buffer,
                               TypeDesc buf_format = TypeDesc::UNKNOWN);
    /// Helper function to copy a tile of data into the right spot in an
    /// image-sized buffer. This is really just a wrapper for
    /// copy_to_image_buffer, passing all the right parameters to copy
    /// exactly one tile.
    bool copy_tile_to_image_buffer (int x, int y, int z, TypeDesc format,
                                    const void *data, stride_t xstride,
                                    stride_t ystride, stride_t zstride,
                                    void *image_buffer,
                                    TypeDesc buf_format = TypeDesc::UNKNOWN);

protected:
    ImageSpec m_spec;           ///< format spec of the currently open image

private:
    void append_error (const std::string& message) const; // add to m_errmessage
    mutable std::string m_errmessage;   ///< private storage of error message
    int m_threads;    // Thread policy
};



// Utility functions

/// Returns a numeric value for the version of OpenImageIO, 10000 for each
/// major version, 100 for each minor version, 1 for each patch.  For
/// example, OpenImageIO 1.2.3 would return a value of 10203. One example of
/// how this is useful is for plugins to query the version to be sure they
/// are linked against an adequate version of the library.
OIIO_API int openimageio_version ();

/// Returns any error string describing what went wrong if
/// `ImageInput::create()` or `ImageOutput::create()` failed (since in such
/// cases, the ImageInput or ImageOutput itself does not exist to have its
/// own `geterror()` function called). This function returns the last error
/// for this particular thread; separate threads will not clobber each
/// other's global error messages.
OIIO_API std::string geterror ();

/// `OIIO::attribute()` sets an global attribute (i.e., a property or
/// option) of OpenImageIO. The `name` designates the name of the attribute,
/// `type` describes the type of data, and `val` is a pointer to memory
/// containing the new value for the attribute.
///
/// If the name is known, valid attribute that matches the type specified,
/// the attribute will be set to the new value and `attribute()` will return
/// `true`.  If `name` is not recognized, or if the types do not match
/// (e.g., `type` is `TypeFloat` but the named attribute is a string), the
/// attribute will not be modified, and `attribute()` will return `false`.
///
/// The following are the recognized attributes:
///
/// - `string options`
///
///    This catch-all is simply a comma-separated list of `name=value`
///    settings of named options, which will be parsed and individually set.
///    For example,
///
///        OIIO::attribute ("options", "threads=4,log_times=1");
///
///    Note that if an option takes a string value that must itself contain
///    a comma, it is permissible to enclose the value in either 'single'
///    or "double" quotes.
///
/// - `int threads`
///
///    How many threads to use for operations that can be sped up by being
///    multithreaded. (Examples: simultaneous format conversions of multiple
///    scanlines read together, or many ImageBufAlgo operations.) The
///    default is 0, meaning to use the full available hardware concurrency
///    detected.
///
///    Situations where the main application logic is essentially single
///    threaded (i.e., one top-level call into OIIO at a time) should leave
///    this at the default value, or some reasonable number of cores, thus
///    allowing lots of threads to fill the cores when OIIO has big tasks to
///    complete. But situations where you have many threads at the
///    application level, each of which is expected to be making separate
///    OIIO calls simultaneously, should set this to 1, thus having each
///    calling thread do its own work inside of OIIO rather than spawning
///    new threads with a high overall "fan out.""
///
/// - `int exr_threads`
///
///    Sets the internal OpenEXR thread pool size. The default is to use as
///    many threads as the amount of hardware concurrency detected. Note
///    that this is separate from the OIIO `"threads"` attribute.
///
/// - `string plugin_searchpath`
///
///    Colon-separated list of directories to search for dynamically-loaded
///    format plugins.
///
/// - `int read_chunk`
///
///    When performing a `read_image()`, this is the number of scanlines it
///    will attempt to read at a time (some formats are more efficient when
///    reading and decoding multiple scanlines).  The default is 256. The
///    special value of 0 indicates that it should try to read the whole
///    image if possible.
///
/// - `float[] missingcolor`, `string missingcolor`
///
///    This attribute may either be an array of float values, or a string
///    containing a comma-separated list of the values. Setting this option
///    globally is equivalent to always passing an `ImageInput`
///    open-with-configuration hint `"oiio:missingcolor"` with the value.
///
///    When set, it gives some `ImageInput` readers the option of ignoring
///    any *missing* tiles or scanlines in the file, and instead of treating
///    the read failure of an individual tile as a full error, will
///    interpret is as an intentionally missing tile and proceed by simply
///    filling in the missing pixels with the color specified. If the first
///    element is negative, it will use the absolute value, but draw
///    alternating diagonal stripes of the color. For example,
///
///        float missing[4] = { -1.0, 0.0, 0.0, 0.0 }; // striped red
///        OIIO::attribute ("missingcolor", TypeDesc("float[4]"), &missing);
///
///    Note that only some file formats support files with missing tiles or
///    scanlines, and this is only taken as a hint. Please see
///    chap-bundledplugins_ for details on which formats accept a
///    `"missingcolor"` configuration hint.
///
/// - `int debug`
///
///    When nonzero, various debug messages may be printed. The default is 0
///    for release builds, 1 for DEBUG builds (values > 1 are for OIIO
///    developers to print even more debugging information), This attribute
///    but also may be overridden by the OPENIMAGEIO_DEBUG environment
///    variable.
///
/// - `int tiff:half`
///
///    When nonzero, allows TIFF to write `half` pixel data. N.B. Most apps
///    may not read these correctly, but OIIO will. That's why the default
///    is not to support it.
///
/// - `int log_times`
///
///    When the `"log_times"` attribute is nonzero, `ImageBufAlgo` functions
///    are instrumented to record the number of times they were called and
///    the total amount of time spent executing them. It can be overridden
///    by environment variable `OPENIMAGEIO_LOG_TIMES`. If the value of
///    `log_times` is 2 or more when the application terminates, the timing
///    report will be printed to `stdout` upon exit.
///
///    When enabled, there is a slight runtime performance cost due to
///    checking the time at the start and end of each of those function
///    calls, and the locking and recording of the data structure that holds
///    the log information. When the `log_times` attribute is disabled,
///    there is no additional performance cost.
///
///    The report of totals can be retrieved as the value of the
///    `"timing_report"` attribute, using `OIIO:get_attribute()` call.
///
///    
///
OIIO_API bool attribute (string_view name, TypeDesc type, const void *val);

/// Shortcut attribute() for setting a single integer.
inline bool attribute (string_view name, int val) {
    return attribute (name, TypeInt, &val);
}
/// Shortcut attribute() for setting a single float.
inline bool attribute (string_view name, float val) {
    return attribute (name, TypeFloat, &val);
}
/// Shortcut attribute() for setting a single string.
inline bool attribute (string_view name, string_view val) {
    const char *s = val.c_str();
    return attribute (name, TypeString, &s);
}

/// Get the named global attribute of OpenImageIO, store it in `*val`.
/// Return `true` if found and it was compatible with the type specified,
/// otherwise return `false` and do not modify the contents of `*val`.  It
/// is up to the caller to ensure that `val` points to the right kind and
/// size of storage for the given type.
///
/// In addition to being able to retrieve all the attributes that are
/// documented as settable by the `OIIO::attribute()` call, `getattribute()`
/// can also retrieve the following read-only attributes:
///
/// - `string format_list`
/// - `string input_format_list`
/// - `string output_format_list`
///   
///   A comma-separated list of all the names of, respectively, all
///   supported image formats, all formats accepted as inputs, and all
///   formats accepted as outputs.
///   
/// - `string extension_list`
///   
///   For each format, the format name, followed by a colon, followed by a
///   comma-separated list of all extensions that are presumed to be used
///   for that format.  Semicolons separate the lists for formats.  For
///   example,
///
///        "tiff:tif;jpeg:jpg,jpeg;openexr:exr"
///
/// - `string library_list`
///   
///   For each format that uses a dependent library, the format name,
///   followed by a colon, followed by the name and version of the
///   dependency. Semicolons separate the lists for formats.  For example,
///
///        "tiff:LIBTIFF 4.0.4;gif:gif_lib 4.2.3;openexr:OpenEXR 2.2.0"
///
/// - string "timing_report"
///         A string containing the report of all the log_times.
///
/// - `string hw:simd`
/// - `string oiio:simd` (read-only)
///   
///   A comma-separated list of hardware CPU features for SIMD (and some
///   other things). The `"oiio:simd"` attribute is similarly a list of
///   which features this build of OIIO was compiled to support.
///
///   This was added in OpenImageIO 1.8.
///   
/// - `float resident_memory_used_MB`
///
///   This read-only attribute can be used for debugging purposes to report
///   the approximate process memory used (resident) by the application, in
///   MB.
///
/// - `string timing_report`
///
///    Retrieving this attribute returns the timing report generated by the
///    `log_timing` attribute (if it was enabled). The report is sorted
///    alphabetically and for each named instrumentation region, prints the
///    number of times it executed, the total runtime, and the average per
///    call, like this:
///
///        IBA::computePixelStats        2   2.69ms  (avg   1.34ms)
///        IBA::make_texture             1  74.05ms  (avg  74.05ms)
///        IBA::mul                      8   2.42ms  (avg   0.30ms)
///        IBA::over                    10  23.82ms  (avg   2.38ms)
///        IBA::resize                  20   0.24s   (avg  12.18ms)
///        IBA::zero                     8   0.66ms  (avg   0.08ms)
///
OIIO_API bool getattribute (string_view name, TypeDesc type, void *val);

/// Shortcut getattribute() for retrieving a single integer.
/// The value is placed in `val`, and the function returns `true` if the
/// attribute was found and was legally convertible to an int.
inline bool getattribute (string_view name, int &val) {
    return getattribute (name, TypeInt, &val);
}
/// Shortcut getattribute() for retrieving a single float.
/// The value is placed in `val`, and the function returns `true` if the
/// attribute was found and was legally convertible to a float.
inline bool getattribute (string_view name, float &val) {
    return getattribute (name, TypeFloat, &val);
}
/// Shortcut getattribute() for retrieving a single string as a
/// `std::string`. The value is placed in `val`, and the function returns
/// `true` if the attribute was found.
inline bool getattribute (string_view name, std::string &val) {
    ustring s;
    bool ok = getattribute (name, TypeString, &s);
    if (ok)
        val = s.string();
    return ok;
}
/// Shortcut getattribute() for retrieving a single string as a `char*`.
inline bool getattribute (string_view name, char **val) {
    return getattribute (name, TypeString, val);
}
/// Shortcut getattribute() for retrieving a single integer, with a supplied
/// default value that will be returned if the attribute is not found or
/// could not legally be converted to an int.
inline int get_int_attribute (string_view name, int defaultval=0) {
    int val;
    return getattribute (name, TypeInt, &val) ? val : defaultval;
}
/// Shortcut getattribute() for retrieving a single float, with a supplied
/// default value that will be returned if the attribute is not found or
/// could not legally be converted to a float.
inline float get_float_attribute (string_view name, float defaultval=0) {
    float val;
    return getattribute (name, TypeFloat, &val) ? val : defaultval;
}
/// Shortcut getattribute() for retrieving a single string, with a supplied
/// default value that will be returned if the attribute is not found.
inline string_view get_string_attribute (string_view name,
                                 string_view defaultval = string_view()) {
    ustring val;
    return getattribute (name, TypeString, &val) ? string_view(val) : defaultval;
}


/// Register the input and output 'create' routines and list of file
/// extensions for a particular format.
OIIO_API void declare_imageio_format (const std::string &format_name,
                                      ImageInput::Creator input_creator,
                                      const char **input_extensions,
                                      ImageOutput::Creator output_creator,
                                      const char **output_extensions,
                                      const char *lib_version);

/// Helper function: convert contiguous data between two arbitrary pixel
/// data types (specified by TypeDesc's). Return true if ok, false if it
/// didn't know how to do the conversion.  If dst_type is UNKNOWN, it will
/// be assumed to be the same as src_type.
///
/// The conversion is of normalized (pixel-like) values -- for example
/// 'UINT8' 255 will convert to float 1.0 and vice versa, not float 255.0.
/// If you want a straight C-like data cast convertion (e.g., uint8 255 ->
/// float 255.0), then you should prefer the un-normalized convert_type()
/// utility function found in typedesc.h.
OIIO_API bool convert_pixel_values (TypeDesc src_type, const void *src,
                                    TypeDesc dst_type, void *dst, int n = 1);

/// DEPRECATED(2.1): old name
inline bool convert_types (TypeDesc src_type, const void *src,
                           TypeDesc dst_type, void *dst, int n = 1) {
    return convert_pixel_values (src_type, src, dst_type, dst, n);
}


/// Helper routine for data conversion: Convert an image of nchannels x
/// width x height x depth from src to dst.  The src and dst may have
/// different data formats and layouts.  Clever use of this function can
/// not only exchange data among different formats (e.g., half to 8-bit
/// unsigned), but also can copy selective channels, copy subimages,
/// etc.  If you're lazy, it's ok to pass AutoStride for any of the
/// stride values, and they will be auto-computed assuming contiguous
/// data.  Return true if ok, false if it didn't know how to do the
/// conversion.
OIIO_API bool convert_image (int nchannels, int width, int height, int depth,
                             const void *src, TypeDesc src_type,
                             stride_t src_xstride, stride_t src_ystride,
                             stride_t src_zstride,
                             void *dst, TypeDesc dst_type,
                             stride_t dst_xstride, stride_t dst_ystride,
                             stride_t dst_zstride);
/// DEPRECATED(2.0) -- the alpha_channel, z_channel were never used
inline bool convert_image(int nchannels, int width, int height, int depth,
            const void *src, TypeDesc src_type,
            stride_t src_xstride, stride_t src_ystride, stride_t src_zstride,
            void *dst, TypeDesc dst_type,
            stride_t dst_xstride, stride_t dst_ystride, stride_t dst_zstride,
            int alpha_channel, int z_channel = -1)
{
    return convert_image(nchannels, width, height, depth, src, src_type,
                         src_xstride, src_ystride, src_zstride, dst, dst_type,
                         dst_xstride, dst_ystride, dst_zstride);
}


/// A version of convert_image that will break up big jobs into multiple
/// threads.
OIIO_API bool parallel_convert_image (
               int nchannels, int width, int height, int depth,
               const void *src, TypeDesc src_type,
               stride_t src_xstride, stride_t src_ystride,
               stride_t src_zstride,
               void *dst, TypeDesc dst_type,
               stride_t dst_xstride, stride_t dst_ystride,
               stride_t dst_zstride, int nthreads=0);
/// DEPRECATED(2.0) -- the alpha_channel, z_channel were never used
inline bool parallel_convert_image(
            int nchannels, int width, int height, int depth,
            const void *src, TypeDesc src_type,
            stride_t src_xstride, stride_t src_ystride, stride_t src_zstride,
            void *dst, TypeDesc dst_type,
            stride_t dst_xstride, stride_t dst_ystride, stride_t dst_zstride,
            int alpha_channel, int z_channel, int nthreads=0)
{
    return parallel_convert_image (nchannels, width, height, depth,
           src, src_type, src_xstride, src_ystride, src_zstride,
           dst, dst_type, dst_xstride, dst_ystride, dst_zstride, nthreads);
}

/// Add random [-theramplitude,ditheramplitude] dither to the color channels
/// of the image.  Dither will not be added to the alpha or z channel.  The
/// image origin and dither seed values allow a reproducible (or variable)
/// dither pattern.  If the strides are set to AutoStride, they will be
/// assumed to be contiguous floats in data of the given dimensions.
OIIO_API void add_dither (int nchannels, int width, int height, int depth,
                          float *data,
                          stride_t xstride, stride_t ystride, stride_t zstride,
                          float ditheramplitude,
                          int alpha_channel = -1, int z_channel = -1,
                          unsigned int ditherseed = 1,
                          int chorigin=0, int xorigin=0,
                          int yorigin=0, int zorigin=0);

/// Convert unassociated to associated alpha by premultiplying all color
/// (non-alpha, non-z) channels by alpha. The nchannels, width, height, and
/// depth parameters describe the "shape" of the image data (along with
/// optional stride overrides). The chbegin/chend describe which range of
/// channels to actually premultiply.
OIIO_API void premult (int nchannels, int width, int height, int depth,
                       int chbegin, int chend,
                       TypeDesc datatype, void *data, stride_t xstride,
                       stride_t ystride, stride_t zstride,
                       int alpha_channel = -1, int z_channel = -1);

/// Helper routine for data conversion: Copy an image of nchannels x
/// width x height x depth from src to dst.  The src and dst may have
/// different data layouts, but must have the same data type.  Clever
/// use of this function can change layouts or strides, copy selective
/// channels, copy subimages, etc.  If you're lazy, it's ok to pass
/// AutoStride for any of the stride values, and they will be
/// auto-computed assuming contiguous data.  Return true if ok, false if
/// it didn't know how to do the conversion.
OIIO_API bool copy_image (int nchannels, int width, int height, int depth,
                          const void *src, stride_t pixelsize,
                          stride_t src_xstride, stride_t src_ystride,
                          stride_t src_zstride,
                          void *dst, stride_t dst_xstride,
                          stride_t dst_ystride, stride_t dst_zstride);


// All the wrap_foo functions implement a wrap mode, wherein coord is
// altered to be origin <= coord < origin+width.  The return value
// indicates if the resulting wrapped value is valid (example, for
// wrap_black, values outside the region are invalid and do not modify
// the coord parameter).
OIIO_API bool wrap_black (int &coord, int origin, int width);
OIIO_API bool wrap_clamp (int &coord, int origin, int width);
OIIO_API bool wrap_periodic (int &coord, int origin, int width);
OIIO_API bool wrap_periodic_pow2 (int &coord, int origin, int width);
OIIO_API bool wrap_mirror (int &coord, int origin, int width);

// Typedef for the function signature of a wrap implementation.
typedef bool (*wrap_impl) (int &coord, int origin, int width);


/// `debug(format, ...)` prints debugging message when attribute "debug" is
/// nonzero, which it is by default for DEBUG compiles or when the
/// environment variable OPENIMAGEIO_DEBUG is set. This is preferred to raw
/// output to stderr for debugging statements.
OIIO_API void debug (string_view str);

/// debug output with `fmt`/`std::format` conventions.
template<typename T1, typename... Args>
void fmtdebug (const char* fmt, const T1& v1, const Args&... args)
{
    debug (Strutil::fmt::format(fmt, v1, args...));
}

/// debug output with printf conventions.
template<typename T1, typename... Args>
void debugf (const char* fmt, const T1& v1, const Args&... args)
{
    debug (Strutil::sprintf(fmt, v1, args...));
}

/// debug output with the same conventions as Strutil::format. Beware, this
/// will change one day!
template<typename T1, typename... Args>
void debug (const char* fmt, const T1& v1, const Args&... args)
{
    debug (Strutil::format(fmt, v1, args...));
}


// to force correct linkage on some systems
OIIO_API void _ImageIO_force_link ();

OIIO_NAMESPACE_END
