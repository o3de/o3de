#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Source/MiniAudioModuleInterface.h
    Source/Clients/MiniAudioImplementation.cpp
    Source/Clients/MiniAudioIncludes.h
    Source/Clients/MiniAudioListenerComponent.cpp
    Source/Clients/MiniAudioListenerComponent.h

    Source/Clients/MiniAudioListenerComponent.cpp
    Source/Clients/MiniAudioListenerComponent.h
    Source/Clients/MiniAudioListenerComponentConfig.cpp
    Source/Clients/MiniAudioListenerComponentConfig.h
    Source/Clients/MiniAudioListenerComponentController.cpp
    Source/Clients/MiniAudioListenerComponentController.h

    Source/Clients/MiniAudioPlaybackComponent.cpp
    Source/Clients/MiniAudioPlaybackComponent.h
    Source/Clients/MiniAudioPlaybackComponentConfig.cpp
    Source/Clients/MiniAudioPlaybackComponentConfig.h
    Source/Clients/MiniAudioPlaybackComponentController.cpp
    Source/Clients/MiniAudioPlaybackComponentController.h

    Source/Clients/MiniAudioSystemComponent.cpp
    Source/Clients/MiniAudioSystemComponent.h
    Source/Clients/SoundAssetHandler.cpp
    Source/Clients/SoundAssetHandler.h
    Source/Clients/SoundAsset.cpp
    Source/Clients/SoundAssetRef.cpp
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    Source/Clients/MiniAudioImplementation.cpp
)
