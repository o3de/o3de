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
    class SlopeFilterNode : public BaseAreaFilterNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SlopeFilterNode, AZ::SystemAllocator);
        AZ_RTTI(SlopeFilterNode, "{00DF204E-6915-488C-801F-E9E72568C8FF}", BaseAreaFilterNode);

        static void Reflect(AZ::ReflectContext* context);

        SlopeFilterNode() = default;
        explicit SlopeFilterNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }
    };
}
