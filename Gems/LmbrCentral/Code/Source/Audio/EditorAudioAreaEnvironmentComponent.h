/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

#include <IAudioSystem.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyAudioCtrlTypes.h>

#include "AudioAreaEnvironmentComponent.h"

namespace LmbrCentral
{
    /*!
     * EditorAudioAreaEnvironmentComponent
     */
    class EditorAudioAreaEnvironmentComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_EDITOR_COMPONENT(EditorAudioAreaEnvironmentComponent, "{6CCCEAA1-02B2-4DE8-B93D-26F1509346A8}",
            AzToolsFramework::Components::EditorComponentBase);

        EditorAudioAreaEnvironmentComponent();

        void BuildGameEntity(AZ::Entity* gameEntity) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            LmbrCentral::AudioAreaEnvironmentComponent::GetProvidedServices(provided);
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            LmbrCentral::AudioAreaEnvironmentComponent::GetRequiredServices(required);
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            LmbrCentral::AudioAreaEnvironmentComponent::GetIncompatibleServices(incompatible);
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        //! Serialized data
        AZ::EntityId m_broadPhaseTriggerArea;
        AzToolsFramework::CReflectedVarAudioControl m_environmentName;
        float m_environmentFadeDistance = 1.f;
    };

} // namespace LmbrCentral
