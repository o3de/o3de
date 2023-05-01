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
    class SurfaceMaskFilterNode : public BaseAreaFilterNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceMaskFilterNode, AZ::SystemAllocator);
        AZ_RTTI(SurfaceMaskFilterNode, "{FCF18A51-A09D-488A-A909-BDD68E3C59E8}", BaseAreaFilterNode);

        static void Reflect(AZ::ReflectContext* context);

        SurfaceMaskFilterNode() = default;
        explicit SurfaceMaskFilterNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }
    };
}
