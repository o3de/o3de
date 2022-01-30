/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

namespace AtomToolsFramework
{
    /**
     * AtomToolsDocument provides an API for modifying and saving documents.
     */
    class AtomToolsDocument
        : public AtomToolsDocumentRequestBus::Handler
        , private AzToolsFramework::AssetSystemBus::Handler
    {
    public:
        AZ_RTTI(AtomToolsDocument, "{8992DF74-88EC-438C-B280-6E71D4C0880B}");
        AZ_CLASS_ALLOCATOR(AtomToolsDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY(AtomToolsDocument);

        AtomToolsDocument();
        virtual ~AtomToolsDocument();

        const AZ::Uuid& GetId() const;

        // AtomToolsDocumentRequestBus::Handler overrides...
        AZStd::string_view GetAbsolutePath() const override;
        const AZStd::any& GetPropertyValue(const AZ::Name& propertyId) const override;
        const AtomToolsFramework::DynamicProperty& GetProperty(const AZ::Name& propertyId) const override;
        bool IsPropertyGroupVisible(const AZ::Name& propertyGroupFullName) const override;
        void SetPropertyValue(const AZ::Name& propertyId, const AZStd::any& value) override;
        bool Open(AZStd::string_view loadPath) override;
        bool Reopen() override;
        bool Save() override;
        bool SaveAsCopy(AZStd::string_view savePath) override;
        bool SaveAsChild(AZStd::string_view savePath) override;
        bool Close() override;
        bool IsOpen() const override;
        bool IsModified() const override;
        bool IsSavable() const override;
        bool CanUndo() const override;
        bool CanRedo() const override;
        bool Undo() override;
        bool Redo() override;
        bool BeginEdit() override;
        bool EndEdit() override;

    protected:
        virtual void Clear();

        virtual bool OpenSucceeded();
        virtual bool OpenFailed();

        virtual bool ReopenRecordState();
        virtual bool ReopenRestoreState();

        virtual bool SaveSucceeded();
        virtual bool SaveFailed();

        // Unique id of this document
        AZ::Uuid m_id = AZ::Uuid::CreateRandom();

        // Absolute path to the material source file
        AZStd::string m_absolutePath;

        AZStd::string m_savePathNormalized;

        AZStd::any m_invalidValue;
        
        AtomToolsFramework::DynamicProperty m_invalidProperty;

        // Set of assets that can trigger a document reload
        AZStd::unordered_set<AZStd::string> m_sourceDependencies;

        // Track if document saved itself last to skip external modification notification
        bool m_saveTriggeredInternally = false;

        // Variables needed for tracking the undo and redo state of this document

        // Function to be bound for undo and redo
        using UndoRedoFunction = AZStd::function<void()>;

        // A pair of functions, where first is the undo operation and second is the redo operation
        using UndoRedoFunctionPair = AZStd::pair<UndoRedoFunction, UndoRedoFunction>;

        // Container for all of the active undo and redo functions and state
        using UndoRedoHistory = AZStd::vector<UndoRedoFunctionPair>;

        // Container of undo commands
        UndoRedoHistory m_undoHistory;
        UndoRedoHistory m_undoHistoryBeforeReopen;

        // The current position in the undo redo history
        int m_undoHistoryIndex = {};
        int m_undoHistoryIndexBeforeReopen = {};

        void AddUndoRedoHistory(const UndoRedoFunction& undoCommand, const UndoRedoFunction& redoCommand);

        // AzToolsFramework::AssetSystemBus::Handler overrides...
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
    };
} // namespace AtomToolsFramework
