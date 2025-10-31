/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// Qt
#include <QString>

// Landscape Canvas
#include <Editor/Core/Core.h>
#include <Editor/Nodes/AreaFilters/BaseAreaFilterNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class SurfaceMaskDepthFilterNode : public BaseAreaFilterNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceMaskDepthFilterNode, AZ::SystemAllocator);
        AZ_RTTI(SurfaceMaskDepthFilterNode, "{0ED56444-24AC-4715-89C2-1DBFCA3C6DFA}", BaseAreaFilterNode);

        static void Reflect(AZ::ReflectContext* context);

        SurfaceMaskDepthFilterNode() = default;
        explicit SurfaceMaskDepthFilterNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }
    };
}
