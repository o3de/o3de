/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/AssetUtils.h>
#include <Atom/RPI.Edit/Common/JsonUtils.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Document/MaterialCanvasDocument.h>

namespace MaterialCanvas
{
    void MaterialCanvasDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<MaterialCanvasDocument, AtomToolsFramework::AtomToolsDocument>()
                ->Version(0);
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<MaterialCanvasDocumentRequestBus>("MaterialCanvasDocumentRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Category, "Editor")
                ->Attribute(AZ::Script::Attributes::Module, "materialcanvas")
                ;
        }
    }

    MaterialCanvasDocument::MaterialCanvasDocument(const AZ::Crc32& toolId)
        : AtomToolsFramework::AtomToolsDocument(toolId)
    {
        MaterialCanvasDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialCanvasDocument::~MaterialCanvasDocument()
    {
        MaterialCanvasDocumentRequestBus::Handler::BusDisconnect();
    }

    AZStd::vector<AtomToolsFramework::DocumentObjectInfo> MaterialCanvasDocument::GetObjectInfo() const
    {
        if (!IsOpen())
        {
            return {};
        }

        AZStd::vector<AtomToolsFramework::DocumentObjectInfo> objects;
        return objects;
    }

    bool MaterialCanvasDocument::Open(AZStd::string_view loadPath)
    {
        if (!AtomToolsDocument::Open(loadPath))
        {
            return false;
        }

        return OpenFailed();
    }

    bool MaterialCanvasDocument::Save()
    {
        if (!AtomToolsDocument::Save())
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

            return SaveFailed();
    }

    bool MaterialCanvasDocument::SaveAsCopy(AZStd::string_view savePath)
    {
        if (!AtomToolsDocument::SaveAsCopy(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveFailed();
    }

    bool MaterialCanvasDocument::SaveAsChild(AZStd::string_view savePath)
    {
        if (!AtomToolsDocument::SaveAsChild(savePath))
        {
            // SaveFailed has already been called so just forward the result without additional notifications.
            // TODO Replace bool return value with enum for open and save states.
            return false;
        }

        return SaveFailed();
    }

    bool MaterialCanvasDocument::IsOpen() const
    {
        return AtomToolsDocument::IsOpen();
    }

    bool MaterialCanvasDocument::IsModified() const
    {
        bool result = false;
        return result;
    }

    bool MaterialCanvasDocument::IsSavable() const
    {
        return false;
    }

    bool MaterialCanvasDocument::BeginEdit()
    {
        // Save the current properties as a momento for undo before any changes are applied
        return true;
    }

    bool MaterialCanvasDocument::EndEdit()
    {
        return true;
    }

    void MaterialCanvasDocument::Clear()
    {
        AtomToolsFramework::AtomToolsDocument::Clear();
    }

    bool MaterialCanvasDocument::ReopenRecordState()
    {
        return AtomToolsDocument::ReopenRecordState();
    }

    bool MaterialCanvasDocument::ReopenRestoreState()
    {
        return AtomToolsDocument::ReopenRestoreState();
    }
} // namespace MaterialCanvas
