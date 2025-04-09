/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/base.h>

/**
* Shorthand for checking a condition, and failing if false.
* Works with any function that returns AZ::Outcome<..., AZStd::string>.
* Unlike assert, it is not removed in release builds.
* Ensure all strings are passed with c_str(), as they are passed to AZStd::string::format().
*/
#define AZ_ENSURE_STRING_OUTCOME_CONDITION(cond, ...) if (!(cond)) { return AZ::Failure(AZStd::string::format(__VA_ARGS__)); }

// Similar to above macro, but ensures on an AZ::Outcome. Not removed in release builds.
#define AZ_ENSURE_STRING_OUTCOME(outcome) if (!(outcome.IsSuccess())) { return AZ::Failure(outcome.GetError()); }

namespace ImageProcessingAtom
{
    //! Common return type for operations that can fail.
    //  Empty success string == Success.
    //  Populated success string == Warning.
    //  Populated error string == Failure.
    using StringOutcome = AZ::Outcome<AZStd::string, AZStd::string>;
#define STRING_OUTCOME_SUCCESS AZ::Success(AZStd::string())
#define STRING_OUTCOME_WARNING(warning) AZ::Success(AZStd::string(warning))
#define STRING_OUTCOME_ERROR(error) AZ::Failure(AZStd::string(error))

    // Common typedefs (with dependent forward-declarations)
    typedef AZStd::string PlatformName;
    typedef AZStd::string FileMask;
    typedef AZ::Name PresetName;
    typedef AZStd::vector<PlatformName> PlatformNameVector;
    typedef AZStd::list<PlatformName> PlatformNameList;

    //! min and max reduce level
    static const unsigned int s_MinReduceLevel = 0;
    static const unsigned int s_MaxReduceLevel = 5;

    static const char* s_SupportedImageExtensions[] = {
        "*.tif",
        "*.tiff",
        "*.png",
        "*.bmp",
        "*.jpg",
        "*.jpeg",
        "*.tga",
        "*.gif",
        "*.dds",
        "*.exr",
        "*.pgm"
    };
    static constexpr int s_TotalSupportedImageExtensions = AZ_ARRAY_SIZE(s_SupportedImageExtensions);

    enum class RGBWeight : AZ::u32
    {
        uniform,        // uniform weights (1.0, 1.0, 1.0) (default)
        luminance,      // luminance-based weights (0.3086, 0.6094, 0.0820)
        ciexyz          // ciexyz-based weights (0.2126, 0.7152, 0.0722)
    };

    enum class ColorSpace : AZ::u32
    {
        linear,
        sRGB,
        autoSelect,
    };

    enum class MipGenType : AZ::u32
    {
        point,          //Also called nearest neighbor
        box,            //Also called 'average'. When shrinking images it will average, and merge the pixels together.
        triangle,       //Also called linear or Bartlett window
        quadratic,      //Also called bilinear or Welch window
        gaussian,       //It remove high frequency noise in a highly controllable way.
        blackmanHarris,
        kaiserSinc      //Good for foliage and tree assets exported from Speedtree.
    };

    enum class MipGenEvalType : AZ::u32
    {
        sum,
        max,
        min
    };

    //cubemap angular filter type. Only two filter types were used in rc.ini
    enum class CubemapFilterType : AZ::u32
    {
        disc = 0,           // same as CP_FILTER_TYPE_DISC in CubemapGen
        cone = 1,           // same as CP_FILTER_TYPE_CONE
        cosine = 2,         // same as CP_FILTER_TYPE_COSINE. only used for [EnvironmentProbeHDR_Irradiance]
        gaussian = 3,       // same as CP_FILTER_TYPE_ANGULAR_GAUSSIAN
        cosine_power = 4,   // same as CP_FILTER_TYPE_COSINE_POWER
        ggx = 5             // same as CP_FILTER_TYPE_GGX. only used for [EnvironmentProbeHDR]
    };
} // namespace ImageProcessingAtom

namespace AZ
{
    // Bind enums with uuids. Required for named enum support.
    // Note: AZ_TYPE_INFO_SPECIALIZE has to be declared in AZ namespace
    AZ_TYPE_INFO_SPECIALIZE(ImageProcessingAtom::RGBWeight, "{80AFA059-2DF5-45A7-A2F1-27A7A045B0DB}");
    AZ_TYPE_INFO_SPECIALIZE(ImageProcessingAtom::ColorSpace, "{C924C0BB-1154-4341-A25A-698A3950B286}");
    AZ_TYPE_INFO_SPECIALIZE(ImageProcessingAtom::CubemapFilterType, "{0D69E9F3-8F4C-4415-96B5-64ACA0B0888B}");
    AZ_TYPE_INFO_SPECIALIZE(ImageProcessingAtom::MipGenType, "{8524F650-1417-44DA-BBB0-C707A7A1A709}");
}
