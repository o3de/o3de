/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Reflect/Material/ShaderCollection.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <Document/ShaderManagementConsoleDocumentRequestBus.h>

namespace ShaderManagementConsole
{
    //! ShaderManagementConsoleDocument provides an API for modifying and saving document properties.
    class ShaderManagementConsoleDocument
        : public AtomToolsFramework::AtomToolsDocument
        , public ShaderManagementConsoleDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(ShaderManagementConsoleDocument, "{504A74BA-F5DD-49E0-BA5E-A381F61DD524}");
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY_MOVE(ShaderManagementConsoleDocument);

        ShaderManagementConsoleDocument(const AZ::Crc32& toolId);
        ~ShaderManagementConsoleDocument();

        // AtomToolsFramework::AtomToolsDocument overrides...
        AZStd::vector<AtomToolsFramework::DocumentObjectInfo> GetObjectInfo() const override;
        bool Open(AZStd::string_view loadPath) override;
        bool Save() override;
        bool SaveAsCopy(AZStd::string_view savePath) override;
        bool SaveAsChild(AZStd::string_view savePath) override;
        bool IsOpen() const override;
        bool IsModified() const override;
        bool IsSavable() const override;

        // ShaderManagementConsoleDocumentRequestBus::Handler overridfes...
        void SetShaderVariantListSourceData(const AZ::RPI::ShaderVariantListSourceData& shaderVariantListSourceData) override;
        const AZ::RPI::ShaderVariantListSourceData& GetShaderVariantListSourceData() const override;
        size_t GetShaderVariantCount() const override;
        const AZ::RPI::ShaderVariantListSourceData::VariantInfo& GetShaderVariantInfo(size_t index) const override;
        size_t GetShaderOptionCount() const override;
        const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const override;

    private:
        // AtomToolsFramework::AtomToolsDocument overrides...
        void Clear() override;

        // Write shader variant list source data to JSON
        bool SaveSourceData();

        // Read shader variant list source data from JSON and initialize the document
        bool LoadShaderSourceData();

        // Read shader source data from JSON then find all references to to populate the shader variant list and initialize the document
        bool LoadShaderVariantListSourceData();

        // Find all material assets that reference material types using shaderFilePath
        AZStd::vector<AZ::Data::AssetId> FindMaterialAssetsUsingShader(const AZStd::string& shaderFilePath);

        // Retrieve all of the shader collection items from a material instance created from materialAssetId
        AZStd::vector<AZ::RPI::ShaderCollection::Item> GetMaterialInstanceShaderItems(const AZ::Data::AssetId& materialAssetId);

        // Source data for shader variant list
        AZ::RPI::ShaderVariantListSourceData m_shaderVariantListSourceData;

        // Shader asset for the corresponding shader variant list
        AZ::Data::Asset<AZ::RPI::ShaderAsset> m_shaderAsset;

        AZ::RPI::ShaderOptionDescriptor m_invalidDescriptor;
    };
} // namespace ShaderManagementConsole
