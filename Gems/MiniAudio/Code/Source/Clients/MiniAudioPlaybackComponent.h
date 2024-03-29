/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Component/Component.h>
#include <AzFramework/Components/ComponentAdapter.h>
#include <MiniAudio/MiniAudioConstants.h>
#include <MiniAudio/MiniAudioPlaybackBus.h>

#include "MiniAudioPlaybackComponentController.h"
#include "MiniAudioPlaybackComponentConfig.h"

namespace MiniAudio
{
    class MiniAudioPlaybackComponent final
        : public AzFramework::Components::ComponentAdapter<MiniAudioPlaybackComponentController, MiniAudioPlaybackComponentConfig>
    {
        using BaseClass = AzFramework::Components::ComponentAdapter<MiniAudioPlaybackComponentController, MiniAudioPlaybackComponentConfig>;
    public:
        AZ_COMPONENT(MiniAudioPlaybackComponent, MiniAudioPlaybackComponentTypeId, BaseClass);
        MiniAudioPlaybackComponent() = default;
        explicit MiniAudioPlaybackComponent(const MiniAudioPlaybackComponentConfig& config);

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace MiniAudio
