/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMPARAMTYPE_H
#define CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMPARAMTYPE_H
#pragma once

// Static common parameters IDs of animation node.
//
// You need to register new params in Movie.cpp/RegisterParamTypes for serialization.
// Note: Enums are serialized by string now, there is no need for specific IDs
// anymore for new parameters. Values are for backward compatibility.
//
// If you want to expand CryMovie to control new stuff this is probably  the enum you want to change.
// For named params see AnimParamType::ByString & CAnimParamType
enum class AnimParamType
{
    // Parameter is specified by string. See CAnimParamType
    ByString                         = 8,

    //FOV                              = 0,
    Position                         = 1,
    Rotation                         = 2,
    Scale                            = 3,
    Event                            = 4,
    Visibility                       = 5,
    Camera                           = 6,
    Animation                        = 7,
    Sound                            = 10,
    Sequence                         = 13,
    // Expression                    = 14,       ///@deprecated Jan 2016
    Console                          = 17,
    Music                            = 18,       ///@deprecated in 1.11, July 2017 - left in for legacy serialization
    Float                            = 19,
    // FaceSequence                  = 20,       ///@deprecated Jan 2016
    LookAt                           = 21,
    TrackEvent                       = 22,

    ShakeAmplitudeA                  = 23,
    ShakeAmplitudeB                  = 24,
    ShakeFrequencyA                  = 25,
    ShakeFrequencyB                  = 26,
    ShakeMultiplier                  = 27,
    ShakeNoise                       = 28,
    ShakeWorking                     = 29,
    ShakeAmpAMult                    = 61,
    ShakeAmpBMult                    = 62,
    ShakeFreqAMult                   = 63,
    ShakeFreqBMult                   = 64,

    DepthOfField                     = 30,
    FocusDistance                    = 31,
    FocusRange                       = 32,
    BlurAmount                       = 33,

    Capture                          = 34,
    TransformNoise                   = 35,
    TimeWarp                         = 36,
    FixedTimeStep                    = 37,
    NearZ                            = 38,
    Goto                             = 39,

    PositionX                        = 51,
    PositionY                        = 52,
    PositionZ                        = 53,

    RotationX                        = 54,
    RotationY                        = 55,
    RotationZ                        = 56,

    ScaleX                           = 57,
    ScaleY                           = 58,
    ScaleZ                           = 59,

    ColorR                           = 82,
    ColorG                           = 83,
    ColorB                           = 84,

    CommentText                      = 70,
    ScreenFader                      = 71,

    LightDiffuse                     = 81,
    LightRadius                      = 85,
    LightDiffuseMult                 = 86,
    LightHDRDynamic                  = 87,
    LightSpecularMult                = 88,
    LightSpecPercentage              = 89,

    MaterialDiffuse          = 90,
    MaterialSpecular         = 91,
    MaterialEmissive         = 92,
    // MaterialEmissiveIntensity = 99,   // declared below to keep enumerations in order
    MaterialOpacity          = 93,
    MaterialSmoothness       = 94,

    TimeRanges               = 95, //!< Generic track with keys that have time ranges

    Physics                  = 96,   //! Not used anymore, see Physicalize & PhysicsDriven

    ProceduralEyes           = 97,

    Unused                   = 98, //!< Mannequin parameter that was removed.

    MaterialEmissiveIntensity = 99,

    // Add new param types without explicit ID here:
    // NOTE: don't forget to register in Movie.cpp

    GSMCache,

    ShutterSpeed,

    Physicalize,
    PhysicsDriven,

    SunLongitude,
    SunLatitude,
    MoonLongitude,
    MoonLatitude,

    User                        = 100000, //!< User node params.

    Invalid                     = static_cast<int>(0xFFFFFFFF)
};

static const int OLD_APARAM_USER = 100;

#endif // CRYINCLUDE_CRYCOMMON_MAESTRO_TYPES_ANIMPARAMTYPE_H
