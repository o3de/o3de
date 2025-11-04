/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CylinderShapeComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>
#include <Shape/ShapeComponentConverters.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void CylinderShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShapeService"));
        provided.push_back(AZ_CRC_CE("CylinderShapeService"));
    }

    void CylinderShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShapeService"));
        incompatible.push_back(AZ_CRC_CE("CylinderShapeService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void CylinderShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void CylinderShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CylinderShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Configuration", &CylinderShapeDebugDisplayComponent::m_cylinderShapeConfig)
                ;
        }
    }

    void CylinderShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
    }

    void CylinderShapeDebugDisplayComponent::Deactivate() 
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void CylinderShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawCylinderShape(m_cylinderShapeConfig.GetDrawParams(), m_cylinderShapeConfig, debugDisplay);
    }

    bool CylinderShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const CylinderShapeConfig*>(baseConfig))
        {
            m_cylinderShapeConfig = *config;
            return true;
        }
        return false;
    }

    bool CylinderShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<CylinderShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_cylinderShapeConfig;
            return true;
        }
        return false;
    }

    void CylinderShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            CylinderShapeComponentRequestsBus::EventResult(m_cylinderShapeConfig, GetEntityId(), &CylinderShapeComponentRequests::GetCylinderConfiguration);
        }
    }

    namespace ClassConverters
    {
        static bool DeprecateCylinderColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static bool DeprecateCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void CylinderShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: CylinderColliderConfiguration -> CylinderShapeConfig
            serializeContext->ClassDeprecate(
                "CylinderColliderConfiguration",
                AZ::Uuid("{E1DCB833-EFC4-43AC-97B0-4E07AA0DFAD9}"),
                &ClassConverters::DeprecateCylinderColliderConfiguration)
                ;

            serializeContext->Class<CylinderShapeConfig, ShapeComponentConfig>()
                ->Version(2)
                ->Field("Height", &CylinderShapeConfig::m_height)
                ->Field("Radius", &CylinderShapeConfig::m_radius)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CylinderShapeConfig>("Configuration", "Cylinder shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShapeConfig::m_height, "Height", "Height of cylinder")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000000.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.1f)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CylinderShapeConfig::m_radius, "Radius", "Radius of cylinder")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1000000.f)
                        ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                        ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<CylinderShapeConfig>()
                ->Property("Height", BehaviorValueProperty(&CylinderShapeConfig::m_height))
                ->Property("Radius", BehaviorValueProperty(&CylinderShapeConfig::m_radius))
                ;
        }
    }

    void CylinderShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        CylinderShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->ClassDeprecate(
                "CylinderColliderComponent",
                AZ::Uuid("{A43F684B-07B6-4CD7-8D59-643709DF9486}"),
                &ClassConverters::DeprecateCylinderColliderComponent)
                ;

            serializeContext->Class<CylinderShapeComponent, AZ::Component>()
                ->Version(2, &ClassConverters::UpgradeCylinderShapeComponent)
                ->Field("CylinderShape", &CylinderShapeComponent::m_cylinderShape)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("CylinderShapeComponentTypeId", BehaviorConstant(CylinderShapeComponentTypeId));

            behaviorContext->EBus<CylinderShapeComponentRequestsBus>("CylinderShapeComponentRequestsBus")
                ->Event("GetCylinderConfiguration", &CylinderShapeComponentRequestsBus::Events::GetCylinderConfiguration)
                ->Event("SetHeight", &CylinderShapeComponentRequestsBus::Events::SetHeight)
                ->Event("SetRadius", &CylinderShapeComponentRequestsBus::Events::SetRadius)
                ;
        }
    }

    void CylinderShapeComponent::Activate()
    {
        m_cylinderShape.Activate(GetEntityId());
    }

    void CylinderShapeComponent::Deactivate()
    {
        m_cylinderShape.Deactivate();
    }

    bool CylinderShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const CylinderShapeConfig*>(baseConfig))
        {
            m_cylinderShape.SetCylinderConfiguration(*config);
            return true;
        }
        return false;
    }

    bool CylinderShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<CylinderShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_cylinderShape.GetCylinderConfiguration();
            return true;
        }
        return false;
    }

    namespace ClassConverters
    {
        static bool DeprecateCylinderColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CylinderColliderConfiguration" field="Configuration" version="1" type="{E1DCB833-EFC4-43AC-97B0-4E07AA0DFAD9}">
             <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="CylinderShapeConfig" field="Configuration" version="1" type="{53254779-82F1-441E-9116-81E1FACFECF4}">
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

            // Convert to CylinderShapeConfig
            const bool result = classElement.Convert<CylinderShapeConfig>(context);
            if (result)
            {
                int newIndex = classElement.AddElement<float>(context, "Height");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldHeight);
                }

                newIndex = classElement.AddElement<float>(context, "Radius");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldRadius);
                }

                return true;
            }

            return false;
        }

        static bool DeprecateCylinderColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="CylinderColliderComponent" version="1" type="{A43F684B-07B6-4CD7-8D59-643709DF9486}">
             <Class name="CylinderColliderConfiguration" field="Configuration" version="1" type="{E1DCB833-EFC4-43AC-97B0-4E07AA0DFAD9}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="CylinderShapeComponent" version="1" type="{B0C6AA97-E754-4E33-8D32-33E267DB622F}">
             <Class name="CylinderShapeConfig" field="Configuration" version="1" type="{53254779-82F1-441E-9116-81E1FACFECF4}">
              <Class name="float" field="Height" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
              <Class name="float" field="Radius" value="0.2500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            CylinderShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<CylinderShapeConfig>(configuration);
            }

            // Convert to CylinderShapeComponent
            const bool result = classElement.Convert<CylinderShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<CylinderShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<CylinderShapeConfig>(context, configuration);
                }

                return true;
            }

            return false;
        }
    } // namespace ClassConverters
} // namespace LmbrCentral
