/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Asset/AssetCommon.h>

#include <Atom/RPI.Reflect/Shader/ShaderAsset.h>
#include <Atom/RPI.Edit/Shader/ShaderVariantListSourceData.h>

#include <Atom/Document/ShaderManagementConsoleDocumentRequestBus.h>

namespace ShaderManagementConsole
{
    /**
     * ShaderManagementConsoleDocument provides an API for modifying and saving document properties.
     */
    class ShaderManagementConsoleDocument
        : public ShaderManagementConsoleDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(ShaderManagementConsoleDocument, "{DBA269AE-892B-415C-8FA1-166B94B0E045}");
        AZ_CLASS_ALLOCATOR(ShaderManagementConsoleDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY(ShaderManagementConsoleDocument);

        ShaderManagementConsoleDocument();
        virtual ~ShaderManagementConsoleDocument();

        const AZ::Uuid& GetId() const;

        ////////////////////////////////////////////////////////////////////////
        // ShaderManagementConsoleDocumentRequestBus::Handler implementation
        AZStd::string_view GetAbsolutePath() const override;
        AZStd::string_view GetRelativePath() const override;
        size_t GetShaderOptionCount() const override;
        const AZ::RPI::ShaderOptionDescriptor& GetShaderOptionDescriptor(size_t index) const override;
        size_t GetShaderVariantCount() const override;
        const AZ::RPI::ShaderVariantListSourceData::VariantInfo& GetShaderVariantInfo(size_t index) const override;
        ShaderManagementConsoleDocumentResult Open(AZStd::string_view loadPath) override;
        ShaderManagementConsoleDocumentResult Save() override;
        ShaderManagementConsoleDocumentResult SaveAsCopy(AZStd::string_view savePath) override;
        ShaderManagementConsoleDocumentResult Close() override;
        bool IsOpen() const override;
        bool IsModified() const override;
        bool IsSavable() const override;
        bool CanUndo() const override;
        bool CanRedo() const override;
        bool Undo() override;
        bool Redo() override;
        bool BeginEdit() override;
        bool EndEdit() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        // Function to be bound for undo and redo
        using UndoRedoFunction = AZStd::function<void()>;

        // A pair of functions, where first is the undo operation and second is the redo operation
        using UndoRedoFunctionPair = AZStd::pair<UndoRedoFunction, UndoRedoFunction>;

        // Container for all of the active undo and redo functions and state
        using UndoRedoHistory = AZStd::vector<UndoRedoFunctionPair>;

        void Clear();
 
        // Unique id of this document
        AZ::Uuid m_id = AZ::Uuid::CreateRandom();

        // Relative path to the document
        AZStd::string m_relativePath;

        // Absolute path to the document
        AZStd::string m_absolutePath;

        // Source data for shader variant list
        AZ::RPI::ShaderVariantListSourceData m_shaderVariantListSourceData;

        // Shader asset for the corresponding shader variant list
        AZ::Data::Asset<AZ::RPI::ShaderAsset> m_shaderAsset;

        // Variables needed for tracking the undo and redo state of this document

        // Container of undo commands
        UndoRedoHistory m_undoHistory;

        // The current position in the undo redo history
        int m_undoHistoryIndex = 0;
    };
} // namespace ShaderManagementConsole
