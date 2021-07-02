/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <ScriptCanvas/Asset/AssetDescription.h>
#include <ScriptCanvas/Assets/ScriptCanvasAsset.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetHandler.h>

namespace AZ
{
    class SerializeContext;
}

namespace ScriptCanvasEditor
{
    class ScriptCanvasFunctionAsset;

    /**
    * Manages editor Script Canvas graph assets.
    */
    class ScriptCanvasFunctionAssetHandler
        : public ScriptCanvasAssetHandler
    {
    public:
        AZ_CLASS_ALLOCATOR(ScriptCanvasFunctionAssetHandler, AZ::SystemAllocator, 0);
        AZ_RTTI(ScriptCanvasFunctionAssetHandler, "{CE1EB0B7-D8DA-4B9B-858B-A34DF5092BC2}", ScriptCanvasAssetHandler);

        ScriptCanvasFunctionAssetHandler(AZ::SerializeContext* context = nullptr);
        ~ScriptCanvasFunctionAssetHandler();

        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        AZ::Data::AssetHandler::LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset,
            AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        bool SaveAssetData(const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZ::IO::GenericStream* stream) override;

        bool SaveAssetData(const ScriptCanvasEditor::ScriptCanvasFunctionAsset* assetData, AZ::IO::GenericStream* stream);

        bool SaveAssetData(const ScriptCanvasEditor::ScriptCanvasFunctionAsset* assetData, AZ::IO::GenericStream* stream, AZ::DataStream::StreamType streamType);

        // Called by asset database on registration.
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

        // Provides editor with information about script canvas graph assets.
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;

        bool CanCreateComponent(const AZ::Data::AssetId& assetId) const override;

        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;

        static AZ::Data::AssetType GetAssetTypeStatic();

        ScriptCanvasEditor::ScriptCanvasFunctionAsset* m_scriptCanvasAsset;

    };
}
