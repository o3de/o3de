/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <ScriptEvents/ScriptEventsGem.h>
#include <Source/Editor/ScriptEventsSystemEditorComponent.h>

#include <ScriptEvents/Components/ScriptEventReferencesComponent.h>
#include <Builder/ScriptEventsBuilderComponent.h>
#include <ScriptEvents/ScriptEventsBus.h>

#include <AzCore/Asset/AssetSerializer.h>
#if defined(SCRIPTEVENTS_EDITOR)

namespace ScriptEvents
{

    class ScriptEventsSystemComponentEditorImpl
        : public ScriptEventsSystemComponentImpl
    {
    public:

        ~ScriptEventsSystemComponentEditorImpl() override
        {
        }

        void RegisterAssetHandler() override
        {
            AZ::Data::AssetType assetType(azrtti_typeid<ScriptEvents::ScriptEventsAsset>());
            if (AZ::Data::AssetManager::Instance().GetHandler(assetType))
            {
                return; // Asset Type already handled
            }

            m_assetHandler = AZStd::make_unique<ScriptEventsEditor::ScriptEventAssetHandler>(
                ScriptEvents::ScriptEventsAsset::GetDisplayName(),
                ScriptEvents::ScriptEventsAsset::GetGroup(),
                ScriptEvents::ScriptEventsAsset::GetFileFilter(),
                AZ::AzTypeInfo<ScriptEventsEditor::ScriptEventEditorSystemComponent>::Uuid());
            AZ::Data::AssetManager::Instance().RegisterHandler(m_assetHandler.get(), assetType);

            // Use AssetCatalog service to register ScriptEvent asset type and extension
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddAssetType, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::EnableCatalogForAsset, assetType);
            AZ::Data::AssetCatalogRequestBus::Broadcast(&AZ::Data::AssetCatalogRequests::AddExtension,
                ScriptEvents::ScriptEventsAsset::GetFileFilter());
        }

        void UnregisterAssetHandler() override
        {
            if (m_assetHandler)
            {
                AZ::Data::AssetManager::Instance().UnregisterHandler(m_assetHandler.get());
                m_assetHandler.reset();
            }
        }

        AZStd::unique_ptr<AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>> m_assetHandler;

    };




    ScriptEventsModule::ScriptEventsModule()
        : AZ::Module()
        , m_systemImpl(nullptr)
    {
        ScriptEvents::ScriptEventModuleConfigurationRequestBus::Handler::BusConnect();

        m_descriptors.insert(m_descriptors.end(), {
            ScriptEventsEditor::ScriptEventEditorSystemComponent::CreateDescriptor(),
            ScriptEvents::Components::ScriptEventReferencesComponent::CreateDescriptor(),
            ScriptEventsBuilder::ScriptEventsBuilderComponent::CreateDescriptor(),
        });
    }

    ScriptEventsSystemComponentImpl* ScriptEventsModule::GetSystemComponentImpl()
    {
        if (!m_systemImpl)
        {
            m_systemImpl = aznew ScriptEventsSystemComponentEditorImpl();
        }

        return m_systemImpl;
    }


    /**
    * Add required SystemComponents to the SystemEntity.
    */
    AZ::ComponentTypeList ScriptEventsModule::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<ScriptEventsEditor::ScriptEventEditorSystemComponent >(),
        };
    }
}

AZ_DECLARE_MODULE_CLASS(Gem_ScriptEvents, ScriptEvents::ScriptEventsModule)

#endif
