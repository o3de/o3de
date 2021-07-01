/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Asset/BlastSliceAsset.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>

namespace Blast
{
    //! Used to create store asset references (i.e. ids) to fill out the EditorBlastMeshDataComponent
    class BlastSliceAssetStorageComponent final : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_COMPONENT(
            BlastSliceAssetStorageComponent, "{696C7E62-1EA4-41E2-B4F6-7BD0D30888DC}",
            AzToolsFramework::Components::EditorComponentBase);

        ~BlastSliceAssetStorageComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        const AZStd::vector<AZ::Data::AssetId>& GetMeshData() const
        {
            return m_meshAssetIdList;
        }

        void SetMeshData(const AZStd::vector<AZ::Data::AssetId>& meshAssetIdList)
        {
            m_meshAssetIdList = meshAssetIdList;
        }

        const AZStd::vector<AZStd::string>& GetMeshPathList() const
        {
            return m_meshAssetPathList;
        }

        void SetMeshPathList(const AZStd::vector<AZStd::string>& meshAssetPathList)
        {
            m_meshAssetPathList = meshAssetPathList;
        }

    private:
        // AZ::Component interface implementation
        void Activate() override {}
        void Deactivate() override {}

        // EditorComponentBase
        void BuildGameEntity([[maybe_unused]] AZ::Entity* gameEntity) override {}

        // Script API
        bool GenerateAssetInfo(
            const AZStd::vector<AZStd::string>& chunkNames,
            AZStd::string_view blastFilename,
            AZStd::string_view assetinfoFilename);

        bool WriteMaterialFile(
            AZStd::string_view materialGroupName,
            const AZStd::vector<AZStd::string>& materialNames,
            AZStd::string_view materialFilename);

        AZStd::vector<AZ::Data::AssetId> m_meshAssetIdList;
        AZStd::vector<AZStd::string> m_meshAssetPathList;
    };

    class EditorBlastSliceAssetHandler final
        : public AZ::Data::AssetHandler
        , public AZ::AssetTypeInfoBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(EditorBlastSliceAssetHandler, AZ::SystemAllocator, 0);

        ~EditorBlastSliceAssetHandler() override;

        // AZ::Data::AssetHandler
        AZ::Data::AssetPtr CreateAsset(const AZ::Data::AssetId& id, const AZ::Data::AssetType& type) override;
        LoadResult LoadAssetData(
            const AZ::Data::Asset<AZ::Data::AssetData>& asset, AZStd::shared_ptr<AZ::Data::AssetDataStream> stream,
            const AZ::Data::AssetFilterCB& assetLoadFilterCB) override;
        void DestroyAsset(AZ::Data::AssetPtr ptr) override;
        void GetHandledAssetTypes(AZStd::vector<AZ::Data::AssetType>& assetTypes) override;

        // AZ::AssetTypeInfoBus::Handler
        AZ::Data::AssetType GetAssetType() const override;
        const char* GetAssetTypeDisplayName() const override;
        const char* GetGroup() const override;
        const char* GetBrowserIcon() const override;
        void GetAssetTypeExtensions(AZStd::vector<AZStd::string>& extensions) override;

        void Register();
        void Unregister();
    };
} // namespace Blast
