/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ScriptEventsSystemComponent.h"

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <ScriptEvents/ScriptEventsAsset.h>
#include <AzToolsFramework/AssetEditor/AssetEditorBus.h>
#include <ScriptEvents/ScriptEventsAssetRef.h>

namespace AzToolsFramework { class PropertyHandlerBase; }

#if defined(SCRIPTEVENTS_EDITOR)

namespace ScriptEventsEditor
{

    // This is the ScriptEvent asset handler used by the Asset Editor, it does additional validation that is not
    // needed when saving the asset through the builder
    class ScriptEventAssetHandler 
        : public AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>
        , AzToolsFramework::AssetEditor::AssetEditorValidationRequestBus::MultiHandler
    {
    public:
        AZ_RTTI(ScriptEventAssetHandler, "{D81DE7D5-5ED0-4D70-8364-AA986E9C490E}", AzFramework::GenericAssetHandler<ScriptEvents::ScriptEventsAsset>);

        ScriptEventAssetHandler(const char* displayName, const char* group, const char* extension, const AZ::Uuid& componentTypeId = AZ::Uuid::CreateNull(), AZ::SerializeContext* serializeContext = nullptr);

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;

        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;

        void InitAsset(const AZ::Data::Asset<AZ::Data::AssetData>& asset, bool loadStageSucceeded, bool isReload) override;

        // AssetEditorValidationRequestBus::Handler
        AZ::Outcome<bool, AZStd::string> IsAssetDataValid(const AZ::Data::Asset<AZ::Data::AssetData>& asset) override;
        void PreAssetSave(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
        void BeforePropertyEdit(AzToolsFramework::InstanceDataNode* node, AZ::Data::Asset<AZ::Data::AssetData> asset) override;

        void SetSaveAsBinary(bool saveAsBinary) { m_saveAsBinary = saveAsBinary; }

    private:

        struct PreviousNameSettings
        {
            AZStd::string   m_previousName;
            AZ::u32         m_version;
        };

        AZStd::unordered_map< AZ::Data::AssetId, PreviousNameSettings > m_previousEbusNames;

        bool m_saveAsBinary;
    };

    class ScriptEventEditorSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(ScriptEventEditorSystemComponent, "{8BAD5292-56C3-4657-99F2-515A2BDE23C1}");

        ScriptEventEditorSystemComponent() = default;
        ScriptEventEditorSystemComponent(const ScriptEventEditorSystemComponent&) = delete;

    protected:

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component...
        void Init() override {}
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        // Script Event Assets
        AZStd::unordered_map<ScriptEvents::ScriptEventKey, AZStd::intrusive_ptr<ScriptEvents::Internal::ScriptEventRegistration>> m_scriptEvents;

    };
}

#endif
