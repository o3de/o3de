/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentRequestBus.h>

namespace AtomToolsFramework
{
    /**
     * AtomToolsDocument provides an API for modifying and saving documents.
     */
    class AtomToolsDocument
        : public AtomToolsDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(AtomToolsDocument, "{8992DF74-88EC-438C-B280-6E71D4C0880B}");
        AZ_CLASS_ALLOCATOR(AtomToolsDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY(AtomToolsDocument);

        AtomToolsDocument();
        virtual ~AtomToolsDocument();

        const AZ::Uuid& GetId() const;

        ////////////////////////////////////////////////////////////////////////
        // AtomToolsDocumentRequestBus::Handler implementation
        AZStd::string_view GetAbsolutePath() const override;
        AZStd::string_view GetRelativePath() const override;
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
        ////////////////////////////////////////////////////////////////////////

    protected:

        // Unique id of this document
        AZ::Uuid m_id = AZ::Uuid::CreateRandom();

        // Relative path to the material source file
        AZStd::string m_relativePath;

        // Absolute path to the material source file
        AZStd::string m_absolutePath;

        AZStd::any m_invalidValue;
        
        AtomToolsFramework::DynamicProperty m_invalidProperty;
    };
} // namespace AtomToolsFramework
