/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorAudioAreaEnvironmentComponent.h"

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace LmbrCentral
{
    //=========================================================================
    namespace ClassConverters
    {
        static bool UpgradeEditorAudioAreaEnvironmentComponent(AZ::SerializeContext&, AZ::SerializeContext::DataElementNode&);
    } // namespace ClassConverters

    //=========================================================================
    void EditorAudioAreaEnvironmentComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<EditorAudioAreaEnvironmentComponent, EditorComponentBase>()
                ->Version(2, &ClassConverters::UpgradeEditorAudioAreaEnvironmentComponent)
                ->Field("Broad-phase Trigger Area entity", &EditorAudioAreaEnvironmentComponent::m_broadPhaseTriggerArea)
                ->Field("Environment name", &EditorAudioAreaEnvironmentComponent::m_environmentName)
                ->Field("Environment fade distance", &EditorAudioAreaEnvironmentComponent::m_environmentFadeDistance)
                ;

            if (auto editContext = serializeContext->GetEditContext())
            {
                editContext->Class<EditorAudioAreaEnvironmentComponent>("Audio Area Environment", "The Audio Area Environment component enables entities that are moving around and throughout a shape to have environment effects applied to any sounds that they trigger")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Audio")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/AudioAreaEnvironment.svg")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/AudioAreaEnvironment.svg")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components/reference/audio/area-environment/")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioAreaEnvironmentComponent::m_broadPhaseTriggerArea,
                        "Broad-phase trigger area", "The entity that contains a Trigger Area component for broad-phase checks")
                        ->Attribute(AZ::Edit::Attributes::RequiredService, AZ_CRC_CE("ProximityTriggerService"))
                    ->DataElement("AudioControl", &EditorAudioAreaEnvironmentComponent::m_environmentName,
                        "Environment name", "The name of the ATL Environment to use")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &EditorAudioAreaEnvironmentComponent::m_environmentFadeDistance,
                        "Fade distance", "Distance around the area shape that the environment amounts will fade")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.01f)
                    ;
            }
        }
    }

    //=========================================================================
    EditorAudioAreaEnvironmentComponent::EditorAudioAreaEnvironmentComponent()
    {
        m_environmentName.m_propertyType = AzToolsFramework::AudioPropertyType::Environment;
    }

    //=========================================================================
    void EditorAudioAreaEnvironmentComponent::BuildGameEntity(AZ::Entity* gameEntity)
    {
        if (auto audioAreaEnvironmentComponent = gameEntity->CreateComponent<AudioAreaEnvironmentComponent>())
        {
            audioAreaEnvironmentComponent->m_broadPhaseTriggerArea = m_broadPhaseTriggerArea;
            audioAreaEnvironmentComponent->m_environmentName = m_environmentName.m_controlName;
            audioAreaEnvironmentComponent->m_environmentFadeDistance = m_environmentFadeDistance;
        }
    }

    //=========================================================================
    namespace ClassConverters
    {
        static bool UpgradeEditorAudioAreaEnvironmentComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() == 1)
            {
                // upgrade V1 to V2
                AZStd::string oldEnvironmentName;

                int environmentIndex = classElement.FindElement(AZ::Crc32("Environment name"));
                if (environmentIndex == -1)
                {
                    AZ_Error("Serialization", false, "Failed to find old Environment name.");
                    return false;
                }

                auto environmentNode = classElement.GetSubElement(environmentIndex);
                if (!environmentNode.GetData<AZStd::string>(oldEnvironmentName))
                {
                    AZ_Error("Serialization", false, "Failed to retrieve old Environment name.");
                    return false;
                }

                classElement.RemoveElement(environmentIndex);

                AzToolsFramework::CReflectedVarAudioControl newEnvironment;
                newEnvironment.m_propertyType = AzToolsFramework::AudioPropertyType::Environment;
                newEnvironment.m_controlName = oldEnvironmentName;
                if (classElement.AddElementWithData(context, "Environment name", newEnvironment) == -1)
                {
                    AZ_Error("Serialization", false, "Failed to replace Environment name with new version.");
                    return false;
                }
            }

            return true;
        }
    } // namespace ClassConverters

} // namespace LmbrCentral
