// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

/// \file
/// An API for accessing filtered texture lookups via a system that
/// automatically manages a cache of resident texture.

#pragma once

#include <OpenImageIO/imageio.h>
#include <OpenImageIO/ustring.h>
#include <OpenImageIO/varyingref.h>

#include <OpenEXR/ImathVec.h> /* because we need V3f */


// Define symbols that let client applications determine if newly added
// features are supported.
#define OIIO_TEXTURESYSTEM_SUPPORTS_CLOSE 1



OIIO_NAMESPACE_BEGIN

// Forward declarations

class ImageCache;


namespace pvt {

class TextureSystemImpl;

// Used internally by TextureSystem.  Unfortunately, this is the only
// clean place to store it.  Sorry, users, this isn't really for you.
enum TexFormat {
    TexFormatUnknown,
    TexFormatTexture,
    TexFormatTexture3d,
    TexFormatShadow,
    TexFormatCubeFaceShadow,
    TexFormatVolumeShadow,
    TexFormatLatLongEnv,
    TexFormatCubeFaceEnv,
    TexFormatLast
};

enum EnvLayout {
    LayoutTexture = 0 /* ordinary texture - no special env wrap */,
    LayoutLatLong,
    LayoutCubeThreeByTwo,
    LayoutCubeOneBySix,
    EnvLayoutLast
};

}  // namespace pvt



namespace Tex {

/// Wrap mode describes what happens when texture coordinates describe
/// a value outside the usual [0,1] range where a texture is defined.
enum class Wrap {
    Default,               ///< Use the default found in the file
    Black,                 ///< Black outside [0..1]
    Clamp,                 ///< Clamp to [0..1]
    Periodic,              ///< Periodic mod 1
    Mirror,                ///< Mirror the image
    PeriodicPow2,          ///< Periodic, but only for powers of 2!!!
    PeriodicSharedBorder,  ///< Periodic with shared border (env)
    Last                   ///< Mark the end -- don't use this!
};

/// Utility: Return the Wrap enum corresponding to a wrap name:
/// "default", "black", "clamp", "periodic", "mirror".
OIIO_API Wrap decode_wrapmode (const char *name);
OIIO_API Wrap decode_wrapmode (ustring name);

/// Utility: Parse a single wrap mode (e.g., "periodic") or a
/// comma-separated wrap modes string (e.g., "black,clamp") into
/// separate Wrap enums for s and t.
OIIO_API void parse_wrapmodes (const char *wrapmodes,
                               Wrap &swrapcode, Wrap &twrapcode);


/// Mip mode determines if/how we use mipmaps
///
enum class MipMode {
    Default,    ///< Default high-quality lookup
    NoMIP,      ///< Just use highest-res image, no MIP mapping
    OneLevel,   ///< Use just one mipmap level
    Trilinear,  ///< Use two MIPmap levels (trilinear)
    Aniso       ///< Use two MIPmap levels w/ anisotropic
};

/// Interp mode determines how we sample within a mipmap level
///
enum class InterpMode {
    Closest,      ///< Force closest texel
    Bilinear,     ///< Force bilinear lookup within a mip level
    Bicubic,      ///< Force cubic lookup within a mip level
    SmartBicubic  ///< Bicubic when maxifying, else bilinear
};


/// Fixed width for SIMD batching texture lookups.
/// May be changed for experimentation or future expansion.
#ifndef OIIO_TEXTURE_SIMD_BATCH_WIDTH
#    define OIIO_TEXTURE_SIMD_BATCH_WIDTH 16
#endif

/// The SIMD width for batched texturing operations. This is fixed within
/// any release of OpenImageIO, but may change from release to release and
/// also may be overridden at build time. A typical batch size is 16.
static constexpr int BatchWidth = OIIO_TEXTURE_SIMD_BATCH_WIDTH;
static constexpr int BatchAlign = BatchWidth * sizeof(float);

/// A type alias for a SIMD vector of floats with the batch width.
typedef simd::VecType<float, OIIO_TEXTURE_SIMD_BATCH_WIDTH>::type FloatWide;

/// A type alias for a SIMD vector of ints with the batch width.
typedef simd::VecType<int, OIIO_TEXTURE_SIMD_BATCH_WIDTH>::type IntWide;

/// `RunMask` is defined to be an integer large enough to hold at least
/// `BatchWidth` bits. The least significant bit corresponds to the first
/// (i.e., `[0]`) position of all batch arrays. For each position `i` in the
/// batch, the bit identified by `(1 << i)` controls whether that position
/// will be computed.
typedef uint64_t RunMask;


// The defined constant `RunMaskOn` contains the value with all bits
// `0..BatchWidth-1` set to 1.
#if OIIO_TEXTURE_SIMD_BATCH_WIDTH == 4
static constexpr RunMask RunMaskOn = 0xf;
#elif OIIO_TEXTURE_SIMD_BATCH_WIDTH == 8
static constexpr RunMask RunMaskOn = 0xff;
#elif OIIO_TEXTURE_SIMD_BATCH_WIDTH == 16
static constexpr RunMask RunMaskOn = 0xffff;
#elif OIIO_TEXTURE_SIMD_BATCH_WIDTH == 32
static constexpr RunMask RunMaskOn = 0xffffffff;
#elif OIIO_TEXTURE_SIMD_BATCH_WIDTH == 64
static constexpr RunMask RunMaskOn = 0xffffffffffffffffULL;
#else
#    error "Not a valid OIIO_TEXTURE_SIMD_BATCH_WIDTH choice"
#endif

}  // namespace Tex


/// Data type for flags that indicate on a point-by-point basis whether
/// we want computations to be performed.
typedef unsigned char Runflag;

/// Pre-defined values for Runflag's.
///
enum RunFlagVal { RunFlagOff = 0, RunFlagOn = 255 };

class TextureOptions;  // forward declaration



/// TextureOpt is a structure that holds many options controlling
/// single-point texture lookups.  Because each texture lookup API call
/// takes a reference to a TextureOpt, the call signatures remain
/// uncluttered rather than having an ever-growing list of parameters, most
/// of which will never vary from their defaults.
class OIIO_API TextureOpt {
public:
    /// Wrap mode describes what happens when texture coordinates describe
    /// a value outside the usual [0,1] range where a texture is defined.
    enum Wrap {
        WrapDefault,               ///< Use the default found in the file
        WrapBlack,                 ///< Black outside [0..1]
        WrapClamp,                 ///< Clamp to [0..1]
        WrapPeriodic,              ///< Periodic mod 1
        WrapMirror,                ///< Mirror the image
        WrapPeriodicPow2,          // Periodic, but only for powers of 2!!!
        WrapPeriodicSharedBorder,  // Periodic with shared border (env)
        WrapLast                   // Mark the end -- don't use this!
    };

    /// Mip mode determines if/how we use mipmaps
    ///
    enum MipMode {
        MipModeDefault,    ///< Default high-quality lookup
        MipModeNoMIP,      ///< Just use highest-res image, no MIP mapping
        MipModeOneLevel,   ///< Use just one mipmap level
        MipModeTrilinear,  ///< Use two MIPmap levels (trilinear)
        MipModeAniso       ///< Use two MIPmap levels w/ anisotropic
    };

    /// Interp mode determines how we sample within a mipmap level
    ///
    enum InterpMode {
        InterpClosest,      ///< Force closest texel
        InterpBilinear,     ///< Force bilinear lookup within a mip level
        InterpBicubic,      ///< Force cubic lookup within a mip level
        InterpSmartBicubic  ///< Bicubic when maxifying, else bilinear
    };


    /// Create a TextureOpt with all fields initialized to reasonable
    /// defaults.
    TextureOpt ()
        : firstchannel(0), subimage(0),
        swrap(WrapDefault), twrap(WrapDefault),
        mipmode(MipModeDefault), interpmode(InterpSmartBicubic),
        anisotropic(32), conservative_filter(true),
        sblur(0.0f), tblur(0.0f), swidth(1.0f), twidth(1.0f),
        fill(0.0f), missingcolor(nullptr),
        // dresultds(nullptr), dresultdt(nullptr),
        time(0.0f), bias(0.0f), samples(1),
        rwrap(WrapDefault), rblur(0.0f), rwidth(1.0f), // dresultdr(nullptr),
        // actualchannels(0),
        envlayout(0)
    { }

    /// Convert a TextureOptions for one index into a TextureOpt.
    ///
    TextureOpt(const TextureOptions& opt, int index);

    int firstchannel;           ///< First channel of the lookup
    int subimage;               ///< Subimage or face ID
    ustring subimagename;       ///< Subimage name
    Wrap swrap;                 ///< Wrap mode in the s direction
    Wrap twrap;                 ///< Wrap mode in the t direction
    MipMode mipmode;            ///< Mip mode
    InterpMode interpmode;      ///< Interpolation mode
    int anisotropic;            ///< Maximum anisotropic ratio
    bool conservative_filter;   ///< True == over-blur rather than alias
    float sblur, tblur;         ///< Blur amount
    float swidth, twidth;       ///< Multiplier for derivatives
    float fill;                 ///< Fill value for missing channels
    const float* missingcolor;  ///< Color for missing texture
    float time;                 ///< Time (for time-dependent texture lookups)
    float bias;                 ///< Bias for shadows
    int samples;                ///< Number of samples for shadows

    // For 3D volume texture lookups only:
    Wrap rwrap;    ///< Wrap mode in the r direction
    float rblur;   ///< Blur amount in the r direction
    float rwidth;  ///< Multiplier for derivatives in r direction

    /// Utility: Return the Wrap enum corresponding to a wrap name:
    /// "default", "black", "clamp", "periodic", "mirror".
    static Wrap decode_wrapmode(const char* name)
    {
        return (Wrap)Tex::decode_wrapmode(name);
    }
    static Wrap decode_wrapmode(ustring name)
    {
        return (Wrap)Tex::decode_wrapmode(name);
    }

    /// Utility: Parse a single wrap mode (e.g., "periodic") or a
    /// comma-separated wrap modes string (e.g., "black,clamp") into
    /// separate Wrap enums for s and t.
    static void parse_wrapmodes(const char* wrapmodes,
                                TextureOpt::Wrap& swrapcode,
                                TextureOpt::Wrap& twrapcode)
    {
        Tex::parse_wrapmodes(wrapmodes, *(Tex::Wrap*)&swrapcode,
                             *(Tex::Wrap*)&twrapcode);
    }

private:
    // Options set INTERNALLY by libtexture after the options are passed
    // by the user.  Users should not attempt to alter these!
    int envlayout;  // Layout for environment wrap
    friend class pvt::TextureSystemImpl;
};



/// Texture options for a batch of Tex::BatchWidth points and run mask.
class OIIO_API TextureOptBatch {
public:
    /// Create a TextureOptBatch with all fields initialized to reasonable
    /// defaults.
    TextureOptBatch () {}   // use inline initializers

    // Options that may be different for each point we're texturing
    alignas(Tex::BatchAlign) float sblur[Tex::BatchWidth];    ///< Blur amount
    alignas(Tex::BatchAlign) float tblur[Tex::BatchWidth];
    alignas(Tex::BatchAlign) float rblur[Tex::BatchWidth];
    alignas(Tex::BatchAlign) float swidth[Tex::BatchWidth];   ///< Multiplier for derivatives
    alignas(Tex::BatchAlign) float twidth[Tex::BatchWidth];
    alignas(Tex::BatchAlign) float rwidth[Tex::BatchWidth];
    // Note: rblur,rwidth only used for volumetric lookups

    // Options that must be the same for all points we're texturing at once
    int firstchannel = 0;                 ///< First channel of the lookup
    int subimage = 0;                     ///< Subimage or face ID
    ustring subimagename;                 ///< Subimage name
    Tex::Wrap swrap = Tex::Wrap::Default; ///< Wrap mode in the s direction
    Tex::Wrap twrap = Tex::Wrap::Default; ///< Wrap mode in the t direction
    Tex::Wrap rwrap = Tex::Wrap::Default; ///< Wrap mode in the r direction (volumetric)
    Tex::MipMode mipmode = Tex::MipMode::Default;  ///< Mip mode
    Tex::InterpMode interpmode = Tex::InterpMode::SmartBicubic;  ///< Interpolation mode
    int anisotropic = 32;                 ///< Maximum anisotropic ratio
    int conservative_filter = 1;          ///< True: over-blur rather than alias
    float fill = 0.0f;                    ///< Fill value for missing channels
    const float *missingcolor = nullptr;  ///< Color for missing texture

private:
    // Options set INTERNALLY by libtexture after the options are passed
    // by the user.  Users should not attempt to alter these!
    int envlayout = 0;               // Layout for environment wrap

    friend class pvt::TextureSystemImpl;
};



/// DEPRECATED(1.8)
/// Encapsulate all the options needed for texture lookups.  Making
/// these options all separate parameters to the texture API routines is
/// very ugly and also a big pain whenever we think of new options to
/// add.  So instead we collect all those little options into one
/// structure that can just be passed by reference to the texture API
/// routines.
class OIIO_API TextureOptions {
public:
    /// Wrap mode describes what happens when texture coordinates describe
    /// a value outside the usual [0,1] range where a texture is defined.
    enum Wrap {
        WrapDefault,               ///< Use the default found in the file
        WrapBlack,                 ///< Black outside [0..1]
        WrapClamp,                 ///< Clamp to [0..1]
        WrapPeriodic,              ///< Periodic mod 1
        WrapMirror,                ///< Mirror the image
        WrapPeriodicPow2,          ///< Periodic, but only for powers of 2!!!
        WrapPeriodicSharedBorder,  ///< Periodic with shared border (env)
        WrapLast                   ///< Mark the end -- don't use this!
    };

    /// Mip mode determines if/how we use mipmaps
    ///
    enum MipMode {
        MipModeDefault,    ///< Default high-quality lookup
        MipModeNoMIP,      ///< Just use highest-res image, no MIP mapping
        MipModeOneLevel,   ///< Use just one mipmap level
        MipModeTrilinear,  ///< Use two MIPmap levels (trilinear)
        MipModeAniso       ///< Use two MIPmap levels w/ anisotropic
    };

    /// Interp mode determines how we sample within a mipmap level
    ///
    enum InterpMode {
        InterpClosest,      ///< Force closest texel
        InterpBilinear,     ///< Force bilinear lookup within a mip level
        InterpBicubic,      ///< Force cubic lookup within a mip level
        InterpSmartBicubic  ///< Bicubic when maxifying, else bilinear
    };

    /// Create a TextureOptions with all fields initialized to reasonable
    /// defaults.
    TextureOptions();

    /// Convert a TextureOpt for one point into a TextureOptions with
    /// uninform values.
    TextureOptions(const TextureOpt& opt);

    // Options that must be the same for all points we're texturing at once
    int firstchannel;          ///< First channel of the lookup
    int subimage;              ///< Subimage or face ID
    ustring subimagename;      ///< Subimage name
    Wrap swrap;                ///< Wrap mode in the s direction
    Wrap twrap;                ///< Wrap mode in the t direction
    MipMode mipmode;           ///< Mip mode
    InterpMode interpmode;     ///< Interpolation mode
    int anisotropic;           ///< Maximum anisotropic ratio
    bool conservative_filter;  ///< True == over-blur rather than alias

    // Options that may be different for each point we're texturing
    VaryingRef<float> sblur, tblur;    ///< Blur amount
    VaryingRef<float> swidth, twidth;  ///< Multiplier for derivatives
    VaryingRef<float> time;            ///< Time
    VaryingRef<float> bias;            ///< Bias
    VaryingRef<float> fill;            ///< Fill value for missing channels
    VaryingRef<float> missingcolor;    ///< Color for missing texture
    VaryingRef<int> samples;           ///< Number of samples

    // For 3D volume texture lookups only:
    Wrap rwrap;                ///< Wrap mode in the r direction
    VaryingRef<float> rblur;   ///< Blur amount in the r direction
    VaryingRef<float> rwidth;  ///< Multiplier for derivatives in r direction

    /// Utility: Return the Wrap enum corresponding to a wrap name:
    /// "default", "black", "clamp", "periodic", "mirror".
    static Wrap decode_wrapmode(const char* name)
    {
        return (Wrap)Tex::decode_wrapmode(name);
    }
    static Wrap decode_wrapmode(ustring name)
    {
        return (Wrap)Tex::decode_wrapmode(name);
    }

    /// Utility: Parse a single wrap mode (e.g., "periodic") or a
    /// comma-separated wrap modes string (e.g., "black,clamp") into
    /// separate Wrap enums for s and t.
    static void parse_wrapmodes(const char* wrapmodes,
                                TextureOptions::Wrap& swrapcode,
                                TextureOptions::Wrap& twrapcode)
    {
        Tex::parse_wrapmodes(wrapmodes, *(Tex::Wrap*)&swrapcode,
                             *(Tex::Wrap*)&twrapcode);
    }

private:
    // Options set INTERNALLY by libtexture after the options are passed
    // by the user.  Users should not attempt to alter these!
    friend class pvt::TextureSystemImpl;
    friend class TextureOpt;
};




/// Define an API to an abstract class that that manages texture files,
/// caches of open file handles as well as tiles of texels so that truly
/// huge amounts of texture may be accessed by an application with low
/// memory footprint, and ways to perform antialiased texture, shadow
/// map, and environment map lookups.
class OIIO_API TextureSystem {
public:
    /// @{
    /// @name Creating and destroying a texture system
    ///
    /// TextureSystem is an abstract API described as a pure virtual class.
    /// The actual internal implementation is not exposed through the
    /// external API of OpenImageIO.  Because of this, you cannot construct
    /// or destroy the concrete implementation, so two static methods of
    /// TextureSystem are provided:

    /// Create a TextureSystem and return a pointer to it.  This should only
    /// be freed by passing it to TextureSystem::destroy()!
    ///
    /// @param  shared
    ///     If `shared` is `true`, the pointer returned will be a shared
    ///     TextureSystem, (so that multiple parts of an application that
    ///     request a TextureSystem will all end up with the same one, and
    ///     the same underlying ImageCache). If `shared` is `false`, a
    ///     completely unique TextureCache will be created and returned.
    ///
    /// @param  imagecache
    ///     If `shared` is `false` and `imagecache` is not `nullptr`, the
    ///     TextureSystem will use this as its underlying ImageCache. In
    ///     that case, it is the caller who is responsible for eventually
    ///     freeing the ImageCache after the TextureSystem is destroyed.  If
    ///     `shared` is `false` and `imagecache` is `nullptr`, then a custom
    ///     ImageCache will be created, owned by the TextureSystem, and
    ///     automatically freed when the TS destroys.
    ///
    /// @returns
    ///     A raw pointer to a TextureSystem, which can only be freed with
    ///     `TextureSystem::destroy()`.
    ///
    /// @see    TextureSystem::destroy
    static TextureSystem *create (bool shared=true,
                                  ImageCache *imagecache=nullptr);

    /// Destroy an allocated TextureSystem, including freeing all system
    /// resources that it holds.
    ///
    /// It is safe to destroy even a shared TextureSystem, as the
    /// implementation of `destroy()` will recognize a shared one and only
    /// truly release its resources if it has been requested to be destroyed
    /// as many times as shared TextureSystem's were created.
    ///
    /// @param  ts
    ///     Raw pointer to the TextureSystem to destroy.
    ///
    /// @param  teardown_imagecache
    ///     For a shared TextureSystem, if the `teardown_imagecache`
    ///     parameter is `true`, it will try to truly destroy the shared
    ///     cache if nobody else is still holding a reference (otherwise, it
    ///     will leave it intact). This parameter has no effect if `ts` was
    ///     not the single globally shared TextureSystem.
    static void destroy (TextureSystem *ts,
                         bool teardown_imagecache = false);

    /// @}


    /// @{
    /// @name Setting options and limits for the texture system
    ///
    /// These are the list of attributes that can bet set or queried by
    /// attribute/getattribute:
    ///
    /// All attributes ordinarily recognized by ImageCache are accepted and
    /// passed through to the underlying ImageCache. These include:
    /// - `int max_open_files` :
    ///             Maximum number of file handles held open.
    /// - `float max_memory_MB` :
    ///             Maximum tile cache size, in MB.
    /// - `string searchpath` :
    ///             Colon-separated search path for texture files.
    /// - `string plugin_searchpath` :
    ///             Colon-separated search path for plugins.
    /// - `int autotile` :
    ///             If >0, tile size to emulate for non-tiled images.
    /// - `int autoscanline` :
    ///             If nonzero, autotile using full width tiles.
    /// - `int automip` :
    ///             If nonzero, emulate mipmap on the fly.
    /// - `int accept_untiled` :
    ///             If nonzero, accept untiled images.
    /// - `int accept_unmipped` :
    ///             If nonzero, accept unmipped images.
    /// - `int failure_retries` :
    ///             How many times to retry a read failure.
    /// - `int deduplicate` :
    ///             If nonzero, detect duplicate textures (default=1).
    /// - `string substitute_image` :
    ///             If supplied, an image to substatute for all texture
    ///             references.
    /// - `int max_errors_per_file` :
    ///             Limits how many errors to issue for each file. (default:
    ///             100)
    ///
    /// Texture-specific settings:
    /// - `matrix44 worldtocommon` / `matrix44 commontoworld` :
    ///             The 4x4 matrices that provide the spatial transformation
    ///             from "world" to a "common" coordinate system and back.
    ///             This is mainly used for shadow map lookups, in which the
    ///             shadow map itself encodes the world coordinate system,
    ///             but positions passed to `shadow()` are expressed in
    ///             "common" coordinates. You do not need to set
    ///             `commontoworld` and `worldtocommon` separately; just
    ///             setting either one will implicitly set the other, since
    ///             each is the inverse of the other.
    /// - `int gray_to_rgb` :
    ///             If set to nonzero, texture lookups of single-channel
    ///             (grayscale) images will replicate the sole channel's
    ///             values into the next two channels, making it behave like
    ///             an RGB image that happens to have all three channels
    ///             with identical pixel values.  (Channels beyond the third
    ///             will get the "fill" value.) The default value of zero
    ///             means that all missing channels will get the "fill"
    ///             color.
    /// - `int max_tile_channels` :
    ///             The maximum number of color channels in a texture file
    ///             for which all channels will be loaded as a single cached
    ///             tile. Files with more than this number of color channels
    ///             will have only the requested subset loaded, in order to
    ///             save cache space (but at the possible wasted expense of
    ///             separate tiles that overlap their channel ranges). The
    ///             default is 5.
    /// - `int max_mip_res` :
    ///             **NEW 2.1** Sets the maximum MIP-map resolution for
    ///             filtered texture lookups. The MIP levels used will be
    ///             clamped to those having fewer than this number of pixels
    ///             in each dimension. This can be helpful as a way to limit
    ///             disk I/O when doing fast preview renders (with the
    ///             tradeoff that you may see some texture more blurry than
    ///             they would ideally be). The default is `1 << 30`, a
    ///             value so large that no such clamping will be performed.
    /// - `string latlong_up` :
    ///             The default "up" direction for latlong environment maps
    ///             (only applies if the map itself doesn't specify a format
    ///             or is in a format that explicitly requires a particular
    ///             orientation).  The default is `"y"`.  (Currently any
    ///             other value will result in *z* being "up.")
    /// - `int flip_t` :
    ///             If nonzero, `t` coordinates will be flipped `1-t` for
    ///             all texture lookups. The default is 0.
    ///
    /// - `string options`
    ///             This catch-all is simply a comma-separated list of
    ///             `name=value` settings of named options, which will be
    ///             parsed and individually set.
    ///
    ///                 ic->attribute ("options", "max_memory_MB=512.0,autotile=1");
    ///
    ///             Note that if an option takes a string value that must
    ///             itself contain a comma, it is permissible to enclose the
    ///             value in either single `\'` or double `"` quotes.
    ///
    /// **Read-only attributes**
    ///
    /// Additionally, there are some read-only attributes that can be
    /// queried with `getattribute()` even though they cannot be set via
    /// `attribute()`:
    ///
    ///
    /// The following member functions of TextureSystem allow you to set
    /// (and in some cases retrieve) options that control the overall
    /// behavior of the texture system:

    /// Set a named attribute (i.e., a property or option) of the
    /// TextureSystem.
    ///
    /// Example:
    ///
    ///     TextureSystem *ts;
    ///     ...
    ///     int maxfiles = 50;
    ///     ts->attribute ("max_open_files", TypeDesc::INT, &maxfiles);
    ///
    ///     const char *path = "/my/path";
    ///     ts->attribute ("searchpath", TypeDesc::STRING, &path);
    ///
    ///     // There are specialized versions for retrieving a single int,
    ///     // float, or string without needing types or pointers:
    ///     ts->getattribute ("max_open_files", 50);
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
    virtual bool attribute (string_view name, TypeDesc type, const void *val) = 0;

    /// Specialized `attribute()` for setting a single `int` value.
    virtual bool attribute (string_view name, int val) = 0;
    /// Specialized `attribute()` for setting a single `float` value.
    virtual bool attribute (string_view name, float val) = 0;
    virtual bool attribute (string_view name, double val) = 0;
    /// Specialized `attribute()` for setting a single string value.
    virtual bool attribute (string_view name, string_view val) = 0;

    /// Get the named attribute of the texture system, store it in `*val`.
    /// All of the attributes that may be set with the `attribute() call`
    /// may also be queried with `getattribute()`.
    ///
    /// Examples:
    ///
    ///     TextureSystem *ic;
    ///     ...
    ///     int maxfiles;
    ///     ts->getattribute ("max_open_files", TypeDesc::INT, &maxfiles);
    ///
    ///     const char *path;
    ///     ts->getattribute ("searchpath", TypeDesc::STRING, &path);
    ///
    ///     // There are specialized versions for retrieving a single int,
    ///     // float, or string without needing types or pointers:
    ///     int maxfiles;
    ///     ts->getattribute ("max_open_files", maxfiles);
    ///     const char *path;
    ///     ts->getattribute ("searchpath", &path);
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
    virtual bool getattribute (string_view name,
                               TypeDesc type, void *val) const = 0;

    /// Specialized `attribute()` for retrieving a single `int` value.
    virtual bool getattribute(string_view name, int& val) const = 0;
    /// Specialized `attribute()` for retrieving a single `float` value.
    virtual bool getattribute(string_view name, float& val) const = 0;
    virtual bool getattribute(string_view name, double& val) const = 0;
    /// Specialized `attribute()` for retrieving a single `string` value
    /// as a `char*`.
    virtual bool getattribute(string_view name, char** val) const = 0;
    /// Specialized `attribute()` for retrieving a single `string` value
    /// as a `std::string`.
    virtual bool getattribute(string_view name, std::string& val) const = 0;

    /// @}

    /// @{
    /// @name Opaque data for performance lookups
    ///
    /// The TextureSystem implementation needs to maintain certain
    /// per-thread state, and some methods take an opaque `Perthread`
    /// pointer to this record. There are three options for how to deal with
    /// it:
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
    /// certain per-thread information that the TextureSystem maintains. Any
    /// given one of these should NEVER be shared between running threads.
    class Perthread;

    /// Retrieve a Perthread, unique to the calling thread. This is a
    /// thread-specific pointer that will always return the Perthread for a
    /// thread, which will also be automatically destroyed when the thread
    /// terminates.
    ///
    /// Applications that want to manage their own Perthread pointers (with
    /// `create_thread_info` and `destroy_thread_info`) should still call
    /// this, but passing in their managed pointer. If the passed-in
    /// thread_info is not nullptr, it won't create a new one or retrieve a
    /// TSP, but it will do other necessary housekeeping on the Perthread
    /// information.
    virtual Perthread* get_perthread_info(Perthread* thread_info = nullptr) = 0;

    /// Create a new Perthread. It is the caller's responsibility to
    /// eventually destroy it using `destroy_thread_info()`.
    virtual Perthread* create_thread_info() = 0;

    /// Destroy a Perthread that was allocated by `create_thread_info()`.
    virtual void destroy_thread_info(Perthread* threadinfo) = 0;

    /// Define an opaque data type that allows us to have a handle to a
    /// texture (already having its name resolved) but without exposing
    /// any internals.
    class TextureHandle;

    /// Retrieve an opaque handle for fast texture lookups.  The opaque
    /// pointer `thread_info` is thread-specific information returned by
    /// `get_perthread_info()`.  Return nullptr if something has gone
    /// horribly wrong.
    virtual TextureHandle * get_texture_handle (ustring filename,
                                            Perthread *thread_info=nullptr) = 0;

    /// Return true if the texture handle (previously returned by
    /// `get_image_handle()`) is a valid texture that can be subsequently
    /// read.
    virtual bool good(TextureHandle* texture_handle) = 0;

    /// @}

    /// @{
    /// @name   Texture lookups
    ///

    /// Perform a filtered 2D texture lookup on a position centered at 2D
    /// coordinates (`s`, `t`) from the texture identified by `filename`,
    /// and using relevant texture `options`.  The `nchannels` parameter
    /// determines the number of channels to retrieve (e.g., 1 for a single
    /// value, 3 for an RGB triple, etc.). The filtered results will be
    /// stored in `result[0..nchannels-1]`.
    ///
    /// We assume that this lookup will be part of an image that has pixel
    /// coordinates `x` and `y`.  By knowing how `s` and `t` change from
    /// pixel to pixel in the final image, we can properly *filter* or
    /// antialias the texture lookups.  This information is given via
    /// derivatives `dsdx` and `dtdx` that define the change in `s` and `t`
    /// per unit of `x`, and `dsdy` and `dtdy` that define the change in `s`
    /// and `t` per unit of `y`.  If it is impossible to know the
    /// derivatives, you may pass 0 for them, but in that case you will not
    /// receive an antialiased texture lookup.
    ///
    /// @param  filename
    ///             The name of the texture.
    /// @param  options
    ///     Fields within `options` that are honored for 2D texture lookups
    ///     include the following:
    ///     - `int firstchannel` :
    ///             The index of the first channel to look up from the texture.
    ///     - `int subimage / ustring subimagename` :
    ///             The subimage or face within the file, specified by
    ///             either by name (if non-empty) or index. This will be
    ///             ignored if the file does not have multiple subimages or
    ///             separate per-face textures.
    ///     - `Wrap swrap, twrap` :
    ///             Specify the *wrap mode* for each direction, one of:
    ///             `WrapBlack`, `WrapClamp`, `WrapPeriodic`, `WrapMirror`,
    ///             or `WrapDefault`.
    ///     - `float swidth, twidth` :
    ///             For each direction, gives a multiplier for the derivatives.
    ///     - `float sblur, tblur` :
    ///             For each direction, specifies an additional amount of
    ///             pre-blur to apply to the texture (*after* derivatives
    ///             are taken into account), expressed as a portion of the
    ///             width of the texture.
    ///     - `float fill` :
    ///             Specifies the value that will be used for any color
    ///             channels that are requested but not found in the file.
    ///             For example, if you perform a 4-channel lookup on a
    ///             3-channel texture, the last channel will get the fill
    ///             value.  (Note: this behavior is affected by the
    ///             `"gray_to_rgb"` TextureSystem attribute.
    ///     - `const float *missingcolor` :
    ///             If not `nullptr`, specifies the color that will be
    ///             returned for missing or broken textures (rather than
    ///             being an error).
    /// @param  s/t
    ///             The 2D texture coordinates.
    /// @param  dsdx,dtdx,dsdy,dtdy
    ///             The differentials of s and t relative to canonical
    ///             directions x and y.  The choice of x and y are not
    ///             important to the implementation; it can be any imposed
    ///             2D coordinates, such as pixels in screen space, adjacent
    ///             samples in parameter space on a surface, etc. The st
    ///             derivatives determine the size and shape of the
    ///             ellipsoid over which the texture lookup is filtered.
    /// @param  nchannels
    ///             The number of channels of data to retrieve into `result`
    ///             (e.g., 1 for a single value, 3 for an RGB triple, etc.).
    /// @param  result[]
    ///             The result of the filtered texture lookup will be placed
    ///             into `result[0..nchannels-1]`.
    /// @param  dresultds/dresultdt
    ///             If non-null, these designate storage locations for the
    ///             derivatives of result, i.e., the rate of change per unit
    ///             s and t, respectively, of the filtered texture. If
    ///             supplied, they must allow for `nchannels` of storage.
    /// @returns
    ///             `true` upon success, or `false` if the file was not
    ///             found or could not be opened by any available ImageIO
    ///             plugin.
    ///
    virtual bool texture (ustring filename, TextureOpt &options,
                          float s, float t, float dsdx, float dtdx,
                          float dsdy, float dtdy,
                          int nchannels, float *result,
                          float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    /// Slightly faster version of texture() lookup if the app already has a
    /// texture handle and per-thread info.
    virtual bool texture (TextureHandle *texture_handle,
                          Perthread *thread_info, TextureOpt &options,
                          float s, float t, float dsdx, float dtdx,
                          float dsdy, float dtdy,
                          int nchannels, float *result,
                          float *dresultds=nullptr, float *dresultdt=nullptr) = 0;


    /// Perform a filtered 3D volumetric texture lookup on a position
    /// centered at 3D position `P` (with given differentials) from the
    /// texture identified by `filename`, and using relevant texture
    /// `options`.  The filtered results will be stored in
    /// `result[0..nchannels-1]`.
    ///
    /// The `P` coordinate and `dPdx`, `dPdy`, and `dPdz` derivatives are
    /// assumed to be in some kind of common global coordinate system
    /// (usually "world" space) and will be automatically transformed into
    /// volume local coordinates, if such a transformation is specified in
    /// the volume file itself.
    ///
    /// @param  filename
    ///             The name of the texture.
    /// @param  options
    ///     Fields within `options` that are honored for 3D texture lookups
    ///     include the following:
    ///     - `int firstchannel` :
    ///             The index of the first channel to look up from the texture.
    ///     - `int subimage / ustring subimagename` :
    ///             The subimage or field within the volume, specified by
    ///             either by name (if non-empty) or index. This will be
    ///             ignored if the file does not have multiple subimages or
    ///             separate per-face textures.
    ///     - `Wrap swrap, twrap, rwrap` :
    ///             Specify the *wrap mode* for each direction, one of:
    ///             `WrapBlack`, `WrapClamp`, `WrapPeriodic`, `WrapMirror`,
    ///             or `WrapDefault`.
    ///     - `float swidth, twidth, rwidth` :
    ///             For each direction, gives a multiplier for the derivatives.
    ///     - `float sblur, tblur, rblur` :
    ///             For each direction, specifies an additional amount of
    ///             pre-blur to apply to the texture (*after* derivatives
    ///             are taken into account), expressed as a portion of the
    ///             width of the texture.
    ///     - `float fill` :
    ///             Specifies the value that will be used for any color
    ///             channels that are requested but not found in the file.
    ///             For example, if you perform a 4-channel lookup on a
    ///             3-channel texture, the last channel will get the fill
    ///             value.  (Note: this behavior is affected by the
    ///             `"gray_to_rgb"` TextureSystem attribute.
    ///     - `const float *missingcolor` :
    ///             If not `nullptr`, specifies the color that will be
    ///             returned for missing or broken textures (rather than
    ///             being an error).
    ///     - `float time` :
    ///             A time value to use if the volume texture specifies a
    ///             time-varying local transformation (default: 0).
    /// @param  P
    ///             The 2D texture coordinates.
    /// @param  dPdx/dPdy/dPdz
    ///             The differentials of `P`. We assume that this lookup
    ///             will be part of an image that has pixel coordinates `x`
    ///             and `y` and depth `z`. By knowing how `P` changes from
    ///             pixel to pixel in the final image, and as we step in *z*
    ///             depth, we can properly *filter* or antialias the texture
    ///             lookups.  This information is given via derivatives
    ///             `dPdx`, `dPdy`, and `dPdz` that define the changes in
    ///             `P` per unit of `x`, `y`, and `z`, respectively.  If it
    ///             is impossible to know the derivatives, you may pass 0
    ///             for them, but in that case you will not receive an
    ///             antialiased texture lookup.
    /// @param  nchannels
    ///             The number of channels of data to retrieve into `result`
    ///             (e.g., 1 for a single value, 3 for an RGB triple, etc.).
    /// @param  result[]
    ///             The result of the filtered texture lookup will be placed
    ///             into `result[0..nchannels-1]`.
    /// @param  dresultds/dresultdt/dresultdr
    ///             If non-null, these designate storage locations for the
    ///             derivatives of result, i.e., the rate of change per unit
    ///             s, t, and r, respectively, of the filtered texture. If
    ///             supplied, they must allow for `nchannels` of storage.
    /// @returns
    ///             `true` upon success, or `false` if the file was not
    ///             found or could not be opened by any available ImageIO
    ///             plugin.
    ///
    virtual bool texture3d (ustring filename, TextureOpt &options,
                            const Imath::V3f &P, const Imath::V3f &dPdx,
                            const Imath::V3f &dPdy, const Imath::V3f &dPdz,
                            int nchannels, float *result,
                            float *dresultds=nullptr, float *dresultdt=nullptr,
                            float *dresultdr=nullptr) = 0;

    /// Slightly faster version of texture3d() lookup if the app already has
    /// a texture handle and per-thread info.
    virtual bool texture3d (TextureHandle *texture_handle,
                            Perthread *thread_info, TextureOpt &options,
                            const Imath::V3f &P, const Imath::V3f &dPdx,
                            const Imath::V3f &dPdy, const Imath::V3f &dPdz,
                            int nchannels, float *result,
                            float *dresultds=nullptr, float *dresultdt=nullptr,
                            float *dresultdr=nullptr) = 0;


    // Retrieve a shadow lookup for a single position P.
    //
    // Return true if the file is found and could be opened by an
    // available ImageIO plugin, otherwise return false.
    virtual bool shadow (ustring filename, TextureOpt &options,
                         const Imath::V3f &P, const Imath::V3f &dPdx,
                         const Imath::V3f &dPdy, float *result,
                         float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    // Slightly faster version of texture3d() lookup if the app already
    // has a texture handle and per-thread info.
    virtual bool shadow (TextureHandle *texture_handle, Perthread *thread_info,
                         TextureOpt &options,
                         const Imath::V3f &P, const Imath::V3f &dPdx,
                         const Imath::V3f &dPdy, float *result,
                         float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    /// Perform a filtered directional environment map lookup in the
    /// direction of vector `R`, from the texture identified by `filename`,
    /// and using relevant texture `options`.  The filtered results will be
    /// stored in `result[]`.
    ///
    /// @param  filename
    ///             The name of the texture.
    /// @param  options
    ///     Fields within `options` that are honored for environment lookups
    ///     include the following:
    ///     - `int firstchannel` :
    ///             The index of the first channel to look up from the texture.
    ///     - `int subimage / ustring subimagename` :
    ///             The subimage or face within the file, specified by
    ///             either by name (if non-empty) or index. This will be
    ///             ignored if the file does not have multiple subimages or
    ///             separate per-face textures.
    ///     - `float swidth, twidth` :
    ///             For each direction, gives a multiplier for the
    ///             derivatives.
    ///     - `float sblur, tblur` :
    ///             For each direction, specifies an additional amount of
    ///             pre-blur to apply to the texture (*after* derivatives
    ///             are taken into account), expressed as a portion of the
    ///             width of the texture.
    ///     - `float fill` :
    ///             Specifies the value that will be used for any color
    ///             channels that are requested but not found in the file.
    ///             For example, if you perform a 4-channel lookup on a
    ///             3-channel texture, the last channel will get the fill
    ///             value.  (Note: this behavior is affected by the
    ///             `"gray_to_rgb"` TextureSystem attribute.
    ///     - `const float *missingcolor` :
    ///             If not `nullptr`, specifies the color that will be
    ///             returned for missing or broken textures (rather than
    ///             being an error).
    /// @param  R
    ///             The direction vector to look up.
    /// @param  dRdx/dRdy
    ///             The differentials of `R` with respect to image
    ///             coordinates x and y.
    /// @param  nchannels
    ///             The number of channels of data to retrieve into `result`
    ///             (e.g., 1 for a single value, 3 for an RGB triple, etc.).
    /// @param  result[]
    ///             The result of the filtered texture lookup will be placed
    ///             into `result[0..nchannels-1]`.
    /// @param  dresultds/dresultdt
    ///             If non-null, these designate storage locations for the
    ///             derivatives of result, i.e., the rate of change per unit
    ///             s and t, respectively, of the filtered texture. If
    ///             supplied, they must allow for `nchannels` of storage.
    /// @returns
    ///             `true` upon success, or `false` if the file was not
    ///             found or could not be opened by any available ImageIO
    ///             plugin.
    virtual bool environment (ustring filename, TextureOpt &options,
                              const Imath::V3f &R, const Imath::V3f &dRdx,
                              const Imath::V3f &dRdy, int nchannels, float *result,
                              float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    /// Slightly faster version of environment() if the app already has a
    /// texture handle and per-thread info.
    virtual bool environment (TextureHandle *texture_handle,
                              Perthread *thread_info, TextureOpt &options,
                              const Imath::V3f &R, const Imath::V3f &dRdx,
                              const Imath::V3f &dRdy, int nchannels, float *result,
                              float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    /// @}

    /// @{
    /// @name   Batched texture lookups
    ///

    /// Perform filtered 2D texture lookups on a batch of positions from the
    /// same texture, all at once.  The parameters `s`, `t`, `dsdx`, `dtdx`,
    /// and `dsdy`, `dtdy` are each a pointer to `[BatchWidth]` values.  The
    /// `mask` determines which of those array elements to actually compute.
    ///
    /// The float* results act like `float[nchannels][BatchWidth]`, so that
    /// effectively `result[0..BatchWidth-1]` are the "red" result for each
    /// lane, `result[BatchWidth..2*BatchWidth-1]` are the "green" results,
    /// etc. The `dresultds` and `dresultdt` should either both be provided,
    /// or else both be nullptr (meaning no derivative results are
    /// required).
    ///
    /// @param  filename
    ///             The name of the texture.
    /// @param  options
    ///             A TextureOptBatch containing texture lookup options.
    ///             This is conceptually the same as a TextureOpt, but the
    ///             following fields are arrays of `[BatchWidth]` elements:
    ///             sblur, tblur, swidth, twidth. The other fields are, as
    ///             with TextureOpt, ordinary scalar values.
    /// @param  mask
    ///             A bit-field designating which "lanes" should be
    ///             computed: if `mask & (1<<i)` is nonzero, then results
    ///             should be computed and stored for `result[...][i]`.
    /// @param  s/t
    ///             Pointers to the 2D texture coordinates, each as a
    ///             `float[BatchWidth]`.
    /// @param  dsdx/dtdx/dsdy/dtdy
    ///             The differentials of s and t relative to canonical
    ///             directions x and y, each as a `float[BatchWidth]`.
    /// @param  nchannels
    ///             The number of channels of data to retrieve into `result`
    ///             (e.g., 1 for a single value, 3 for an RGB triple, etc.).
    /// @param  result[]
    ///             The result of the filtered texture lookup will be placed
    ///             here, as `float [nchannels][BatchWidth]`. (Note the
    ///             "SOA" data layout.)
    /// @param  dresultds/dresultdt
    ///             If non-null, these designate storage locations for the
    ///             derivatives of result, and like `result` are in SOA
    ///             layout: `float [nchannels][BatchWidth]`
    /// @returns
    ///             `true` upon success, or `false` if the file was not
    ///             found or could not be opened by any available ImageIO
    ///             plugin.
    ///
    virtual bool texture (ustring filename, TextureOptBatch &options,
                          Tex::RunMask mask, const float *s, const float *t,
                          const float *dsdx, const float *dtdx,
                          const float *dsdy, const float *dtdy,
                          int nchannels, float *result,
                          float *dresultds=nullptr,
                          float *dresultdt=nullptr) = 0;
    /// Slightly faster version of texture() lookup if the app already has a
    /// texture handle and per-thread info.
    virtual bool texture (TextureHandle *texture_handle,
                          Perthread *thread_info, TextureOptBatch &options,
                          Tex::RunMask mask, const float *s, const float *t,
                          const float *dsdx, const float *dtdx,
                          const float *dsdy, const float *dtdy,
                          int nchannels, float *result,
                          float *dresultds=nullptr,
                          float *dresultdt=nullptr) = 0;

    // Old multi-point API call.
    // DEPRECATED (1.8)
    virtual bool texture (ustring filename, TextureOptions &options,
                          Runflag *runflags, int beginactive, int endactive,
                          VaryingRef<float> s, VaryingRef<float> t,
                          VaryingRef<float> dsdx, VaryingRef<float> dtdx,
                          VaryingRef<float> dsdy, VaryingRef<float> dtdy,
                          int nchannels, float *result,
                          float *dresultds=nullptr, float *dresultdt=nullptr) = 0;
    virtual bool texture (TextureHandle *texture_handle,
                          Perthread *thread_info, TextureOptions &options,
                          Runflag *runflags, int beginactive, int endactive,
                          VaryingRef<float> s, VaryingRef<float> t,
                          VaryingRef<float> dsdx, VaryingRef<float> dtdx,
                          VaryingRef<float> dsdy, VaryingRef<float> dtdy,
                          int nchannels, float *result,
                          float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    /// Perform filtered 3D volumetric texture lookups on a batch of
    /// positions from the same texture, all at once. The "point-like"
    /// parameters `P`, `dPdx`, `dPdy`, and `dPdz` are each a pointers to
    /// arrays of `float value[3][BatchWidth]` (or alternately like
    /// `Imath::Vec3<FloatWide>`). That is, each one points to all the *x*
    /// values for the batch, immediately followed by all the *y* values,
    /// followed by the *z* values. The `mask` determines which of those
    /// array elements to actually compute.
    ///
    /// The various results arrays are also arranged as arrays that behave
    /// as if they were declared `float result[channels][BatchWidth]`, where
    /// all the batch values for channel 0 are adjacent, followed by all the
    /// batch values for channel 1, etc.
    ///
    /// @param  filename
    ///             The name of the texture.
    /// @param  options
    ///             A TextureOptBatch containing texture lookup options.
    ///             This is conceptually the same as a TextureOpt, but the
    ///             following fields are arrays of `[BatchWidth]` elements:
    ///             sblur, tblur, swidth, twidth. The other fields are, as
    ///             with TextureOpt, ordinary scalar values.
    /// @param  mask
    ///             A bit-field designating which "lanes" should be
    ///             computed: if `mask & (1<<i)` is nonzero, then results
    ///             should be computed and stored for `result[...][i]`.
    /// @param  P
    ///             Pointers to the 3D texture coordinates, each as a
    ///             `float[3][BatchWidth]`.
    /// @param  dPdx/dPdy/dPdz
    ///             The differentials of P relative to canonical directions
    ///             x, y, and z, each as a `float[3][BatchWidth]`.
    /// @param  nchannels
    ///             The number of channels of data to retrieve into `result`
    ///             (e.g., 1 for a single value, 3 for an RGB triple, etc.).
    /// @param  result[]
    ///             The result of the filtered texture lookup will be placed
    ///             here, as `float [nchannels][BatchWidth]`. (Note the
    ///             "SOA" data layout.)
    /// @param  dresultds/dresultdt/dresultdr
    ///             If non-null, these designate storage locations for the
    ///             derivatives of result, and like `result` are in SOA
    ///             layout: `float [nchannels][BatchWidth]`
    /// @returns
    ///             `true` upon success, or `false` if the file was not
    ///             found or could not be opened by any available ImageIO
    ///             plugin.
    ///
    virtual bool texture3d (ustring filename,
                            TextureOptBatch &options, Tex::RunMask mask,
                            const float *P, const float *dPdx,
                            const float *dPdy, const float *dPdz,
                            int nchannels, float *result,
                            float *dresultds=nullptr, float *dresultdt=nullptr,
                            float *dresultdr=nullptr) = 0;
    /// Slightly faster version of texture3d() lookup if the app already
    /// has a texture handle and per-thread info.
    virtual bool texture3d (TextureHandle *texture_handle,
                            Perthread *thread_info,
                            TextureOptBatch &options, Tex::RunMask mask,
                            const float *P, const float *dPdx,
                            const float *dPdy, const float *dPdz,
                            int nchannels, float *result,
                            float *dresultds=nullptr, float *dresultdt=nullptr,
                            float *dresultdr=nullptr) = 0;

    // Retrieve a 3D texture lookup at many points at once.
    // DEPRECATED(1.8)
    virtual bool texture3d (ustring filename, TextureOptions &options,
                            Runflag *runflags, int beginactive, int endactive,
                            VaryingRef<Imath::V3f> P,
                            VaryingRef<Imath::V3f> dPdx,
                            VaryingRef<Imath::V3f> dPdy,
                            VaryingRef<Imath::V3f> dPdz,
                            int nchannels, float *result,
                            float *dresultds=nullptr, float *dresultdt=nullptr,
                            float *dresultdr=nullptr) = 0;
    virtual bool texture3d (TextureHandle *texture_handle,
                            Perthread *thread_info, TextureOptions &options,
                            Runflag *runflags, int beginactive, int endactive,
                            VaryingRef<Imath::V3f> P,
                            VaryingRef<Imath::V3f> dPdx,
                            VaryingRef<Imath::V3f> dPdy,
                            VaryingRef<Imath::V3f> dPdz,
                            int nchannels, float *result,
                            float *dresultds=nullptr, float *dresultdt=nullptr,
                            float *dresultdr=nullptr) = 0;

    /// Perform filtered directional environment map lookups on a batch of
    /// directions from the same texture, all at once. The "point-like"
    /// parameters `R`, `dRdx`, and `dRdy` are each a pointers to arrays of
    /// `float value[3][BatchWidth]` (or alternately like
    /// `Imath::Vec3<FloatWide>`). That is, each one points to all the *x*
    /// values for the batch, immediately followed by all the *y* values,
    /// followed by the *z* values. The `mask` determines which of those
    /// array elements to actually compute.
    ///
    /// The various results arrays are also arranged as arrays that behave
    /// as if they were declared `float result[channels][BatchWidth]`, where
    /// all the batch values for channel 0 are adjacent, followed by all the
    /// batch values for channel 1, etc.
    ///
    /// @param  filename
    ///             The name of the texture.
    /// @param  options
    ///             A TextureOptBatch containing texture lookup options.
    ///             This is conceptually the same as a TextureOpt, but the
    ///             following fields are arrays of `[BatchWidth]` elements:
    ///             sblur, tblur, swidth, twidth. The other fields are, as
    ///             with TextureOpt, ordinary scalar values.
    /// @param  mask
    ///             A bit-field designating which "lanes" should be
    ///             computed: if `mask & (1<<i)` is nonzero, then results
    ///             should be computed and stored for `result[...][i]`.
    /// @param  R
    ///             Pointers to the 3D texture coordinates, each as a
    ///             `float[3][BatchWidth]`.
    /// @param  dRdx/dRdy
    ///             The differentials of R relative to canonical directions
    ///             x and y, each as a `float[3][BatchWidth]`.
    /// @param  nchannels
    ///             The number of channels of data to retrieve into `result`
    ///             (e.g., 1 for a single value, 3 for an RGB triple, etc.).
    /// @param  result[]
    ///             The result of the filtered texture lookup will be placed
    ///             here, as `float [nchannels][BatchWidth]`. (Note the
    ///             "SOA" data layout.)
    /// @param  dresultds/dresultdt
    ///             If non-null, these designate storage locations for the
    ///             derivatives of result, and like `result` are in SOA
    ///             layout: `float [nchannels][BatchWidth]`
    /// @returns
    ///             `true` upon success, or `false` if the file was not
    ///             found or could not be opened by any available ImageIO
    ///             plugin.
    ///
    virtual bool environment (ustring filename,
                              TextureOptBatch &options, Tex::RunMask mask,
                              const float *R, const float *dRdx, const float *dRdy,
                              int nchannels, float *result,
                              float *dresultds=nullptr, float *dresultdt=nullptr) = 0;
    /// Slightly faster version of environment() if the app already has a
    /// texture handle and per-thread info.
    virtual bool environment (TextureHandle *texture_handle, Perthread *thread_info,
                              TextureOptBatch &options, Tex::RunMask mask,
                              const float *R, const float *dRdx, const float *dRdy,
                              int nchannels, float *result,
                              float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    // Retrieve an environment map lookup for direction R, for many
    // points at once.
    // DEPRECATED(1.8)
    virtual bool environment (ustring filename, TextureOptions &options,
                              Runflag *runflags, int beginactive, int endactive,
                              VaryingRef<Imath::V3f> R,
                              VaryingRef<Imath::V3f> dRdx,
                              VaryingRef<Imath::V3f> dRdy,
                              int nchannels, float *result,
                              float *dresultds=nullptr, float *dresultdt=nullptr) = 0;
    virtual bool environment (TextureHandle *texture_handle,
                              Perthread *thread_info, TextureOptions &options,
                              Runflag *runflags, int beginactive, int endactive,
                              VaryingRef<Imath::V3f> R,
                              VaryingRef<Imath::V3f> dRdx,
                              VaryingRef<Imath::V3f> dRdy,
                              int nchannels, float *result,
                              float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    // Batched shadow lookups
    virtual bool shadow (ustring filename,
                         TextureOptBatch &options, Tex::RunMask mask,
                         const float *P, const float *dPdx, const float *dPdy,
                         float *result, float *dresultds=nullptr, float *dresultdt=nullptr) = 0;
    virtual bool shadow (TextureHandle *texture_handle, Perthread *thread_info,
                         TextureOptBatch &options, Tex::RunMask mask,
                         const float *P, const float *dPdx, const float *dPdy,
                         float *result, float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    // Retrieve a shadow lookup for position P at many points at once.
    // DEPRECATED(1.8)
    virtual bool shadow (ustring filename, TextureOptions &options,
                         Runflag *runflags, int beginactive, int endactive,
                         VaryingRef<Imath::V3f> P,
                         VaryingRef<Imath::V3f> dPdx,
                         VaryingRef<Imath::V3f> dPdy,
                         float *result,
                         float *dresultds=nullptr, float *dresultdt=nullptr) = 0;
    virtual bool shadow (TextureHandle *texture_handle, Perthread *thread_info,
                         TextureOptions &options,
                         Runflag *runflags, int beginactive, int endactive,
                         VaryingRef<Imath::V3f> P,
                         VaryingRef<Imath::V3f> dPdx,
                         VaryingRef<Imath::V3f> dPdy,
                         float *result,
                         float *dresultds=nullptr, float *dresultdt=nullptr) = 0;

    /// @}


    /// @{
    /// @name   Texture metadata and raw texels
    ///

    /// Given possibly-relative 'filename', resolve it using the search
    /// path rules and return the full resolved filename.
    virtual std::string resolve_filename (const std::string &filename) const=0;

    /// Get information or metadata about the named texture and store it in
    /// `*data`.
    ///
    /// Data names may include any of the following:
    ///
    ///   - `exists` (int):
    ///         Stores the value 1 if the file exists and is an image format
    ///         that OpenImageIO can read, or 0 if the file does not exist,
    ///         or could not be properly read as a texture. Note that unlike
    ///         all other queries, this query will "succeed" (return `true`)
    ///         even if the file does not exist.
    ///
    ///   - `udim` (int) :
    ///         Stores the value 1 if the file is a "virtual UDIM" or
    ///         texture atlas file (as described in Section
    ///         :ref:`sec-texturesys-udim`) or 0 otherwise.
    ///
    ///   - `subimages` (int) :
    ///         The number of subimages/faces in the file, as an integer.
    ///
    ///   - `resolution` (int[2] or int[3]):
    ///         The resolution of the texture file, if an array of 2
    ///         integers (described as `TypeDesc(INT,2)`), or the 3D
    ///         resolution of the texture file, which is an array of 3
    ///         integers (described as `TypeDesc(INT,3)`)  The third value
    ///         will be 1 unless it's a volumetric (3D) image.
    ///
    ///   - `miplevels` (int) :
    ///         The number of MIPmap levels for the specified
    ///         subimage (an integer).
    ///
    ///   - `texturetype` (string) :
    ///         A string describing the type of texture of the given file,
    ///         which describes how the texture may be used (also which
    ///         texture API call is probably the right one for it). This
    ///         currently may return one of: "unknown", "Plain Texture",
    ///         "Volume Texture", "Shadow", or "Environment".
    ///
    ///   - `textureformat` (string) :
    ///         A string describing the format of the given file, which
    ///         describes the kind of texture stored in the file. This
    ///         currently may return one of: "unknown", "Plain Texture",
    ///         "Volume Texture", "Shadow", "CubeFace Shadow", "Volume
    ///         Shadow", "LatLong Environment", or "CubeFace Environment".
    ///         Note that there are several kinds of shadows and environment
    ///         maps, all accessible through the same API calls.
    ///
    ///   - `channels` (int) :
    ///         The number of color channels in the file.
    ///
    ///   - `format` (int) :
    ///         The native data format of the pixels in the file (an
    ///         integer, giving the `TypeDesc::BASETYPE` of the data). Note
    ///         that this is not necessarily the same as the data format
    ///         stored in the image cache.
    ///
    ///   - `cachedformat` (int) :
    ///         The native data format of the pixels as stored in the image
    ///         cache (an integer, giving the `TypeDesc::BASETYPE` of the
    ///         data).  Note that this is not necessarily the same as the
    ///         native data format of the file.
    ///
    ///   - `datawindow` (int[4] or int[6]):
    ///         Returns the pixel data window of the image, which is either
    ///         an array of 4 integers (returning xmin, ymin, xmax, ymax) or
    ///         an array of 6 integers (returning xmin, ymin, zmin, xmax,
    ///         ymax, zmax). The z values may be useful for 3D/volumetric
    ///         images; for 2D images they will be 0).
    ///
    ///   - `displaywindow` (matrix) :
    ///         Returns the display (a.k.a. full) window of the image, which
    ///         is either an array of 4 integers (returning xmin, ymin,
    ///         xmax, ymax) or an array of 6 integers (returning xmin, ymin,
    ///         zmin, xmax, ymax, zmax). The z values may be useful for
    ///         3D/volumetric images; for 2D images they will be 0).
    ///
    ///   - `worldtocamera` (matrix) :
    ///         The viewing matrix, which is a 4x4 matrix (an `Imath::M44f`,
    ///         described as `TypeMatrix44`) giving the world-to-camera 3D
    ///         transformation matrix that was used when  the image was
    ///         created. Generally, only rendered images will have this.
    ///
    ///   - `worldtoscreen` (matrix) :
    ///         The projection matrix, which is a 4x4 matrix (an
    ///         `Imath::M44f`, described as `TypeMatrix44`) giving the
    ///         matrix that projected points from world space into a 2D
    ///         screen coordinate system where *x* and *y* range from $-1$
    ///         to $+1$.  Generally, only rendered images will have this.
    ///
    ///   - `averagecolor` (float[nchannels]) :
    ///         If available in the metadata (generally only for files that
    ///         have been processed by `maketx`), this will return the
    ///         average color of the texture (into an array of floats).
    ///
    ///   - `averagealpha` (float) :
    ///         If available in the metadata (generally only for files that
    ///         have been processed by `maketx`), this will return the
    ///         average alpha value of the texture (into a float).
    ///
    ///   - `constantcolor` (float[nchannels]) :
    ///         If the metadata (generally only for files that have been
    ///         processed by `maketx`) indicates that the texture has the
    ///         same values for all pixels in the texture, this will
    ///         retrieve the constant color of the texture (into an array of
    ///         floats). A non-constant image (or one that does not have the
    ///         special metadata tag identifying it as a constant texture)
    ///         will fail this query (return false).
    ///
    ///   - `constantalpha` (float) :
    ///         If the metadata indicates that the texture has the same
    ///         values for all pixels in the texture, this will retrieve the
    ///         constant alpha value of the texture. A non-constant image
    ///         (or one that does not have the special metadata tag
    ///         identifying it as a constant texture) will fail this query
    ///         (return false).
    ///
    ///   - `stat:tilesread` (int64) :
    ///         Number of tiles read from this file.
    ///
    ///   - `stat:bytesread` (int64) :
    ///         Number of bytes of uncompressed pixel data read
    ///
    ///   - `stat:redundant_tiles` (int64) :
    ///         Number of times a tile was read, where the same tile had
    ///         been rad before.
    ///
    ///   - `stat:redundant_bytesread` (int64) :
    ///         Number of bytes (of uncompressed pixel data) in tiles that
    ///         were read redundantly.
    ///
    ///   - `stat:redundant_bytesread` (int) :
    ///         Number of tiles read from this file.
    ///
    ///   - `stat:timesopened` (int) :
    ///         Number of times this file was opened.
    ///
    ///   - `stat:iotime` (float) :
    ///         Time (in seconds) spent on all I/O for this file.
    ///
    ///   - `stat:mipsused` (int) :
    ///         Stores 1 if any MIP levels beyond the highest resolution
    ///         were accesed, otherwise 0.
    ///
    ///   - `stat:is_duplicate` (int) :
    ///         Stores 1 if this file was a duplicate of another image,
    ///         otherwise 0.
    ///
    ///   - *Anything else* :
    ///         For all other data names, the the metadata of the image file
    ///         will be searched for an item that matches both the name and
    ///         data type.
    ///
    ///
    /// @param  filename
    ///             The name of the texture.
    /// @param  subimage
    ///             The subimage to query. (The metadata retrieved is for
    ///             the highest-resolution MIP level of that subimage.)
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
    ///             `true` if `get_textureinfo()` is able to find the
    ///             requested `dataname` for the texture and it matched the
    ///             requested `datatype`.  If the requested data was not
    ///             found or was not of the right data type, return `false`.
    ///             Except for the `"exists"` query, a file that does not
    ///             exist or could not be read properly as an image also
    ///             constitutes a query failure that will return `false`.
    virtual bool get_texture_info (ustring filename, int subimage,
                          ustring dataname, TypeDesc datatype, void *data) = 0;

    /// A more efficient variety of `get_texture_info()` for cases where you
    /// can use a `TextureHandle*` to specify the image and optionally have
    /// a `Perthread*` for the calling thread.
    virtual bool get_texture_info (TextureHandle *texture_handle,
                          Perthread *thread_info, int subimage,
                          ustring dataname, TypeDesc datatype, void *data) = 0;

    /// Copy the ImageSpec associated with the named texture (the first
    /// subimage by default, or as set by `subimage`).
    ///
    /// @param  filename
    ///             The name of the image.
    /// @param  subimage
    ///             The subimage to query. (The spec retrieved is for the
    ///             highest-resolution MIP level of that subimage.)
    /// @param  spec
    ///             ImageSpec into which will be copied the spec for the
    ///             requested image.
    /// @returns
    ///             `true` upon success, `false` upon failure failure (such
    ///             as being unable to find, open, or read the file, or if
    ///             it does not contain the designated subimage or MIP
    ///             level).
    virtual bool get_imagespec (ustring filename, int subimage,
                                ImageSpec &spec) = 0;
    /// A more efficient variety of `get_imagespec()` for cases where you
    /// can use a `TextureHandle*` to specify the image and optionally have
    /// a `Perthread*` for the calling thread.
    virtual bool get_imagespec (TextureHandle *texture_handle,
                                Perthread *thread_info, int subimage,
                                ImageSpec &spec) = 0;

    /// Return a pointer to an ImageSpec associated with the named texture
    /// if the file is found and is an image format that can be read,
    /// otherwise return `nullptr`.
    ///
    /// This method is much more efficient than `get_imagespec()`, since it
    /// just returns a pointer to the spec held internally by the
    /// TextureSystem (rather than copying the spec to the user's memory).
    /// However, the caller must beware that the pointer is only valid as
    /// long as nobody (even other threads) calls `invalidate()` on the
    /// file, or `invalidate_all()`, or destroys the TextureSystem and its
    /// underlying ImageCache.
    ///
    /// @param  filename
    ///             The name of the image.
    /// @param  subimage
    ///             The subimage to query.  (The spec retrieved is for the
    ///             highest-resolution MIP level of that subimage.)
    /// @returns
    ///             A pointer to the spec, if the image is found and able to
    ///             be opened and read by an available image format plugin,
    ///             and the designated subimage exists.
    virtual const ImageSpec *imagespec (ustring filename, int subimage=0) = 0;
    /// A more efficient variety of `imagespec()` for cases where you can
    /// use a `TextureHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    virtual const ImageSpec *imagespec (TextureHandle *texture_handle,
                                        Perthread *thread_info = nullptr,
                                        int subimage=0) = 0;

    /// For a texture specified by name, retrieve the rectangle of raw
    /// unfiltered texels from the subimage specified in `options` and at
    /// the designated `miplevel`, storing the pixel values beginning at the
    /// address specified by `result`.  The pixel values will be converted
    /// to the data type specified by `format`. The rectangular region to be
    /// retrieved includes `begin` but does not include `end` (much like STL
    /// begin/end usage). Requested pixels that are not part of the valid
    /// pixel data region of the image file will be filled with zero values.
    /// Channels requested but not present in the file will get the
    /// `options.fill` value.
    ///
    /// @param  filename
    ///             The name of the image.
    /// @param  options
    ///             A TextureOpt describing access options, including wrap
    ///             modes, fill value, and subimage, that will be used when
    ///             retrieving pixels.
    /// @param  miplevel
    ///             The MIP level to retrieve pixels from (0 is the highest
    ///             resolution level).
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
    ///             stored in the file or in the cache.
    /// @param  result
    ///             Pointer to the memory where the pixel values should be
    ///             stored.  It is up to the caller to ensure that `result`
    ///             points to an area of memory big enough to accommodate
    ///             the requested rectangle (taking into consideration its
    ///             dimensions, number of channels, and data format).
    ///
    /// @returns
    ///             `true` for success, `false` for failure.
    virtual bool get_texels (ustring filename, TextureOpt &options,
                             int miplevel, int xbegin, int xend,
                             int ybegin, int yend, int zbegin, int zend,
                             int chbegin, int chend,
                             TypeDesc format, void *result) = 0;
    /// A more efficient variety of `get_texels()` for cases where you can
    /// use a `TextureHandle*` to specify the image and optionally have a
    /// `Perthread*` for the calling thread.
    virtual bool get_texels (TextureHandle *texture_handle,
                             Perthread *thread_info, TextureOpt &options,
                             int miplevel, int xbegin, int xend,
                             int ybegin, int yend, int zbegin, int zend,
                             int chbegin, int chend,
                             TypeDesc format, void *result) = 0;

    /// @}

    /// @{
    /// @name Controlling the cache
    ///

    /// Invalidate any cached information about the named file, including
    /// loaded texture tiles from that texture, and close any open file
    /// handle associated with the file. This calls
    /// `ImageCache::invalidate(filename,force)` on the underlying
    /// ImageCache.
    virtual void invalidate (ustring filename, bool force = true) = 0;

    /// Invalidate all cached data for all textures.  This calls
    /// `ImageCache::invalidate_all(force)` on the underlying ImageCache.
    virtual void invalidate_all (bool force=false) = 0;

    /// Close any open file handles associated with a named file, but do not
    /// invalidate any image spec information or pixels associated with the
    /// files.  A client might do this in order to release OS file handle
    /// resources, or to make it safe for other processes to modify textures
    /// on disk.  This calls `ImageCache::close(force)` on the underlying
    /// ImageCache.
    virtual void close (ustring filename) = 0;

    /// `close()` all files known to the cache.
    virtual void close_all () = 0;

    /// @}

    /// @{
    /// @name Errors and statistics
    
    /// If any of the API routines returned false indicating an error,
    /// this routine will return the error string (and clear any error
    /// flags).  If no error has occurred since the last time geterror()
    /// was called, it will return an empty string.
    virtual std::string geterror () const = 0;

    /// Returns a big string containing useful statistics about the
    /// TextureSystem operations, suitable for saving to a file or
    /// outputting to the terminal. The `level` indicates the amount of
    /// detail in the statistics, with higher numbers (up to a maximum of 5)
    /// yielding more and more esoteric information.  If `icstats` is true,
    /// the returned string will also contain all the statistics of the
    /// underlying ImageCache, but if false will only contain
    /// texture-specific statistics.
    virtual std::string getstats (int level=1, bool icstats=true) const = 0;

    /// Reset most statistics to be as they were with a fresh TextureSystem.
    /// Caveat emptor: this does not flush the cache itelf, so the resulting
    /// statistics from the next set of texture requests will not match the
    /// number of tile reads, etc., that would have resulted from a new
    /// TextureSystem.
    virtual void reset_stats () = 0;

    /// @}

    /// Return an opaque, non-owning pointer to the underlying ImageCache
    /// (if there is one).
    virtual ImageCache *imagecache () const = 0;

    virtual ~TextureSystem () { }

protected:
    // User code should never directly construct or destruct a TextureSystem.
    // Always use TextureSystem::create() and TextureSystem::destroy().
    TextureSystem (void) { }
private:
    // Make delete private and unimplemented in order to prevent apps
    // from calling it.  Instead, they should call TextureSystem::destroy().
    void operator delete(void* /*todel*/) {}
};


OIIO_NAMESPACE_END
