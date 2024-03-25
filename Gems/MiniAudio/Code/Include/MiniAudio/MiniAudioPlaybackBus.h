/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <MiniAudio/SoundAsset.h>
#include <MiniAudio/SoundAssetRef.h>

namespace MiniAudio
{
    class MiniAudioPlaybackRequests : public AZ::ComponentBus
    {
    public:
        ~MiniAudioPlaybackRequests() override = default;

        virtual void Play() = 0;
        virtual void Stop() = 0;
        virtual void SetVolume(float volume) = 0;
        virtual void SetLooping(bool loop) = 0;
        virtual bool IsLooping() const = 0;
        virtual void SetSoundAsset(AZ::Data::Asset<SoundAsset> soundAsset) = 0;
        virtual AZ::Data::Asset<SoundAsset> GetSoundAsset() const = 0;

        //! Custom setter for scripting
        virtual void SetSoundAssetRef(const SoundAssetRef& soundAssetRef) = 0;
        //! Custom getter for scripting
        virtual SoundAssetRef GetSoundAssetRef() const = 0;
    };

    using MiniAudioPlaybackRequestBus = AZ::EBus<MiniAudioPlaybackRequests>;

} // namespace MiniAudio
