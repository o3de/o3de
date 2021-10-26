/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>

namespace AtomToolsFramework
{
    AtomToolsDocument::AtomToolsDocument()
    {
        AtomToolsDocumentRequestBus::Handler::BusConnect(m_id);
        AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentCreated, m_id);
    }

    AtomToolsDocument::~AtomToolsDocument()
    {
        AtomToolsDocumentNotificationBus::Broadcast(&AtomToolsDocumentNotificationBus::Events::OnDocumentDestroyed, m_id);
        AtomToolsDocumentRequestBus::Handler::BusDisconnect();
    }

    const AZ::Uuid& AtomToolsDocument::GetId() const
    {
        return m_id;
    }

    AZStd::string_view AtomToolsDocument::GetAbsolutePath() const
    {
        return m_absolutePath;
    }

    AZStd::string_view AtomToolsDocument::GetRelativePath() const
    {
        return m_relativePath;
    }

    const AZStd::any& AtomToolsDocument::GetPropertyValue([[maybe_unused]] const AZ::Name& propertyId) const
    {
        AZ_UNUSED(propertyId);
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return m_invalidValue;
    }

    const AtomToolsFramework::DynamicProperty& AtomToolsDocument::GetProperty([[maybe_unused]] const AZ::Name& propertyId) const
    {
        AZ_UNUSED(propertyId);
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return m_invalidProperty;
    }
    
    bool AtomToolsDocument::IsPropertyGroupVisible([[maybe_unused]] const AZ::Name& propertyGroupFullName) const
    {
        AZ_UNUSED(propertyGroupFullName);
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    void AtomToolsDocument::SetPropertyValue([[maybe_unused]] const AZ::Name& propertyId, [[maybe_unused]] const AZStd::any& value)
    {
        AZ_UNUSED(propertyId);
        AZ_UNUSED(value);
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
    }

    bool AtomToolsDocument::Open([[maybe_unused]] AZStd::string_view loadPath)
    {
        AZ_UNUSED(loadPath);
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::Reopen()
    {
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::Save()
    {
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::SaveAsCopy([[maybe_unused]] AZStd::string_view savePath)
    {
        AZ_UNUSED(savePath);
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }


    bool AtomToolsDocument::SaveAsChild([[maybe_unused]] AZStd::string_view savePath)
    {
        AZ_UNUSED(savePath);
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::Close()
    {
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::IsOpen() const
    {
        return false;
    }

    bool AtomToolsDocument::IsModified() const
    {
        return false;
    }

    bool AtomToolsDocument::IsSavable() const
    {
        return false;
    }

    bool AtomToolsDocument::CanUndo() const
    {
        return false;
    }

    bool AtomToolsDocument::CanRedo() const
    {
        return false;
    }

    bool AtomToolsDocument::Undo()
    {
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::Redo()
    {
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::BeginEdit()
    {
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }

    bool AtomToolsDocument::EndEdit()
    {
        AZ_Error("AtomToolsDocument", false, "%s not implemented.", __FUNCTION__);
        return false;
    }
} // namespace AtomToolsFramework
