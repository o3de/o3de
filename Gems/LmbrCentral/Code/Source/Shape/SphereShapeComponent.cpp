/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SphereShapeComponent.h"
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <Shape/ShapeComponentConverters.h>
#include <Shape/ShapeDisplay.h>

namespace LmbrCentral
{
    void SphereShapeComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ShapeService"));
        provided.push_back(AZ_CRC_CE("SphereShapeService"));
    }

    void SphereShapeComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("ShapeService"));
        incompatible.push_back(AZ_CRC_CE("SphereShapeService"));
        incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
    }

    void SphereShapeComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    void SphereShapeDebugDisplayComponent::Reflect(AZ::ReflectContext* context)
    {
        EntityDebugDisplayComponent::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SphereShapeDebugDisplayComponent, EntityDebugDisplayComponent>()
                ->Version(1)
                ->Field("Configuration", &SphereShapeDebugDisplayComponent::m_sphereShapeConfig)
                ;
        }
    }

    void SphereShapeDebugDisplayComponent::Activate()
    {
        EntityDebugDisplayComponent::Activate();
        ShapeComponentNotificationsBus::Handler::BusConnect(GetEntityId());
    }

    void SphereShapeDebugDisplayComponent::Deactivate()
    {
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        EntityDebugDisplayComponent::Deactivate();
    }

    void SphereShapeDebugDisplayComponent::Draw(AzFramework::DebugDisplayRequests& debugDisplay)
    {
        DrawSphereShape(m_sphereShapeConfig.GetDrawParams(), m_sphereShapeConfig, debugDisplay);
    }

    bool SphereShapeDebugDisplayComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const SphereShapeConfig*>(baseConfig))
        {
            m_sphereShapeConfig = *config;
            return true;
        }
        return false;
    }

    void SphereShapeDebugDisplayComponent::OnShapeChanged(ShapeChangeReasons changeReason)
    {
        if (changeReason == ShapeChangeReasons::ShapeChanged)
        {
            SphereShapeComponentRequestsBus::EventResult(m_sphereShapeConfig, GetEntityId(), &SphereShapeComponentRequests::GetSphereConfiguration);
        }
    }

    bool SphereShapeDebugDisplayComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<SphereShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_sphereShapeConfig;
            return true;
        }
        return false;
    }

    namespace ClassConverters
    {
        static bool DeprecateSphereColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
        static bool DeprecateSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
    }

    void SphereShapeConfig::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: SphereColliderConfiguration -> SphereShapeConfig
            serializeContext->ClassDeprecate(
                "SphereColliderConfiguration",
                AZ::Uuid("{0319AE62-3355-4C98-873D-3139D0427A53}"),
                &ClassConverters::DeprecateSphereColliderConfiguration)
                ;

            serializeContext->Class<SphereShapeConfig, ShapeComponentConfig>()
                ->Version(2)
                ->Field("Radius", &SphereShapeConfig::m_radius)
                ->Field("TranslationOffset", &SphereShapeConfig::m_translationOffset)
                ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<SphereShapeConfig>("Configuration", "Sphere shape configuration parameters")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SphereShapeConfig::m_radius, "Radius", "Radius of sphere")
                    ->Attribute(AZ::Edit::Attributes::Min, 0.f)
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.05f)
                    ->DataElement(
                        AZ::Edit::UIHandlers::Default,
                        &SphereShapeConfig::m_translationOffset,
                        "Translation Offset",
                        "Translation offset of shape relative to its entity")
                    ->Attribute(AZ::Edit::Attributes::Suffix, " m")
                    ->Attribute(AZ::Edit::Attributes::Step, 0.05f);
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<SphereShapeConfig>()
                ->Constructor()
                ->Constructor<float>()
                ->Property("Radius", BehaviorValueProperty(&SphereShapeConfig::m_radius))
                ;
        }
    }

    void SphereShapeComponent::Reflect(AZ::ReflectContext* context)
    {
        SphereShape::Reflect(context);

        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Deprecate: SphereColliderComponent -> SphereShapeComponent
            serializeContext->ClassDeprecate(
                "SphereColliderComponent",
                AZ::Uuid("{99F33E4A-4EFB-403C-8918-9171D47A03A4}"),
                &ClassConverters::DeprecateSphereColliderComponent)
                ;

            serializeContext->Class<SphereShapeComponent, AZ::Component>()
                ->Version(2, &ClassConverters::UpgradeSphereShapeComponent)
                ->Field("SphereShape", &SphereShapeComponent::m_sphereShape)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Constant("SphereShapeComponentTypeId", BehaviorConstant(SphereShapeComponentTypeId));

            behaviorContext->EBus<SphereShapeComponentRequestsBus>("SphereShapeComponentRequestsBus")
                ->Event("GetSphereConfiguration", &SphereShapeComponentRequestsBus::Events::GetSphereConfiguration)
                ->Event("SetRadius", &SphereShapeComponentRequestsBus::Events::SetRadius)
                ;
        }
    }

    void SphereShapeComponent::Activate()
    {
        m_sphereShape.Activate(GetEntityId());
    }

    void SphereShapeComponent::Deactivate()
    {
        m_sphereShape.Deactivate();
    }

    bool SphereShapeComponent::ReadInConfig(const AZ::ComponentConfig* baseConfig)
    {
        if (const auto config = azrtti_cast<const SphereShapeConfig*>(baseConfig))
        {
            m_sphereShape.SetSphereConfiguration(*config);
            return true;
        }
        return false;
    }

    bool SphereShapeComponent::WriteOutConfig(AZ::ComponentConfig* outBaseConfig) const
    {
        if (auto outConfig = azrtti_cast<SphereShapeConfig*>(outBaseConfig))
        {
            *outConfig = m_sphereShape.GetSphereConfiguration();
            return true;
        }
        return false;
    }

    namespace ClassConverters
    {
        static bool DeprecateSphereColliderConfiguration(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="SphereColliderConfiguration" field="Configuration" version="1" type="{0319AE62-3355-4C98-873D-3139D0427A53}">
            <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>

            New:
            <Class name="SphereShapeConfig" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
            <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
            </Class>
            */

            // Cache the Radius
            float oldRadius = 0.f;
            const int oldIndex = classElement.FindElement(AZ_CRC_CE("Radius"));
            if (oldIndex != -1)
            {
                classElement.GetSubElement(oldIndex).GetData<float>(oldRadius);
            }
            else
            {
                return false;
            }

            // Convert to SphereShapeConfig
            const bool result = classElement.Convert<SphereShapeConfig>(context);
            if (result)
            {
                const int newIndex = classElement.AddElement<float>(context, "Radius");
                if (newIndex != -1)
                {
                    classElement.GetSubElement(newIndex).SetData<float>(context, oldRadius);
                    return true;
                }

                return false;
            }

            return false;
        }

        static bool DeprecateSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            /*
            Old:
            <Class name="SphereColliderComponent" version="1" type="{99F33E4A-4EFB-403C-8918-9171D47A03A4}">
             <Class name="SphereColliderConfiguration" field="Configuration" version="1" type="{0319AE62-3355-4C98-873D-3139D0427A53}">
              <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>

            New:
            <Class name="SphereShapeComponent" version="1" type="{E24CBFF0-2531-4F8D-A8AB-47AF4D54BCD2}">
             <Class name="SphereShapeConfig" field="Configuration" version="1" type="{4AADFD75-48A7-4F31-8F30-FE4505F09E35}">
              <Class name="float" field="Radius" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
             </Class>
            </Class>
            */

            // Cache the Configuration
            SphereShapeConfig configuration;
            int configIndex = classElement.FindElement(AZ_CRC_CE("Configuration"));
            if (configIndex != -1)
            {
                classElement.GetSubElement(configIndex).GetData<SphereShapeConfig>(configuration);
            }
            else
            {
                return false;
            }

            // Convert to SphereShapeComponent
            const bool result = classElement.Convert<SphereShapeComponent>(context);
            if (result)
            {
                configIndex = classElement.AddElement<SphereShapeConfig>(context, "Configuration");
                if (configIndex != -1)
                {
                    classElement.GetSubElement(configIndex).SetData<SphereShapeConfig>(context, configuration);
                    return true;
                }

                return false;
            }

            return false;
        }
    } // namespace ClassConverters
} // namespace LmbrCentral
