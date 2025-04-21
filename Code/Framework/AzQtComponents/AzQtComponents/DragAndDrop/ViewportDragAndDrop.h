/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Vector3.h>

#include <AzQtComponents/Buses/DragAndDrop.h>

#pragma once

namespace AzQtComponents
{
    namespace DragAndDropContexts 
    {
        static const AZ::u32 EditorViewport = AZ_CRC_CE("EditorViewport");
    }

    /**
    *   This context is sent when dragging OVER a viewport type view.
    *   It adds common viewport-useful details to the context, such as a suggested position to spawn objects
    *   (in 3d world space) if such an operation is desired.
    */
    class ViewportDragContext : public AzQtComponents::DragAndDropContextBase
    {
    public:
        // viewport adds hitlocation
        AZ_CLASS_ALLOCATOR(ViewportDragContext, AZ::SystemAllocator);
        AZ_RTTI(ViewportDragContext, "{8297256B-8DD4-499C-B564-0EAA829E8ACA}", AzQtComponents::DragAndDropContextBase);
        AZ::Vector3 m_hitLocation;
    };

}
