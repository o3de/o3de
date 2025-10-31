/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CapsuleShapeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeComponentConverters.h>
#include <Shape/ShapeDisplay.h>
#include <LmbrCentral/Geometry/GeometrySystemComponentBus.h>

namespace LmbrCentral
{
    void CapsuleShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShapeService"));
        provided.push_back(AZ_CRC_CE("CapsuleShapeService"));
    }

    void CapsuleShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShapeService"));
        incompatible.push_back(AZ_CRC_CE("CapsuleShapeService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void CapsuleShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void CapsuleShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CapsuleShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Configuration", &CapsuleShapeDebugDisplayComponent::m_capsuleShapeConfig)
                ;
        }
    }

    void CapsuleShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
        GenerateVertices();
    }

    void CapsuleShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void CapsuleShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawShape(debugDisplay, m_capsuleShapeConfig.GetDrawParams(), m_capsuleShapeMesh, m_capsuleShapeConfig.m_translationOffset);
    }

    bool CapsuleShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const CapsuleShapeConfig*>(baseConfig))
        {
            m_capsuleShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool CapsuleShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<CapsuleShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_capsuleShapeConfig;
            return true;
        }
        return false;
    }

    void CapsuleShapeDebugDisplayComponent::GenerateVertices()
    {
        CapsuleGeometrySystemRequestBus::Broadcast(
            &CapsuleGeometrySystemRequestBus::Events::GenerateCapsuleMesh,
            m_capsuleShapeConfig.m_radius,
            m_capsuleShapeConfig.m_height,
            g_capsuleDebugShapeSides,
            g_capsuleDebugShapeCapSegments,
            m_capsuleShapeMesh.m_vertexBuffer,
            m_capsuleShapeMesh.m_indexBuffer,
            m_capsuleShapeMesh.m_lineBuffer
        );
    }

    void CapsuleShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            CapsuleShapeComponentRequestsBus::EventResult(m_capsuleShapeConfig, GetEntityId(), &CapsuleShapeComponentRequests::GetCapsuleConfiguration);
            GenerateVertices();
        }
    }

    namespace ClassConverters
    {
        static bool DeprecateCapsuleColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static bool DeprecateCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void CapsuleShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: CapsuleColliderConfiguration -> CapsuleShapeConfig
            serializeContext->ClassDeprecate(
                "CapsuleColliderConfiguration",
                AZ::Uuid("{902BCDA9-C9E5-429C-991B-74C241ED2889}"),
                &ClassConverters::DeprecateCapsuleColliderConfiguration)
                ;

            serializeContext->Class<CapsuleShapeConfig, ShapeComponentConfig>()
                ->Version(2)
                ->Field("Height", &CapsuleShapeConfig::m_height)
                ->Field("Radius", &CapsuleShapeConfig::m_radius)
                ->Field("TranslationOffset", &CapsuleShapeConfig::m_translationOffset)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CapsuleShapeConfig>("Configuration", "Capsule shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &CapsuleShapeConfig::m_height,
                        "Height",
                        "End to end height of capsule, this includes the cylinder and both caps")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CapsuleShapeConfig::m_radius, "Radius", "Radius of capsule")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &CapsuleShapeConfig::m_translationOffset,
                        "Translation Offset",
                        "Translation offset of shape relative to its entity")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.05f);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<CapsuleShapeConfig>()
                ->Property("Height", BehaviorValueProperty(&CapsuleShapeConfig::m_height))
                ->Property("Radius", BehaviorValueProperty(&CapsuleShapeConfig::m_radius))
                ;
        }
    }

    void CapsuleShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        CapsuleShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: CapsuleColliderComponent -> CapsuleShapeComponent
            serializeContext->ClassDeprecate(
                "CapsuleColliderComponent",
                AZ::Uuid("{D1F746A9-FC24-48E4-88DE-5B3122CB6DE7}"),
                &ClassConverters::DeprecateCapsuleColliderComponent
                );

            serializeContext->Class<CapsuleShapeComponent, AZ::Component>()
                ->Version(2, &ClassConverters::UpgradeCapsuleShapeComponent)
                ->Field("CapsuleShape", &CapsuleShapeComponent::m_capsuleShape)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("CapsuleShapeComponentTypeId", BehaviorConstant(CapsuleShapeComponentTypeId));

            behaviorContext->EBus<CapsuleShapeComponentRequestsBus>("CapsuleShapeComponentRequestsBus")
                ->Event("GetCapsuleConfiguration", &CapsuleShapeComponentRequestsBus::Events::GetCapsuleConfiguration)
                ->Event("SetHeight", &CapsuleShapeComponentRequestsBus::Events::SetHeight)
                ->Event("SetRadius", &CapsuleShapeComponentRequestsBus::Events::SetRadius)
                ;
        }
    }

    void CapsuleShapeComponent::Activate()
    {
        m_capsuleShape.Activate(GetEntityId());
    }

    void CapsuleShapeComponent::Deactivate()
    {
        m_capsuleShape.Deactivate();
    }

    bool CapsuleShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const CapsuleShapeConfig*>(baseConfig))
        {
            m_capsuleShape.SetCapsuleConfiguration(*config);
            return true;
        }
        return false;
    }

    bool CapsuleShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<CapsuleShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_capsuleShape.GetCapsuleConfiguration();
            return true;
        }
        return false;
    }

    namespace ClassConverters
    {
        static bool DeprecateCapsuleColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CapsuleColliderConfiguration" field="Configuration" version="1" type="{902BCDA9-C9E5-429C-991B-74C241ED2889}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="CapsuleShapeConfig" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            */

            // Cache the Height and Radius
            float oldHeight = 0.f;
            float oldRadius = 0.f;

            int oldIndex = classElement.FindElement(AZ_CRC_CE("Height"));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldHeight);
            }

            oldIndex = classElement.FindElement(AZ_CRC_CE("Radius"));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldRadius);
            }

            // Convert to CapsuleShapeConfig
            const bool result = classElement.Convert<CapsuleShapeConfig>(context);
            if (result)
            {
                int newIndex = classElement.AddElement<float>(context, "Height");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldHeight);
                }
                else
                {
                    return false;
                }

                newIndex = classElement.AddElement<float>(context, "Radius");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldRadius);
                    return true;
                }
            }

            return false;
        }

        static bool DeprecateCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CapsuleColliderComponent" version="1" type="{D1F746A9-FC24-48E4-88DE-5B3122CB6DE7}">
             <Class name="CapsuleColliderConfiguration" field="Configuration" version="1" type="{902BCDA9-C9E5-429C-991B-74C241ED2889}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="CapsuleShapeComponent" version="1" type="{967EC13D-364D-4696-AB5C-C00CC05A2305}">
             <Class name="CapsuleShapeConfig" field="Configuration" version="1" type="{00931AEB-2AD8-42CE-B1DC-FA4332F51501}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            CapsuleShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CapsuleShapeConfig>(configuration);
            }

            // Convert to CapsuleShapeComponent
            const bool result = classElement.Convert<CapsuleShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<CapsuleShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CapsuleShapeConfig>(context, configuration);
                }
                return true;
            }
            return false;
        }

    } // namespace ClassConverters
} // namespace LmbrCentral
