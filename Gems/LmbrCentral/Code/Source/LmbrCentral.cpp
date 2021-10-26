/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LmbrCentral.h"

#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/std/containers/list.h>
#include <AzCore/Memory/AllocatorScope.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Metrics/MetricsPlainTextNameRegistration.h>

// Component descriptors
#include "Audio/AudioAreaEnvironmentComponent.h"
#include "Audio/AudioEnvironmentComponent.h"
#include "Audio/AudioListenerComponent.h"
#include "Audio/AudioMultiPositionComponent.h"
#include "Audio/AudioPreloadComponent.h"
#include "Audio/AudioProxyComponent.h"
#include "Audio/AudioRtpcComponent.h"
#include "Audio/AudioSwitchComponent.h"
#include "Audio/AudioSystemComponent.h"
#include "Audio/AudioTriggerComponent.h"
#include "Bundling/BundlingSystemComponent.h"
#include "Ai/NavigationComponent.h"
#include "Scripting/TagComponent.h"
#include "Scripting/SimpleStateComponent.h"
#include "Scripting/SpawnerComponent.h"
#include "Scripting/LookAtComponent.h"
#include "Scripting/RandomTimedSpawnerComponent.h"
#include "Ai/NavigationSystemComponent.h"
#include "Geometry/GeometrySystemComponent.h"
#include <Asset/AssetSystemDebugComponent.h>

// Unhandled asset types
// Material
#include "Unhandled/Material/MaterialAssetTypeInfo.h"
// Other
#include "Unhandled/Other/AudioAssetTypeInfo.h"
#include "Unhandled/Other/CharacterPhysicsAssetTypeInfo.h"
#include "Unhandled/Other/GroupAssetTypeInfo.h"
#include "Unhandled/Other/PrefabsLibraryAssetTypeInfo.h"
#include "Unhandled/Other/GameTokenAssetTypeInfo.h"
#include "Unhandled/Other/EntityPrototypeLibraryAssetTypeInfo.h"

// Texture
#include "Unhandled/Texture/SubstanceAssetTypeInfo.h"
#include "Unhandled/Texture/TextureAssetTypeInfo.h"
// Hidden
#include "Unhandled/Hidden/TextureMipmapAssetTypeInfo.h"
//UI
#include <Unhandled/UI/EntityIconAssetTypeInfo.h>
#include "Unhandled/UI/FontAssetTypeInfo.h"
#include "Unhandled/UI/UICanvasAssetTypeInfo.h"

// Asset types
#include <AzCore/Slice/SliceAsset.h>
#include <AzCore/Script/ScriptAsset.h>
#include <LmbrCentral/Rendering/MaterialAsset.h>
#include <LmbrCentral/Rendering/MeshAsset.h>
#include <LmbrCentral/Rendering/MaterialHandle.h>

// Scriptable Ebus Registration
#include "Events/ReflectScriptableEvents.h"

// Shape components
#include "Shape/SphereShapeComponent.h"
#include "Shape/DiskShapeComponent.h"
#include "Shape/AxisAlignedBoxShapeComponent.h"
#include "Shape/BoxShapeComponent.h"
#include "Shape/QuadShapeComponent.h"
#include "Shape/CylinderShapeComponent.h"
#include "Shape/CapsuleShapeComponent.h"
#include "Shape/TubeShapeComponent.h"
#include "Shape/CompoundShapeComponent.h"
#include "Shape/SplineComponent.h"
#include "Shape/PolygonPrismShapeComponent.h"

namespace LmbrCentral
{
    using LmbrCentralAllocatorScope = AZ::AllocatorScope<AZ::LegacyAllocator>;

    // This component boots the required allocators for LmbrCentral everywhere but AssetBuilders
    class LmbrCentralAllocatorComponent
        : public AZ::Component
        , protected LmbrCentralAllocatorScope
    {
    public:
        AZ_COMPONENT(LmbrCentralAllocatorComponent, "{B0512A75-AC4A-423A-BB55-C3355C0B186A}", AZ::Component);

        LmbrCentralAllocatorComponent() = default;
        ~LmbrCentralAllocatorComponent() override = default;

        void Activate() override
        {
            LmbrCentralAllocatorScope::ActivateAllocators();
        }

        void Deactivate() override
        {
            LmbrCentralAllocatorScope::DeactivateAllocators();
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MemoryAllocators", 0xd59acbcc));
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<LmbrCentralAllocatorComponent, AZ::Component>()->Version(1);

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<LmbrCentralAllocatorComponent>(
                        "LmbrCentral Allocator Component", "Manages initialization of memory allocators required by LmbrCentral")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ;
                }
            }
        }
    };

    // This component is opted in to AssetBuilders
    class LmbrCentralAssetBuilderAllocatorComponent
        : public LmbrCentralAllocatorComponent
    {
    public:
        AZ_COMPONENT(LmbrCentralAssetBuilderAllocatorComponent, "{030B63DE-7DC1-4E08-9AAF-1D089D3D0C46}", LmbrCentralAllocatorComponent);

        LmbrCentralAssetBuilderAllocatorComponent() = default;
        ~LmbrCentralAssetBuilderAllocatorComponent() override = default;

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("MemoryAllocators", 0xd59acbcc));
        }

        static void Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<LmbrCentralAssetBuilderAllocatorComponent, LmbrCentralAllocatorComponent>()->Version(1)
                    ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC("AssetBuilder", 0xc739c7d7) }));

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<LmbrCentralAssetBuilderAllocatorComponent>(
                        "LmbrCentral Asset Builder Allocator Component", "Manages initialization of memory allocators required by LmbrCentral during asset building")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Category, "Engine")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ;
                }
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////
    // LmbrCentral::LmbrCentralModule
    ////////////////////////////////////////////////////////////////////////////

    //! Create ComponentDescriptors and add them to the list.
    //! The descriptors will be registered at the appropriate time.
    //! The descriptors will be destroyed (and thus unregistered) at the appropriate time.
    LmbrCentralModule::LmbrCentralModule()
        : AZ::Module()
    {
        m_descriptors.insert(m_descriptors.end(), {
            AudioAreaEnvironmentComponent::CreateDescriptor(),
            AudioEnvironmentComponent::CreateDescriptor(),
            AudioListenerComponent::CreateDescriptor(),
            AudioMultiPositionComponent::CreateDescriptor(),
            AudioPreloadComponent::CreateDescriptor(),
            AudioProxyComponent::CreateDescriptor(),
            AudioRtpcComponent::CreateDescriptor(),
            AudioSwitchComponent::CreateDescriptor(),
            AudioSystemComponent::CreateDescriptor(),
            AudioTriggerComponent::CreateDescriptor(),
            BundlingSystemComponent::CreateDescriptor(),
            LmbrCentralAllocatorComponent::CreateDescriptor(),
            LmbrCentralAssetBuilderAllocatorComponent::CreateDescriptor(),
            LmbrCentralSystemComponent::CreateDescriptor(),
            NavigationComponent::CreateDescriptor(),
            SimpleStateComponent::CreateDescriptor(),
            SpawnerComponent::CreateDescriptor(),
            LookAtComponent::CreateDescriptor(),
            TagComponent::CreateDescriptor(),
            SphereShapeComponent::CreateDescriptor(),
            DiskShapeComponent::CreateDescriptor(),
            BoxShapeComponent::CreateDescriptor(),
            AxisAlignedBoxShapeComponent::CreateDescriptor(),
            QuadShapeComponent::CreateDescriptor(),
            CylinderShapeComponent::CreateDescriptor(),
            CapsuleShapeComponent::CreateDescriptor(),
            TubeShapeComponent::CreateDescriptor(),
            CompoundShapeComponent::CreateDescriptor(),
            SplineComponent::CreateDescriptor(),
            PolygonPrismShapeComponent::CreateDescriptor(),
            NavigationSystemComponent::CreateDescriptor(),
            GeometrySystemComponent::CreateDescriptor(),
            RandomTimedSpawnerComponent::CreateDescriptor(),
            SphereShapeDebugDisplayComponent::CreateDescriptor(),
            DiskShapeDebugDisplayComponent::CreateDescriptor(),
            BoxShapeDebugDisplayComponent::CreateDescriptor(),
            AxisAlignedBoxShapeDebugDisplayComponent::CreateDescriptor(),
            QuadShapeDebugDisplayComponent::CreateDescriptor(),
            CapsuleShapeDebugDisplayComponent::CreateDescriptor(),
            CylinderShapeDebugDisplayComponent::CreateDescriptor(),
            PolygonPrismShapeDebugDisplayComponent::CreateDescriptor(),
            TubeShapeDebugDisplayComponent::CreateDescriptor(),
            AssetSystemDebugComponent::CreateDescriptor(),
#if AZ_LOADSCREENCOMPONENT_ENABLED
            LoadScreenComponent::CreateDescriptor(),
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
            });

        // This is an internal Amazon gem, so register it's components for metrics tracking, otherwise the name of the component won't get sent back.
        // IF YOU ARE A THIRDPARTY WRITING A GEM, DO NOT REGISTER YOUR COMPONENTS WITH EditorMetricsComponentRegistrationBus
        AZStd::vector<AZ::Uuid> typeIds;
        typeIds.reserve(m_descriptors.size());
        for (AZ::ComponentDescriptor* descriptor : m_descriptors)
        {
            typeIds.emplace_back(descriptor->GetUuid());
        }
        EBUS_EVENT(AzFramework::MetricsPlainTextNameRegistrationBus, RegisterForNameSending, typeIds);
    }

    //! Request system components on the system entity.
    //! These components' memory is owned by the system entity.
    AZ::ComponentTypeList LmbrCentralModule::GetRequiredSystemComponents() const
    {
        return {
                   azrtti_typeid<LmbrCentralAllocatorComponent>(),
                   azrtti_typeid<LmbrCentralAssetBuilderAllocatorComponent>(),
                   azrtti_typeid<LmbrCentralSystemComponent>(),
                   azrtti_typeid<NavigationSystemComponent>(),
                   azrtti_typeid<GeometrySystemComponent>(),
                   azrtti_typeid<AudioSystemComponent>(),
                   azrtti_typeid<BundlingSystemComponent>(),
                   azrtti_typeid<AssetSystemDebugComponent>(),
#if AZ_LOADSCREENCOMPONENT_ENABLED
                   azrtti_typeid<LoadScreenComponent>(),
#endif // if AZ_LOADSCREENCOMPONENT_ENABLED
        };
    }

    ////////////////////////////////////////////////////////////////////////////
    // LmbrCentralSystemComponent

    void LmbrCentralSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Allow loading of Material and Texture Assets which explicitly specialized AzTypeInfo instead of using
            // the AZ_RTTI Uuid from SimpleAssetReference<T>
            serializeContext->ClassDeprecate("SimpleAssetReference_MaterialAsset", "{B7B8ECC7-FF89-4A76-A50E-4C6CA2B6E6B4}",
                [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
                {
                    AZStd::vector<AZ::SerializeContext::DataElementNode> childNodeElements;
                    for (int index = 0; index < rootElement.GetNumSubElements(); ++index)
                    {
                        childNodeElements.push_back(rootElement.GetSubElement(index));
                    }
                    // Convert the rootElement now, the existing child DataElmentNodes are now removed
                    rootElement.Convert<AzFramework::SimpleAssetReference<MaterialAsset>>(context);
                    for (AZ::SerializeContext::DataElementNode& childNodeElement : childNodeElements)
                    {
                        rootElement.AddElement(AZStd::move(childNodeElement));
                    }
                    return true;
                });
            serializeContext->ClassDeprecate("SimpleAssetReference_TextureAsset", "{68E92460-5C0C-4031-9620-6F1A08763243}",
                [](AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootElement)
                {
                    AZStd::vector<AZ::SerializeContext::DataElementNode> childNodeElements;
                    for (int index = 0; index < rootElement.GetNumSubElements(); ++index)
                    {
                        childNodeElements.push_back(rootElement.GetSubElement(index));
                    }
                    // Convert the rootElement now, the existing child DataElmentNodes are now removed
                    rootElement.Convert<AzFramework::SimpleAssetReference<TextureAsset>>(context);
                    for (AZ::SerializeContext::DataElementNode& childNodeElement : childNodeElements)
                    {
                        rootElement.AddElement(AZStd::move(childNodeElement));
                    }
                    return true;
                });
            AzFramework::SimpleAssetReference<MaterialAsset>::Register(*serializeContext);
            AzFramework::SimpleAssetReference<TextureAsset>::Register(*serializeContext);

            serializeContext->Class<LmbrCentralSystemComponent, AZ::Component>()
                ->Version(1)
            ;

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<LmbrCentralSystemComponent>(
                    "LmbrCentral", "Coordinates initialization of systems within LmbrCentral")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Game")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                ;
            }
        }

        ReflectScriptableEvents::Reflect(context);
    }

    void LmbrCentralSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("LmbrCentralService", 0xc3a02410));
    }

    void LmbrCentralSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("LmbrCentralService", 0xc3a02410));
    }

    void LmbrCentralSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("AssetDatabaseService", 0x3abf5601));
    }

    void LmbrCentralSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(AZ_CRC("MemoryAllocators", 0xd59acbcc));
        dependent.push_back(AZ_CRC("AssetCatalogService", 0xc68ffc57));
    }

    void LmbrCentralSystemComponent::Activate()
    {
        if (!AZ::AllocatorInstance<AZ::LegacyAllocator>::IsReady())
        {
            AZ::AllocatorInstance<AZ::LegacyAllocator>::Create();
            m_allocatorShutdowns.push_back([]() { AZ::AllocatorInstance<AZ::LegacyAllocator>::Destroy(); });
        }

        // Register asset handlers. Requires "AssetDatabaseService"
        AZ_Assert(AZ::Data::AssetManager::IsReady(), "Asset manager isn't ready!");

        // Add asset types and extensions to AssetCatalog. Uses "AssetCatalogService".
        if (auto assetCatalog = AZ::Data::AssetCatalogRequestBus::FindFirstHandler(); assetCatalog)
        {
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<AZ::ScriptAsset>::Uuid());
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<MaterialAsset>::Uuid());
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<DccMaterialAsset>::Uuid());
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<MeshAsset>::Uuid());
            assetCatalog->EnableCatalogForAsset(AZ::AzTypeInfo<CharacterDefinitionAsset>::Uuid());

            assetCatalog->AddExtension("cgf");
            assetCatalog->AddExtension("chr");
            assetCatalog->AddExtension("cdf");
            assetCatalog->AddExtension("dds");
            assetCatalog->AddExtension("caf");
            assetCatalog->AddExtension("xml");
            assetCatalog->AddExtension("mtl");
            assetCatalog->AddExtension("dccmtl");
            assetCatalog->AddExtension("lua");
            assetCatalog->AddExtension("sprite");
            assetCatalog->AddExtension("cax");
        }

        AZ::Data::AssetManagerNotificationBus::Handler::BusConnect();


        // Register unhandled asset type info
        // Material
        auto materialAssetTypeInfo = aznew MaterialAssetTypeInfo();
        materialAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(materialAssetTypeInfo);
        // DCC Material
        auto dccMaterialAssetTypeInfo = aznew DccMaterialAssetTypeInfo();
        dccMaterialAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(dccMaterialAssetTypeInfo);
        // Other
        auto audioAssetTypeInfo = aznew AudioAssetTypeInfo();
        audioAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(audioAssetTypeInfo);

        auto characterPhysicsAssetTypeInfo = aznew CharacterPhysicsAssetTypeInfo();
        characterPhysicsAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(characterPhysicsAssetTypeInfo);

        auto groupAssetTypeInfo = aznew GroupAssetTypeInfo();
        groupAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(groupAssetTypeInfo);

        auto prefabsLibraryAssetTypeInfo = aznew PrefabsLibraryAssetTypeInfo();
        prefabsLibraryAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(prefabsLibraryAssetTypeInfo);

        auto entityPrototypeAssetTypeInfo = aznew EntityPrototypeLibraryAssetTypeInfo();
        entityPrototypeAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(entityPrototypeAssetTypeInfo);

        auto gameTokenAssetTypeInfo = aznew GameTokenAssetTypeInfo();
        gameTokenAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(gameTokenAssetTypeInfo);

        // Texture
        auto substanceAssetTypeInfo = aznew SubstanceAssetTypeInfo();
        substanceAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(substanceAssetTypeInfo);

        auto textureAssetTypeInfo = aznew TextureAssetTypeInfo();
        textureAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(textureAssetTypeInfo);

        // Hidden
        auto textureMipmapAssetTypeInfo = aznew TextureMipmapAssetTypeInfo();
        textureMipmapAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(textureMipmapAssetTypeInfo);

        // UI
        auto fontAssetTypeInfo = aznew FontAssetTypeInfo();
        fontAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(fontAssetTypeInfo);

        auto uiCanvasAssetTypeInfo = aznew UICanvasAssetTypeInfo();
        uiCanvasAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(uiCanvasAssetTypeInfo);

        auto entityIconAssetTypeInfo = aznew EntityIconAssetTypeInfo();
        entityIconAssetTypeInfo->Register();
        m_unhandledAssetInfo.emplace_back(entityIconAssetTypeInfo);
    }

    void LmbrCentralSystemComponent::Deactivate()
    {
        // AssetTypeInfo's destructor calls Unregister()
        m_unhandledAssetInfo.clear();

        AZ::Data::AssetManagerNotificationBus::Handler::BusDisconnect();

        // AssetHandler's destructor calls Unregister()
        m_assetHandlers.clear();

        for (auto allocatorIt = m_allocatorShutdowns.rbegin(); allocatorIt != m_allocatorShutdowns.rend(); ++allocatorIt)
        {
            (*allocatorIt)();
        }
        m_allocatorShutdowns.clear();
    }
} // namespace LmbrCentral

#if !defined(LMBR_CENTRAL_EDITOR)
AZ_DECLARE_MODULE_CLASS(Gem_LmbrCentral, LmbrCentral::LmbrCentralModule)
#endif
