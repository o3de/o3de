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
    class DistanceBetweenFilterNode : public BaseAreaFilterNode
    {
    public:
        AZ_CLASS_ALLOCATOR(DistanceBetweenFilterNode, AZ::SystemAllocator);
        AZ_RTTI(DistanceBetweenFilterNode, "{32D28506-B2A1-4EC6-A0A2-CBCA0B7073CF}", BaseAreaFilterNode);

        static void Reflect(AZ::ReflectContext* context);

        DistanceBetweenFilterNode() = default;
        explicit DistanceBetweenFilterNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }
    };
}
