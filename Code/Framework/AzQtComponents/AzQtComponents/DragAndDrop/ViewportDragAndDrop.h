/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
        static const AZ::u32 EditorViewport = AZ_CRC("EditorViewport", 0x17395aca);
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
        AZ_CLASS_ALLOCATOR(ViewportDragContext, AZ::SystemAllocator, 0);
        AZ_RTTI(ViewportDragContext, "{8297256B-8DD4-499C-B564-0EAA829E8ACA}", AzQtComponents::DragAndDropContextBase);
        AZ::Vector3 m_hitLocation;
    };

}
