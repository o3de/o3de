/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Libraries.h"

#include <Libraries/Comparison/ComparisonLibrary.h>
#include <Libraries/Core/CoreLibrary.h>
#include <Libraries/Logic/LogicLibrary.h>
#include <Libraries/Operators/OperatorsLibrary.h>
#include <Libraries/UnitTesting/UnitTestingLibrary.h>

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>



namespace ScriptCanvas
{
    void InitLibraries()
    {
        auto nodeRegistry = NodeRegistry::GetInstance();
        ComparisonLibrary::InitNodeRegistry(nodeRegistry);
        CoreLibrary::InitNodeRegistry(nodeRegistry);
        LogicLibrary::InitNodeRegistry(nodeRegistry);
        OperatorsLibrary::InitNodeRegistry(nodeRegistry);
    }

    void ResetLibraries()
    {
        NodeRegistry::ResetInstance();
    }

    class TestEbusInterface : public AZ::EBusTraits
    {
    public:

        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;

        using BusIdType = AZStd::string;

        virtual void Test(float) {}

    };

    using TestEBus = AZ::EBus<TestEbusInterface>;

    struct SpawnDecalConfig
    {
        AZ_RTTI(SpawnDecalConfig, "{FC3DA616-174B-48FD-9BFB-BC277132FB47}");

        float m_scale = 1.0f;             // Scale in meters
        float m_opacity = 1.0f;           // How visible the decal is
        float m_attenutationAngle = 1.0f; // How much to attenuate based on the angle of the geometry vs the decal
        float m_lifeTime = 0.0f;          // Time until the decal begins to fade, in seconds.
        float m_fadeTime = 1.0f;          // Time it takes the decal to fade, in seconds.
        uint8_t m_sortKey = 0;            // Higher numbers sort in front of lower numbers

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<SpawnDecalConfig>()
                    ->Version(0)
                    ->Field("Scale", &SpawnDecalConfig::m_scale)
                    ->Field("Opacity", &SpawnDecalConfig::m_opacity)
                    ->Field("AttenuationAngle", &SpawnDecalConfig::m_attenutationAngle)
                    ->Field("LifeTime", &SpawnDecalConfig::m_lifeTime)
                    ->Field("FadeTime", &SpawnDecalConfig::m_fadeTime)
                    ->Field("SortKey", &SpawnDecalConfig::m_sortKey)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<SpawnDecalConfig>("SpawnDecalConfig", "Configuration settings for spawning a decal.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnDecalConfig::m_scale, "Scale", "The scale of the decal.")
                        ->DataElement(AZ::Edit::UIHandlers::Slider, &SpawnDecalConfig::m_opacity, "Opacity", "The opacity of the decal.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnDecalConfig::m_attenutationAngle, "Angle attenuation", "How much to attenuate the opacity of the decal based on the different in the angle between the decal and the surface.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnDecalConfig::m_lifeTime, "Life time", "How long before the decal should begin to fade out, in seconds.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnDecalConfig::m_fadeTime, "Fade time", "How long the decal should spend fading out at the end of its life time.")
                        ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &SpawnDecalConfig::m_sortKey, "Sort key", "Used to sort the decal with other decals. Higher numbered decals show on top of lower number decals.")
                        ;
                }
            }

            if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->EBus<TestEBus>("TestEBus")
                    ->Attribute(AZ_CRC_CE("AddressNameOverride"), "POTATO")
                    ->Event("Test", &TestEBus::Events::Test,
                        {{{ "Source","Address", behaviorContext->MakeDefaultValue(AZStd::string_view("DefaultAddress")) }}}
                    )
                        ->Attribute(AZ_CRC_CE("AddressNameOverride"), "POTATO")
                    
                    ;

                behaviorContext->Class<SpawnDecalConfig>("SpawnDecalConfig")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Category, "Graphics")
                    ->Attribute(AZ::Script::Attributes::Module, "decals")
                    ->Constructor()
                    ->Constructor<const SpawnDecalConfig&>()
                    ->Property("scale", BehaviorValueProperty(&SpawnDecalConfig::m_scale))
                    ->Property("opacity", BehaviorValueProperty(&SpawnDecalConfig::m_opacity))
                    ->Property("m_attenutationAngle", BehaviorValueProperty(&SpawnDecalConfig::m_attenutationAngle))
                    ->Property("m_lifeTime", BehaviorValueProperty(&SpawnDecalConfig::m_lifeTime))
                    ->Property("m_sortKey", BehaviorValueProperty(&SpawnDecalConfig::m_sortKey))
                    ;
            }
        }
    };


    void ReflectLibraries(AZ::ReflectContext* reflectContext)
    {
        SpawnDecalConfig::Reflect(reflectContext);

        CoreLibrary::Reflect(reflectContext);
        LogicLibrary::Reflect(reflectContext);
        OperatorsLibrary::Reflect(reflectContext);

#ifndef _RELEASE
        UnitTestingLibrary::Reflect(reflectContext);
#endif
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors(ComparisonLibrary::GetComponentDescriptors());

        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors = CoreLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = LogicLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = OperatorsLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        return libraryDescriptors;
    }

    AZ::EnvironmentVariable<NodeRegistry> GetNodeRegistry()
    {
        return AZ::Environment::FindVariable<NodeRegistry>(s_nodeRegistryName);
    }
}
