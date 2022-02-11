/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/RTTI/RTTI.h>
#include <Document/MaterialCanvasDocumentRequestBus.h>

namespace MaterialCanvas
{
    //! MaterialCanvasDocument
    class MaterialCanvasDocument
        : public AtomToolsFramework::AtomToolsDocument
        , public MaterialCanvasDocumentRequestBus::Handler
    {
    public:
        AZ_RTTI(MaterialCanvasDocument, "{628858AB-4FE0-4284-BF40-BB38E7374F93}");
        AZ_CLASS_ALLOCATOR(MaterialCanvasDocument, AZ::SystemAllocator, 0);
        AZ_DISABLE_COPY_MOVE(MaterialCanvasDocument);

        MaterialCanvasDocument(const AZ::Crc32& toolId);
        virtual ~MaterialCanvasDocument();
    };
} // namespace MaterialCanvas
