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

# Additional Libraries
# These can be added/enabled to the linker depending on what your Wwise project uses.
# In addition to uncommenting the libraries here, be sure to add the appropriate plugin factory
# header includes to PluginRegistration_wwise.h.

set(WWISE_ADDITIONAL_LIB_NAMES
# Common
    #AkConvolutionReverbFX
    #AkReflectFX
    #AkRouterMixerFX
    #ResonanceAudioFX
    #MasteringSuiteFX
    #AkSoundSeedImpactFX
    #AkSoundSeedGrainSource
    #AkSoundSeedWindSource
    #AkSoundSeedWooshSource
    #AuroHeadphoneFX
    #CrankcaseAudioREVModelPlayerSource
    #McDSPFutzBoxFX
    #McDSPLimiterFX

# iZotope
    #iZHybridReverbFX
    #iZTrashBoxModelerFX
    #iZTrashDelayFX
    #iZTrashDistortionFX
    #iZTrashDynamicsFX
    #iZTrashFiltersFX
    #iZTrashMultibandDistortionFX
)

set(WWISE_COMPILE_DEFINITIONS
    $<IF:$<CONFIG:Release>,AK_OPTIMIZED,>
)

ly_add_external_target(
    NAME Wwise
    VERSION 2019.2.8.7432
    INCLUDE_DIRECTORIES SDK/include
    COMPILE_DEFINITIONS ${WWISE_COMPILE_DEFINITIONS}
)
