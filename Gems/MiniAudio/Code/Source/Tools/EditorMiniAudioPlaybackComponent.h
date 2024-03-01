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
#include <AzCore/Component/TickBus.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <Clients/MiniAudioPlaybackComponent.h>
#include <Clients/MiniAudioPlaybackComponentController.h>
#include <ToolsComponents/EditorComponentAdapter.h>

namespace MiniAudio
{
    class EditorMiniAudioPlaybackComponent
        : public AzToolsFramework::Components::EditorComponentAdapter<MiniAudioPlaybackComponentController,
            MiniAudioPlaybackComponent, MiniAudioPlaybackComponentConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<MiniAudioPlaybackComponentController, MiniAudioPlaybackComponent, MiniAudioPlaybackComponentConfig>;
        AZ_EDITOR_COMPONENT(EditorMiniAudioPlaybackComponent, EditorMiniAudioPlaybackComponentTypeId, BaseClass);
        static void Reflect(AZ::ReflectContext* context);

        EditorMiniAudioPlaybackComponent() = default;
        explicit EditorMiniAudioPlaybackComponent(const MiniAudioPlaybackComponentConfig& config);

        void Activate() override;
        void Deactivate() override;

        AZ::u32 OnConfigurationChanged() override;

        AZ::Crc32 PlaySoundInEditor();
        AZ::Crc32 StopSoundInEditor();
        AZ::Crc32 PauseSoundInEditor();
        AZ::Crc32 OnVolumeChanged();
    };

} // namespace MiniAudio
