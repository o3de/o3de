/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <Clients/MiniAudioSystemComponent.h>
#include <Tools/SoundAssetBuilder.h>

namespace MiniAudio
{
    /// System component for MiniAudio editor
    class MiniAudioEditorSystemComponent
        : public MiniAudioSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = MiniAudioSystemComponent;

    public:
        AZ_COMPONENT(MiniAudioEditorSystemComponent, "{C221724F-CCA2-454E-97A9-E418A91CB072}", BaseSystemComponent);
        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        MiniAudioEditorSystemComponent() = default;
        MiniAudioEditorSystemComponent(const MiniAudioEditorSystemComponent&) = delete;
        MiniAudioEditorSystemComponent& operator=(const MiniAudioEditorSystemComponent&) = delete;

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        SoundAssetBuilder m_soundAssetBuilder;
    };
} // namespace MiniAudio
