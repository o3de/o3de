/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CoreLights/CoreLightsSystemComponent.h>

#include <AzCore/Console/Console.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

#include <Atom/Feature/RenderCommon.h>
#include <Atom/Feature/CoreLights/EsmShadowmapsPassData.h>
#include <Atom/Feature/CoreLights/PhotometricValue.h>
#include <Atom/Feature/CoreLights/ShadowConstants.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Shader/ShaderSystem.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <CoreLights/CascadedShadowmapsPass.h>
#include <CoreLights/SimplePointLightFeatureProcessor.h>
#include <CoreLights/SimpleSpotLightFeatureProcessor.h>
#include <CoreLights/CapsuleLightFeatureProcessor.h>
#include <CoreLights/DepthExponentiationPass.h>
#include <CoreLights/DirectionalLightFeatureProcessor.h>
#include <CoreLights/DiskLightFeatureProcessor.h>
#include <CoreLights/EsmShadowmapsPass.h>
#include <CoreLights/PointLightFeatureProcessor.h>
#include <CoreLights/PolygonLightFeatureProcessor.h>
#include <CoreLights/QuadLightFeatureProcessor.h>
#include <CoreLights/ShadowmapPass.h>
#include <CoreLights/ProjectedShadowmapsPass.h>

namespace AZ
{
    namespace Render
    {
        AZ_CVAR(bool,
            r_validateAreaLights,
            false,
            [](const bool& value)
            {
                RPI::ShaderSystemInterface::Get()->SetGlobalShaderOption(AZ::Name{ "o_area_light_validation" }, AZ::RPI::ShaderOptionValue{ value });
            },
            ConsoleFunctorFlags::Null,
            "Turns on a much more accurate an expensive mode for area lights for validating the accuracy of the inexpensive versions."
        );

        void CoreLightsSystemComponent::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context))
            {
                serializeContext->Class<CoreLightsSystemComponent, Component>()
                    ->Version(0)
                    ;

                auto enumBuilder = serializeContext->Enum<ShadowmapSize>();
                for (auto item : ShadowmapSizeMembers)
                {
                    enumBuilder->Value(item.m_string.data(), item.m_value);
                }
            }

            PhotometricValue::Reflect(context);
            SimplePointLightFeatureProcessor::Reflect(context);
            SimpleSpotLightFeatureProcessor::Reflect(context);
            PointLightFeatureProcessor::Reflect(context);
            DirectionalLightFeatureProcessor::Reflect(context);
            DiskLightFeatureProcessor::Reflect(context);
            CapsuleLightFeatureProcessor::Reflect(context);
            QuadLightFeatureProcessor::Reflect(context);
            PolygonLightFeatureProcessor::Reflect(context);

            EsmShadowmapsPassData::Reflect(context);

            if (auto* behaviorContext = azrtti_cast<BehaviorContext*>(context))
            {
                behaviorContext->Class<ShadowmapSize>()
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::None)>("ShadowmapSize_None")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size256)>("ShadowmapSize_256")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size512)>("ShadowmapSize_512")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size1024)>("ShadowmapSize_1024")
                    ->Enum<static_cast<uint32_t>(ShadowmapSize::Size2048)>("ShadowmapSize_2048")
                    ;
            }
        }

        void CoreLightsSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("CoreLightsService"));
        }

        void CoreLightsSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("CoreLightsService"));
        }

        void CoreLightsSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
        }

        void CoreLightsSystemComponent::Init()
        {
        }

        void CoreLightsSystemComponent::Activate()
        {
            AZ::ApplicationTypeQuery appType;
            ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Events::QueryApplicationType, appType);
            if (appType.IsHeadless())
            {
                return;
            }

            m_ltcCommonInterface.reset(aznew LtcCommon());

            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<SimplePointLightFeatureProcessor, SimplePointLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<SimpleSpotLightFeatureProcessor, SimpleSpotLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<PointLightFeatureProcessor, PointLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<DirectionalLightFeatureProcessor, DirectionalLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<DiskLightFeatureProcessor, DiskLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<CapsuleLightFeatureProcessor, CapsuleLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<QuadLightFeatureProcessor, QuadLightFeatureProcessorInterface>();
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessorWithInterface<PolygonLightFeatureProcessor, PolygonLightFeatureProcessorInterface>();

            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");
            passSystem->AddPassCreator(Name("CascadedShadowmapsPass"), &CascadedShadowmapsPass::Create);
            passSystem->AddPassCreator(Name("DepthExponentiationPass"), &DepthExponentiationPass::Create);
            passSystem->AddPassCreator(Name("EsmShadowmapsPass"), &EsmShadowmapsPass::Create);
            passSystem->AddPassCreator(Name("ShadowmapPass"), &ShadowmapPass::Create);
            passSystem->AddPassCreator(Name("ProjectedShadowmapsPass"), &ProjectedShadowmapsPass::Create);

            // Add the ShadowmapPassTemplate to the pass system. It will be cleaned up automatically when the pass system shuts down
            ShadowmapPass::CreatePassTemplate();
        }

        void CoreLightsSystemComponent::Deactivate()
        {
            m_ltcCommonInterface.reset();
        }
    } // namespace Render
} // namespace AZ

