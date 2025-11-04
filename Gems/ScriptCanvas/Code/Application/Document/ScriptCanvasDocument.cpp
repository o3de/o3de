/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptCanvasDocument.h"

#include <AtomToolsFramework/Document/AtomToolsDocumentTypeInfo.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    void ScriptCanvasDocument::Reflect(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<ScriptCanvasDocument, AtomToolsFramework::AtomToolsDocument>()->Version(0);
        }
    }

    ScriptCanvasDocument::ScriptCanvasDocument(const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo)
        : AtomToolsFramework::AtomToolsDocument(toolId, documentTypeInfo)
    {
        ScriptCanvasDocumentRequestBus::Handler::BusConnect(m_id);
    }

    ScriptCanvasDocument::~ScriptCanvasDocument()
    {
        ScriptCanvasDocumentRequestBus::Handler::BusDisconnect();
    }

    AtomToolsFramework::DocumentTypeInfo ScriptCanvasDocument::BuildDocumentTypeInfo()
    {
        AtomToolsFramework::DocumentTypeInfo documentType;
        documentType.m_documentTypeName = "ScriptCanvas";
        documentType.m_documentFactoryCallback = [](const AZ::Crc32& toolId, const AtomToolsFramework::DocumentTypeInfo& documentTypeInfo)
        {
            return aznew ScriptCanvasDocument(toolId, documentTypeInfo);
        };
        documentType.m_supportedExtensionsToCreate.push_back({ "Script Canvas", "scriptcanvas" });
        documentType.m_supportedExtensionsToOpen.push_back({ "Script Canvas", "scriptcanvas" });
        return documentType;
    }

} // namespace ScriptCanvas
