/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>

#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

#include <Editor/Assets/ScriptCanvasAssetTrackerDefinitions.h>
#include <Editor/Assets/ScriptCanvasMemoryAsset.h>
#include <ScriptCanvas/Bus/RequestBus.h>

class QWidget;

namespace AZ 
{ 
    class EntityId; 
}

namespace ScriptCanvas
{
    class ScriptCanvasAssetBase;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasAssetHandler;
    
    class AssetTrackerRequests
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Callback used to know when a save operation failed or succeeded
        using OnSave = AZStd::function<void(bool saveSuccess, AZ::Data::Asset<ScriptCanvas::ScriptCanvasAssetBase>)>;

        //! Callback used when the asset has been loaded and is ready for use
        using OnAssetReadyCallback = AZStd::function<void(ScriptCanvasMemoryAsset&)>;

        //! Callback used when the asset is newly created
        using OnAssetCreatedCallback = OnAssetReadyCallback;

        // Operations

        //! Creates a new Script Canvas asset and tracks it
        virtual AZ::Data::AssetId Create([[maybe_unused]] AZStd::string_view assetAbsolutePath, [[maybe_unused]] AZ::Data::AssetType assetType, Callbacks::OnAssetCreatedCallback onAssetCreatedCallback) { return AZ::Data::AssetId(); }

        //! Saves a Script Canvas asset to a new file, once the save is complete it will use the source Id (not the Id of the in-memory asset)
        virtual void SaveAs([[maybe_unused]] AZ::Data::AssetId assetId, [[maybe_unused]] const AZStd::string& path, Callbacks::OnSave onSaveCallback) {}

        //! Saves a previously loaded Script Canvas asset to file
        virtual void Save([[maybe_unused]] AZ::Data::AssetId assetId, Callbacks::OnSave onSaveCallback) {}

        //! Returns whether or not the specified asset is currently saving
        virtual bool IsSaving([[maybe_unused]] AZ::Data::AssetId assetId) const { return false; }

        //! Loads a Script Canvas graph
        virtual bool Load([[maybe_unused]] AZ::Data::AssetId assetId, [[maybe_unused]] AZ::Data::AssetType assetType, Callbacks::OnAssetReadyCallback onAssetReadyCallback) { return false; }

        //! Closes and unloads a Script Canvas graph from the tracker
        virtual void Close([[maybe_unused]] AZ::Data::AssetId assetId) {}

        //! Creates the asset's view
        virtual void CreateView([[maybe_unused]] AZ::Data::AssetId assetId, [[maybe_unused]] QWidget* parent) {}

        //! Releases the asset's view
        virtual void ClearView([[maybe_unused]] AZ::Data::AssetId assetId) {}

        //! Used to make sure assets that are unloaded also get removed from tracking
        virtual void UntrackAsset([[maybe_unused]] AZ::Data::AssetId assetId) {}

        //! Recreates the view for all tracked assets
        virtual void RefreshAll() {}

        using AssetList = AZStd::vector<ScriptCanvasMemoryAsset::pointer>;

        // Accessors
        virtual ScriptCanvasMemoryAsset::pointer GetAsset([[maybe_unused]] AZ::Data::AssetId assetId) { return nullptr; }
        virtual ScriptCanvas::ScriptCanvasId GetScriptCanvasId([[maybe_unused]] AZ::Data::AssetId assetId) { return AZ::EntityId(); }
        virtual ScriptCanvas::ScriptCanvasId GetScriptCanvasIdFromGraphId([[maybe_unused]] AZ::EntityId graphId) { return AZ::EntityId(); }
        virtual ScriptCanvas::ScriptCanvasId GetGraphCanvasId(AZ::EntityId) { return AZ::EntityId(); }
        virtual ScriptCanvas::ScriptCanvasId GetGraphId([[maybe_unused]] AZ::Data::AssetId assetId) { return ScriptCanvas::ScriptCanvasId(); }
        virtual Tracker::ScriptCanvasFileState GetFileState([[maybe_unused]] AZ::Data::AssetId assetId) { return Tracker::ScriptCanvasFileState::INVALID; }
        virtual AZ::Data::AssetId GetAssetId([[maybe_unused]] ScriptCanvas::ScriptCanvasId scriptCanvasSceneId) { return {}; }
        virtual AZ::Data::AssetType GetAssetType([[maybe_unused]] ScriptCanvas::ScriptCanvasId scriptCanvasSceneId) { return {}; }
        virtual AZStd::string GetTabName([[maybe_unused]] AZ::Data::AssetId assetId) { return {}; }
        virtual AssetList GetUnsavedAssets() { return {}; }
        virtual AssetList GetAssets() { return {}; }
        virtual AssetList GetAssetsIf(AZStd::function<bool(ScriptCanvasMemoryAsset::pointer asset)>) { return {}; }

        virtual AZ::EntityId GetSceneEntityIdFromEditorEntityId([[maybe_unused]] AZ::Data::AssetId assetId, [[maybe_unused]] AZ::EntityId editorEntityId) { return AZ::EntityId(); }
        virtual AZ::EntityId GetEditorEntityIdFromSceneEntityId([[maybe_unused]] AZ::Data::AssetId assetId, [[maybe_unused]] AZ::EntityId sceneEntityId) { return AZ::EntityId(); }

        // Setters / Updates
        virtual void UpdateFileState([[maybe_unused]] AZ::Data::AssetId assetId, [[maybe_unused]] Tracker::ScriptCanvasFileState state) {}

        // Helpers
        virtual ScriptCanvasAssetHandler* GetAssetHandlerForType([[maybe_unused]] AZ::Data::AssetType assetType) { return nullptr; }
    };

    using AssetTrackerRequestBus = AZ::EBus<AssetTrackerRequests>;

    //! These are the notifications sent by the AssetTracker only.
    //! We use these to communicate the asset status, do not use the AssetBus directly, all Script Canvas
    //! assets are managed by the AssetTracker
    class AssetTrackerNotifications
        : public AZ::EBusTraits
    {
    public:

        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Data::AssetId;

        // These will be forwarded as a result of the Asset System's events
        // this is deliberate in order to keep the AssetTracker as the only
        // place that interacts directly with the asset bus, but allowing
        // other systems to know the status of tracked assets
        virtual void OnAssetReady(const ScriptCanvasMemoryAsset::pointer asset) {}
        virtual void OnAssetReloaded(const ScriptCanvasMemoryAsset::pointer asset) {}
        virtual void OnAssetUnloaded([[maybe_unused]] const AZ::Data::AssetId assetId, [[maybe_unused]] const AZ::Data::AssetType assetType) {}
        virtual void OnAssetSaved(const ScriptCanvasMemoryAsset::pointer asset, [[maybe_unused]] bool isSuccessful) {}
        virtual void OnAssetError(const ScriptCanvasMemoryAsset::pointer asset) {}

    };
    using AssetTrackerNotificationBus = AZ::EBus<AssetTrackerNotifications>;

    namespace Internal
    {
        class MemoryAssetSystemNotifications
            : public AZ::EBusTraits
        {
        public:

            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

            virtual void OnAssetReady([[maybe_unused]] const ScriptCanvasMemoryAsset* asset) {}
            virtual void OnAssetReloaded([[maybe_unused]] const ScriptCanvasMemoryAsset* asset) {}
            virtual void OnAssetSaved([[maybe_unused]] const ScriptCanvasMemoryAsset* asset, [[maybe_unused]] bool isSuccessful) {}
            virtual void OnAssetError([[maybe_unused]] const ScriptCanvasMemoryAsset* asset) {}

        };
        using MemoryAssetSystemNotificationBus = AZ::EBus<MemoryAssetSystemNotifications>;
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
}
