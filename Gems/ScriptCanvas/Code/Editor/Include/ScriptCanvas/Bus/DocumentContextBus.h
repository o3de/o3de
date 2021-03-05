/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once
#include <AzCore/EBus/EBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/string/string_view.h>

#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>

namespace AZ
{
    namespace Data
    {
        class AssetInfo;
    }
}

namespace ScriptCanvasEditor
{
    enum class ScriptCanvasFileState : AZ::s32
    {
        NEW,
        MODIFIED,
        UNMODIFIED,
        INVALID = -1
    };

    struct ScriptCanvasAssetFileInfo
    {
        AZ_TYPE_INFO(ScriptCanvasAssetFileInfo, "{81F6B390-7CF3-4A97-B5A6-EC09330F184E}");
        AZ_CLASS_ALLOCATOR(ScriptCanvasAssetFileInfo, AZ::SystemAllocator, 0);
        ScriptCanvasFileState m_fileModificationState = ScriptCanvasFileState::INVALID;
        bool m_reloadable = false;
        AZStd::string m_absolutePath;
    };

    //! Bus for handling transactions involving ScriptCanvas Assets, such as graph Saving, graph modification state, etc
    class DocumentContextRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Creates a new ScriptCanvas asset and registers it with the document context
        virtual AZ::Data::Asset<ScriptCanvasAsset> CreateScriptCanvasAsset(AZStd::string_view relativeAssetPath) = 0;

        using SaveCB = AZStd::function<void(bool saveSuccess)>;
        // !Callback will fire when a new in-memory Script Canvas asset is saved to disk for the first time and a new AssetId is generated for it
        using SourceFileChangedCB = AZStd::function<void(AZStd::string relPath, AZStd::string scanFolder, const AZ::Data::AssetId& newAssetId)>;
        //! Saves a ScriptCanvas asset using the supplied AssetPath structure
        //! \param assetAbsolutePath path where ScriptCanvas asset will be saved on disk
        virtual void SaveScriptCanvasAsset(AZStd::string_view/*assetAbsolutePath*/, AZ::Data::Asset<ScriptCanvasAsset>, const SaveCB&, const SourceFileChangedCB&) = 0;

        //! Loads a ScriptCanvas asset by looking up the assetPath in the AssetCatalog
        //! \param assetPath filepath to asset that will be looked up in the AssetCatalog for the AssetId
        //! \param loadBlocking should the loading of the ScriptCanvas asset be blocking
        virtual AZ::Data::Asset<ScriptCanvasAsset> LoadScriptCanvasAsset(const char* assetPath, bool loadBlocking) = 0;
        virtual AZ::Data::Asset<ScriptCanvasAsset> LoadScriptCanvasAssetById(const AZ::Data::AssetId& assetId, bool loadBlocking) = 0;

        //! Registers a ScriptCanvas assetId with the DocumentContext for lookup. ScriptCanvasAssetFileInfo will be associated with the ScriptCanvasAsset
        //! \return true if the assetId is newly registered, false if the assetId is already registered
        virtual bool RegisterScriptCanvasAsset(const AZ::Data::AssetId& assetId, const ScriptCanvasAssetFileInfo& assetFileInfo) = 0;

        //! Unregisters a ScriptCanvas assetId with the DocumentContext for lookup
        //! \return true if assetId was registered with the DocumentContext, false otherwise
        virtual bool UnregisterScriptCanvasAsset(const AZ::Data::AssetId& assetId) = 0;

        virtual ScriptCanvasFileState GetScriptCanvasAssetModificationState(const AZ::Data::AssetId& assetId) = 0;
        virtual void SetScriptCanvasAssetModificationState(const AZ::Data::AssetId& assetId, ScriptCanvasFileState) = 0;

        //! Retrieves the file information for the registered ScriptCanvas asset
        virtual AZ::Outcome<ScriptCanvasAssetFileInfo, AZStd::string> GetFileInfo(const AZ::Data::AssetId& assetId) const = 0;
        virtual AZ::Outcome<void, AZStd::string> SetFileInfo(const AZ::Data::AssetId& assetId, const ScriptCanvasAssetFileInfo& fileInfo) = 0;
    };

    using DocumentContextRequestBus = AZ::EBus<DocumentContextRequests>;

    class DocumentContextNotifications
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        using BusIdType = AZ::Data::AssetId;

        virtual void OnAssetModificationStateChanged(ScriptCanvasFileState) {}

        //! Notification which fires after an ScriptCanvasDocumentContext has received it's on AssetReady callback
        //! \param scriptCanvasAsset Script Canvas asset which is now ready for use in the Editor
        virtual void OnScriptCanvasAssetReady(const AZ::Data::Asset<ScriptCanvasAsset>& /*scriptCanvasAsset*/) {}

        //! Notification which fires after an ScriptCanvasDocumentContext has received it's on AssetReloaded callback
        //! \param scriptCanvasAsset Script Canvas asset which is now ready for use in the Editor
        virtual void OnScriptCanvasAssetReloaded(const AZ::Data::Asset<ScriptCanvasAsset>& /*scriptCanvaAsset */) {}

        //! Notification which fires after an ScriptCanvasDocumentContext has received it's on AssetReady callback
        //! \param AssetId AssetId of unloaded ScriptCanvas
        virtual void OnScriptCanvasAssetUnloaded(const AZ::Data::AssetId& /*assetId*/) {}
    };

    using DocumentContextNotificationBus = AZ::EBus<DocumentContextNotifications>;
}
