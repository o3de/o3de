// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

#pragma once

#include <memory>

#include <OpenImageIO/export.h>
#include <OpenImageIO/fmath.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/ustring.h>

#include <OpenEXR/ImathMatrix.h> /* because we need M44f */


OIIO_NAMESPACE_BEGIN

/// The ColorProcessor encapsulates a baked color transformation, suitable for
/// application to raw pixels, or ImageBuf(s). These are generated using
/// ColorConfig::createColorProcessor, and referenced in ImageBufAlgo
/// (amongst other places)
class OIIO_API ColorProcessor {
public:
    ColorProcessor() {};
    virtual ~ColorProcessor(void) {};
    virtual bool isNoOp() const { return false; }
    virtual bool hasChannelCrosstalk() const { return false; }

    // Convert an array/image of color values. The strides are the distance,
    // in bytes, between subsequent color channels, pixels, and scanlines.
    virtual void apply(float* data, int width, int height, int channels,
                       stride_t chanstride, stride_t xstride,
                       stride_t ystride) const = 0;
    // Convert a single 3-color
    void apply(float* data)
    {
        apply((float*)data, 1, 1, 3, sizeof(float), 0, 0);
    }
};

// Preprocessor symbol to allow conditional compilation depending on
// whether the ColorProcesor class is exposed (it was not prior to OIIO 1.9).
#define OIIO_HAS_COLORPROCESSOR 1



typedef std::shared_ptr<ColorProcessor> ColorProcessorHandle;

// Preprocessor symbol to allow conditional compilation depending on
// whether the ColorConfig returns ColorProcessor shared pointers or raw.
#define OIIO_COLORCONFIG_USES_SHARED_PTR 1



/// Represents the set of all color transformations that are allowed.
/// If OpenColorIO is enabled at build time, this configuration is loaded
/// at runtime, allowing the user to have complete control of all color
/// transformation math. ($OCIO)  (See opencolorio.org for details).
/// If OpenColorIO is not enabled at build time, a generic color configuration
/// is provided for minimal color support.
///
/// NOTE: ColorConfig(s) and ColorProcessor(s) are potentially heavy-weight.
/// Their construction / destruction should be kept to a minimum.

class OIIO_API ColorConfig {
public:
    /// Construct a ColorConfig using the named OCIO configuration file,
    /// or if filename is empty, to the current color configuration
    /// specified by env variable $OCIO.
    ///
    /// Multiple calls to this are potentially expensive. A ColorConfig
    /// should usually be shared by an app for its entire runtime.
    ColorConfig(string_view filename = "");

    ~ColorConfig();

    /// Reset the config to the named OCIO configuration file, or if
    /// filename is empty, to the current color configuration specified
    /// by env variable $OCIO. Return true for success, false if there
    /// was an error.
    ///
    /// Multiple calls to this are potentially expensive. A ColorConfig
    /// should usually be shared by an app for its entire runtime.
    bool reset(string_view filename = "");

    /// Has an error string occurred?
    /// (This will not affect the error state.)
    bool error() const;

    /// This routine will return the error string (and clear any error
    /// flags).  If no error has occurred since the last time geterror()
    /// was called, it will return an empty string.
    std::string geterror();

    /// Get the number of ColorSpace(s) defined in this configuration
    int getNumColorSpaces() const;

    /// Query the name of the specified ColorSpace.
    const char* getColorSpaceNameByIndex(int index) const;

    /// Get the name of the color space representing the named role,
    /// or NULL if none could be identified.
    const char* getColorSpaceNameByRole(string_view role) const;

    /// Get the data type that OCIO thinks this color space is. The name
    /// may be either a color space name or a role.
    OIIO::TypeDesc getColorSpaceDataType(string_view name, int* bits) const;

    /// Retrieve the full list of known color space names, as a vector
    /// of strings.
    std::vector<std::string> getColorSpaceNames() const;

    /// Get the name of the color space family of the named color space,
    /// or NULL if none could be identified.
    const char* getColorSpaceFamilyByName(string_view name) const;

    // Get the number of Roles defined in this configuration
    int getNumRoles() const;

    /// Query the name of the specified Role.
    const char* getRoleByIndex(int index) const;

    /// Retrieve the full list of known Roles, as a vector of strings.
    std::vector<std::string> getRoles() const;

    /// Get the number of Looks defined in this configuration
    int getNumLooks() const;

    /// Query the name of the specified Look.
    const char* getLookNameByIndex(int index) const;

    /// Retrieve the full list of known look names, as a vector of strings.
    std::vector<std::string> getLookNames() const;

    /// Given the specified input and output ColorSpace, request a handle to
    /// a ColorProcessor.  It is possible that this will return an empty
    /// handle, if the inputColorSpace doesnt exist, the outputColorSpace
    /// doesn't exist, or if the specified transformation is illegal (for
    /// example, it may require the inversion of a 3D-LUT, etc).
    ///
    /// The handle is actually a shared_ptr, so when you're done with a
    /// ColorProcess, just discard it. ColorProcessor(s) remain valid even
    /// if the ColorConfig that created them no longer exists.
    ///
    /// Created ColorProcessors are cached, so asking for the same color
    /// space transformation multiple times shouldn't be very expensive.
    ColorProcessorHandle createColorProcessor(
        string_view inputColorSpace, string_view outputColorSpace,
        string_view context_key = "", string_view context_value = "") const;
    ColorProcessorHandle
    createColorProcessor(ustring inputColorSpace, ustring outputColorSpace,
                         ustring context_key   = ustring(),
                         ustring context_value = ustring()) const;

    /// Given the named look(s), input and output color spaces, request a
    /// color processor that applies an OCIO look transformation.  If
    /// inverse==true, request the inverse transformation.  The
    /// context_key and context_value can optionally be used to establish
    /// extra key/value pairs in the OCIO context if they are comma-
    /// separated lists of ontext keys and values, respectively.
    ///
    /// The handle is actually a shared_ptr, so when you're done with a
    /// ColorProcess, just discard it. ColorProcessor(s) remain valid even
    /// if the ColorConfig that created them no longer exists.
    ///
    /// Created ColorProcessors are cached, so asking for the same color
    /// space transformation multiple times shouldn't be very expensive.
    ColorProcessorHandle
    createLookTransform(string_view looks, string_view inputColorSpace,
                        string_view outputColorSpace, bool inverse = false,
                        string_view context_key   = "",
                        string_view context_value = "") const;
    ColorProcessorHandle
    createLookTransform(ustring looks, ustring inputColorSpace,
                        ustring outputColorSpace, bool inverse = false,
                        ustring context_key   = ustring(),
                        ustring context_value = ustring()) const;

    /// Get the number of displays defined in this configuration
    int getNumDisplays() const;

    /// Query the name of the specified display.
    const char* getDisplayNameByIndex(int index) const;

    /// Retrieve the full list of known display names, as a vector of
    /// strings.
    std::vector<std::string> getDisplayNames() const;

    /// Get the name of the default display.
    const char* getDefaultDisplayName() const;

    /// Get the number of views for a given display defined in this
    /// configuration. If the display is empty or not specified, the default
    /// display will be used.
    int getNumViews(string_view display = "") const;

    /// Query the name of the specified view for the specified display
    const char* getViewNameByIndex(string_view display, int index) const;

    /// Retrieve the full list of known view names for the display, as a
    /// vector of strings. If the display is empty or not specified, the
    /// default display will be used.
    std::vector<std::string> getViewNames(string_view display = "") const;

    /// Query the name of the default view for the specified display. If the
    /// display is empty or not specified, the default display will be used.
    const char* getDefaultViewName(string_view display = "") const;

    /// Construct a processor to transform from the given color space
    /// to the color space of the given display and view. You may optionally
    /// override the looks that are, by default, used with the display/view
    /// combination. Looks is a potentially comma (or colon) delimited list
    /// of lookNames, where +/- prefixes are optionally allowed to denote
    /// forward/inverse transformation (and forward is assumed in the
    /// absence of either). It is possible to remove all looks from the
    /// display by passing an empty string. The context_key and context_value
    /// can optionally be used to establish extra key/value pair in the OCIO
    /// context if they are comma-separated lists of context keys and
    /// values, respectively.
    ///
    /// It is possible that this will return an empty handle if one of the
    /// color spaces or the display or view doesn't exist or is not allowed.
    ///
    /// The handle is actually a shared_ptr, so when you're done with a
    /// ColorProcess, just discard it. ColorProcessor(s) remain valid even
    /// if the ColorConfig that created them no longer exists.
    ///
    /// Created ColorProcessors are cached, so asking for the same color
    /// space transformation multiple times shouldn't be very expensive.
    ColorProcessorHandle
    createDisplayTransform(string_view display, string_view view,
                           string_view inputColorSpace, string_view looks = "",
                           string_view context_key   = "",
                           string_view context_value = "") const;
    ColorProcessorHandle
    createDisplayTransform(ustring display, ustring view,
                           ustring inputColorSpace, ustring looks = ustring(),
                           ustring context_key   = ustring(),
                           ustring context_value = ustring()) const;

    /// Construct a processor to perform color transforms determined by an
    /// OpenColorIO FileTransform. It is possible that this will return an
    /// empty handle if the FileTransform doesn't exist or is not allowed.
    ///
    /// The handle is actually a shared_ptr, so when you're done with a
    /// ColorProcess, just discard it. ColorProcessor(s) remain valid even
    /// if the ColorConfig that created them no longer exists.
    ///
    /// Created ColorProcessors are cached, so asking for the same color
    /// space transformation multiple times shouldn't be very expensive.
    ColorProcessorHandle createFileTransform(string_view name,
                                             bool inverse = false) const;
    ColorProcessorHandle createFileTransform(ustring name,
                                             bool inverse = false) const;

    /// Construct a processor to perform color transforms specified by a
    /// 4x4 matrix.
    ///
    /// The handle is actually a shared_ptr, so when you're done with a
    /// ColorProcess, just discard it.
    ///
    /// Created ColorProcessors are cached, so asking for the same color
    /// space transformation multiple times shouldn't be very expensive.
    ColorProcessorHandle createMatrixTransform(const Imath::M44f& M,
                                               bool inverse = false) const;

    /// Given a string (like a filename), look for the longest, right-most
    /// colorspace substring that appears. Returns "" if no such color space
    /// is found. (This is just a wrapper around OCIO's
    /// ColorConfig::parseColorSpaceFromString.)
    string_view parseColorSpaceFromString(string_view str) const;

    /// Return a filename or other identifier for the config we're using.
    std::string configname() const;

    // DEPRECATED(1.9) -- no longer necessary, because it's a shared ptr
    static void deleteColorProcessor(const ColorProcessorHandle& processor) {}

    /// Return if OpenImageIO was built with OCIO support
    static bool supportsOpenColorIO();

private:
    ColorConfig(const ColorConfig&) = delete;
    ColorConfig& operator=(const ColorConfig&) = delete;

    class Impl;
    std::unique_ptr<Impl> m_impl;
    Impl* getImpl() const { return m_impl.get(); }
};



/// Utility -- convert sRGB value to linear
///    http://en.wikipedia.org/wiki/SRGB
inline float
sRGB_to_linear(float x)
{
    return (x <= 0.04045f) ? (x * (1.0f / 12.92f))
                           : powf((x + 0.055f) * (1.0f / 1.055f), 2.4f);
}


#ifndef __CUDA_ARCH__
inline simd::vfloat4
sRGB_to_linear(const simd::vfloat4& x)
{
    return simd::select(
        x <= 0.04045f, x * (1.0f / 12.92f),
        fast_pow_pos(madd(x, (1.0f / 1.055f), 0.055f * (1.0f / 1.055f)), 2.4f));
}
#endif

/// Utility -- convert linear value to sRGB
inline float
linear_to_sRGB(float x)
{
    return (x <= 0.0031308f) ? (12.92f * x)
                             : (1.055f * powf(x, 1.f / 2.4f) - 0.055f);
}


#ifndef __CUDA_ARCH__
/// Utility -- convert linear value to sRGB
inline simd::vfloat4
linear_to_sRGB(const simd::vfloat4& x)
{
    // x = simd::max (x, simd::vfloat4::Zero());
    return simd::select(x <= 0.0031308f, 12.92f * x,
                        madd(1.055f, fast_pow_pos(x, 1.f / 2.4f), -0.055f));
}
#endif


/// Utility -- convert Rec709 value to linear
///    http://en.wikipedia.org/wiki/Rec._709
inline float
Rec709_to_linear(float x)
{
    if (x < 0.081f)
        return x * (1.0f / 4.5f);
    else
        return powf((x + 0.099f) * (1.0f / 1.099f), (1.0f / 0.45f));
}

/// Utility -- convert linear value to Rec709
inline float
linear_to_Rec709(float x)
{
    if (x < 0.018f)
        return x * 4.5f;
    else
        return 1.099f * powf(x, 0.45f) - 0.099f;
}


OIIO_NAMESPACE_END
