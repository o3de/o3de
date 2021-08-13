/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>
#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>

namespace ShaderManagementConsole
{
    /**
     * ShaderManagementConsoleDocument provides an API for modifying and saving document properties.
     */
    class ShaderManagementConsoleDocument
        : public AtomToolsFramework::AtomToolsDocument
        , public ShaderManagementConsoleDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(ShaderManagementConsoleDocument, "{DBA269AE-892B-415C-8FA1-166B94B0E045}");
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY(ShaderManagementConsoleDocument);

        ShaderManagementConsoleDocument();
        virtual ~ShaderManagementConsoleDocument();

        ////////////////////////////////////////////////////////////////////////
        // AtomToolsFramework::AtomToolsDocument
        ////////////////////////////////////////////////////////////////////////
        bool Open(AZStd::string_view loadPath) override;
        bool Close() override;
        bool IsOpen() const override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // ShaderManagementConsoleDocumentRequestBus::Handler implementation
        size_t GetShaderOptionCount() const override;
        const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const override;
        size_t GetShaderVariantCount() const override;
        const AZ::RPI::ShaderVariantListSourceData::VariantInfo& GetShaderVariantInfo(size_t index) const override;
        ////////////////////////////////////////////////////////////////////////

    private:
        // Function to be bound for undo and redo
        using UndoRedoFunction = AZStd::function<void()>;

        // A pair of functions, where first is the undo operation and second is the redo operation
        using UndoRedoFunctionPair = AZStd::pair<UndoRedoFunction, UndoRedoFunction>;

        // Container for all of the active undo and redo functions and state
        using UndoRedoHistory = AZStd::vector<UndoRedoFunctionPair>;

        void Clear();

        // Source data for shader variant list
        AZ::RPI::ShaderVariantListSourceData m_shaderVariantListSourceData;

        // Shader asset for the corresponding shader variant list
        AZ::Data::Asset<AZ::RPI::ShaderAsset> m_shaderAsset;
    };
} // namespace ShaderManagementConsole
