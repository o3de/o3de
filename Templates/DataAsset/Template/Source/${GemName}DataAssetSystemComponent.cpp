// {BEGIN_LICENSE}
/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
// {END_LICENSE}

#include "${GemName}DataAssetSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/RTTI/BehaviorContext.h>

namespace ${GemName}
{
    /*
     * AZ_COMPONENT_IMPL provides the static type information and UUID registration for
     * this system component. The UUID must be unique across the entire project.
     */
    AZ_COMPONENT_IMPL(${GemName}DataAssetSystemComponent, "${GemName}DataAssetSystemComponent", "{${Random_Uuid}}");

    /*
     * Activate is called when the system entity is activated during engine startup.
     * This is the correct place to register asset handlers with the Asset Manager.
     *
     * GenericAssetHandler<T> is a ready-made handler from AzFramework that handles
     * loading, saving, and lifecycle management for simple serialized asset types.
     * It requires no custom builder - the asset pipeline will process files with the
     * registered extension using the JSON serializer automatically.
     *
     * Registration steps:
     *   1. Allocate the handler with aznew (it must be heap-allocated).
     *   2. Call handler->Register() to register it with the Asset Manager.
     *   3. Store it in m_assetHandlers so Deactivate() can unregister it later.
     *
     * The three arguments to GenericAssetHandler<T>() are:
     *   - Display name shown in the Editor asset browser and asset picker.
     *   - Asset group string used to categorize assets (e.g. "Data", "Animation").
     *   - File extension for source files this handler processes (e.g. "myasset").
     *
     * Uncomment and fill in the block below to register your asset type:
     */
    void ${GemName}DataAssetSystemComponent::Activate()
    {
        // Register Generic Assets
        //auto* ${AssetName}Handler = aznew AzFramework::GenericAssetHandler<${AssetName}>("${AssetName}", "${AssetGroup}", "${FileExtension}");
        //${AssetName}Handler->SetAutoBuildAssetToCache(true);
        //${AssetName}Handler->Register();
        //m_assetHandlers.emplace_back(${AssetName}Handler);
    }

    /*
     * Deactivate is called when the system entity is being shut down.
     * Every asset handler registered in Activate() must be unregistered here.
     *
     * The Asset Manager must be checked for readiness before unregistering because
     * the system may be shutting down in an order where the Asset Manager is already
     * gone. Failing to check will cause a crash on shutdown.
     *
     * m_assetHandlers is cleared after unregistration, which triggers the unique_ptr
     * destructors and frees the handler memory.
     */
    void ${GemName}DataAssetSystemComponent::Deactivate()
    {
        for (auto& assetHandler : m_assetHandlers)
        {
            if (assetHandler)
            {
                if (AZ::Data::AssetManager::IsReady())
                {
                    AZ::Data::AssetManager::Instance().UnregisterHandler(assetHandler.get());
                }
            }
        }
        m_assetHandlers.clear();
    }

    /*
     * Reflect registers this system component and all managed asset types with the
     * reflection contexts. Asset types must be reflected here (by forwarding to their
     * own Reflect() methods) so the serialization system understands their data layout
     * before any asset files are loaded.
     *
     * Uncomment the forwarding call for each asset type you register in Activate():
     *   ${AssetName}::Reflect(context);
     */
    void ${GemName}DataAssetSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect the asset types managed by this system component.
        //${AssetName}::Reflect(context);

        // Setting the SystemComponentTags enables the SetAutoBuildAssetToCache method to call during Asset Builder.
        // This is a necessary process to make the asset appear in Editor.
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<${GemName}DataAssetsSystemComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ;
        }
    }

    /*
     * GetProvidedServices declares the named service this system component provides.
     * Other components that depend on data asset availability can list this service
     * in their GetRequiredServices() or GetDependentServices() to ensure correct
     * activation ordering.
     */
    void ${GemName}DataAssetSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${GemName}DataAssetSystemComponentService"));
        provided.push_back(AzFramework::s_GenericAssetRegistrar); // Activate me before things that need these registrations.
    }

    /*
     * GetIncompatibleServices prevents a second instance of this system component
     * from being added to the system entity. System components should almost always
     * declare themselves incompatible with their own service to enforce singleton behavior.
     */
    void ${GemName}DataAssetSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${GemName}DataAssetSystemComponentService"));
    }

    /*
     * GetRequiredServices lists services that must be active before this component activates.
     * Asset-related system components typically do not have hard requirements, but if your
     * asset type depends on another system (e.g. a rendering system), declare it here.
     */
    void ${GemName}DataAssetSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    /*
     * GetDependentServices lists optional ordering dependencies. This component will
     * activate after these services if present, without failing if they are absent.
     */
    void ${GemName}DataAssetSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }
} // namespace ${GemName}
