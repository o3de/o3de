#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

# Wwise Install Path
set(LY_WWISE_INSTALL_PATH "" CACHE PATH "Path to Wwise installation.")

# Wwise Version
set(WWISE_VERSION)
set(MINIMUM_WWISE_VERSION "2021.1.1.0")

# Inspect a file in the Wwise SDK, extract the version and check it...
function(is_valid_sdk sdk_path is_valid)
    set(${is_valid} FALSE PARENT_SCOPE)
    if(EXISTS ${sdk_path})
        set(sdk_version_file ${sdk_path}/SDK/include/AK/AkWwiseSDKVersion.h)
        if(EXISTS ${sdk_version_file})
            set(version_regex "^.*VERSION_(MAJOR|MINOR|SUBMINOR|BUILD)[ \t]+([0-9]+)")
            file(STRINGS ${sdk_version_file} version_strings REGEX ${version_regex})
            list(LENGTH version_strings num_parts)
            if(num_parts EQUAL 4)
                set(wwise_ver)
                foreach(version_str ${version_strings})
                    string(REGEX REPLACE ${version_regex} "\\2" version_part ${version_str})
                    string(JOIN "." wwise_ver ${wwise_ver} ${version_part})
                endforeach()

                if(${wwise_ver} VERSION_GREATER_EQUAL ${MINIMUM_WWISE_VERSION})
                    set(WWISE_VERSION ${wwise_ver} PARENT_SCOPE)
                    set(${is_valid} TRUE PARENT_SCOPE)
                else()
                    message(STATUS "Minimum supported Wwise SDK version is ${MINIMUM_WWISE_VERSION}, but detected version ${wwise_ver}.")
                endif()
            endif()
        endif()
    endif()
endfunction()

# Paths that will be checked, in order:
# - CMake cache variable
# - WWISEROOT Environment Variable
set(WWISE_SDK_PATHS
    "${LY_WWISE_INSTALL_PATH}"
    "$ENV{WWISEROOT}"
)

set(found_sdk FALSE)
foreach(candidate_path ${WWISE_SDK_PATHS})
    is_valid_sdk(${candidate_path} found_sdk)
    if(found_sdk)
        # Update the Wwise Install Path variable internally
        set(LY_WWISE_INSTALL_PATH "${candidate_path}")
        break()
    endif()
endforeach()

if(NOT found_sdk)
    # If we don't find a path that appears to be a valid Wwise install, we can bail here.
    # No 3rdParty::Wwise target will exist, so that can be checked elsewhere.
    return()
endif()

message(STATUS "Using Wwise SDK version ${WWISE_VERSION} at ${LY_WWISE_INSTALL_PATH}")

set(WWISE_COMMON_LIB_NAMES
    # Core AK
    AkMemoryMgr
    AkMusicEngine
    AkSoundEngine
    AkStreamMgr
    AkSpatialAudio

    # AK Effects
    AkCompressorFX
    AkDelayFX
    AkMatrixReverbFX
    AkMeterFX
    AkExpanderFX
    AkParametricEQFX
    AkGainFX
    AkPeakLimiterFX
    AkRoomVerbFX
    AkGuitarDistortionFX
    AkStereoDelayFX
    AkPitchShifterFX
    AkTimeStretchFX
    AkFlangerFX
    AkTremoloFX
    AkHarmonizerFX
    AkRecorderFX

    # AK Sources
    AkSilenceSource
    AkSineSource
    AkToneSource
    AkAudioInputSource
    AkSynthOneSource
)

set(WWISE_CODEC_LIB_NAMES
    # AK Codecs
    AkVorbisDecoder
    AkOpusDecoder
)

set(WWISE_NON_RELEASE_LIB_NAMES
    # For remote profiling
    CommunicationCentral
)

set(WWISE_ADDITIONAL_LIB_NAMES
    # Additional Libraries
)

set(WWISE_COMPILE_DEFINITIONS
    $<IF:$<CONFIG:Release>,AK_OPTIMIZED,>
)


# Use these to get the parent path and folder name before adding the external 3p target.
get_filename_component(WWISE_INSTALL_ROOT ${LY_WWISE_INSTALL_PATH} DIRECTORY)
get_filename_component(WWISE_FOLDER ${LY_WWISE_INSTALL_PATH} NAME)

ly_add_external_target(
    NAME Wwise
    VERSION "${WWISE_FOLDER}"
    3RDPARTY_ROOT_DIRECTORY "${WWISE_INSTALL_ROOT}"
    INCLUDE_DIRECTORIES SDK/include
    COMPILE_DEFINITIONS ${WWISE_COMPILE_DEFINITIONS}
)

set(Wwise_FOUND TRUE)
