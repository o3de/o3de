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
#include <Clients/MiniAudioListenerComponent.h>
#include <Clients/MiniAudioListenerComponentController.h>
#include <ToolsComponents/EditorComponentAdapter.h>

namespace MiniAudio
{
    class EditorMiniAudioListenerComponent
        : public AzToolsFramework::Components::EditorComponentAdapter<MiniAudioListenerComponentController,
            MiniAudioListenerComponent, MiniAudioListenerComponentConfig>
    {
    public:
        using BaseClass = AzToolsFramework::Components::EditorComponentAdapter<MiniAudioListenerComponentController, MiniAudioListenerComponent, MiniAudioListenerComponentConfig>;
        AZ_EDITOR_COMPONENT(EditorMiniAudioListenerComponent, EditorMiniAudioListenerComponentTypeId, BaseClass);
        static void Reflect(AZ::ReflectContext* context);

        EditorMiniAudioListenerComponent() = default;
        explicit EditorMiniAudioListenerComponent(const MiniAudioListenerComponentConfig& config);

        void Activate() override;
        void Deactivate() override;

        AZ::u32 OnConfigurationChanged() override;
    };

} // namespace MiniAudio
