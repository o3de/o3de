#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(WWISE_VERSION 2021.1.0.7575)

function(check_wwise)
    set(stamp_file install-${WWISE_VERSION}.stamp)
    if (EXISTS ${stamp_file})
        return()
    endif()

    message(STATUS "Checking for Wwise...")

    # Check various file paths for the Wwise version...
    # If found, touch the stamp file
    #file(TOUCH ${stamp_file})
endfunction()

check_wwise()

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
    # Additional Libraries (i.e. Plugins)
    # These can be added/enabled to the linker depending on what your Wwise project uses.
    # In addition to adding libraries here, be sure to add the appropriate plugin factory
    # header includes to PluginRegistration_wwise.h.

# Examples...
    #AkConvolutionReverbFX
    #AkReflectFX
    #AkRouterMixerFX
    # ...
)

set(WWISE_COMPILE_DEFINITIONS
    $<IF:$<CONFIG:Release>,AK_OPTIMIZED,>
)

ly_add_external_target(
    NAME Wwise
    VERSION ${WWISE_VERSION}
    INCLUDE_DIRECTORIES SDK/include
    COMPILE_DEFINITIONS ${WWISE_COMPILE_DEFINITIONS}
)
