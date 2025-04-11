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
    class ShapeIntersectionFilterNode : public BaseAreaFilterNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ShapeIntersectionFilterNode, AZ::SystemAllocator);
        AZ_RTTI(ShapeIntersectionFilterNode, "{5E4CED27-A263-446F-A325-4D25891855F1}", BaseAreaFilterNode);

        static void Reflect(AZ::ReflectContext* context);

        ShapeIntersectionFilterNode() = default;
        explicit ShapeIntersectionFilterNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }

    protected:
        void RegisterSlots() override;
    };
}
