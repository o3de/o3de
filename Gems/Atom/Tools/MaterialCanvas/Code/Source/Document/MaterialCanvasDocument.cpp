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
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>
#include <Document/MaterialCanvasDocument.h>

namespace MaterialCanvas
{
    MaterialCanvasDocument::MaterialCanvasDocument(const AZ::Crc32& toolId)
        : AtomToolsFramework::AtomToolsDocument(toolId)
    {
        MaterialCanvasDocumentRequestBus::Handler::BusConnect(m_id);
    }

    MaterialCanvasDocument::~MaterialCanvasDocument()
    {
        MaterialCanvasDocumentRequestBus::Handler::BusDisconnect();
    }
} // namespace MaterialCanvas
