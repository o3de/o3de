/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QString>

#include <Editor/Core/Core.h>
#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class PhysXHeightfieldColliderNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(PhysXHeightfieldColliderNode, AZ::SystemAllocator);
        AZ_RTTI(PhysXHeightfieldColliderNode, "{F2214078-EB6E-4EDE-AE5C-65AB3D34ACD7}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        PhysXHeightfieldColliderNode() = default;
        explicit PhysXHeightfieldColliderNode(GraphModel::GraphPtr graph);

        const BaseNodeType GetBaseNodeType() const override;

        static const char* TITLE;
        const char* GetTitle() const override;
    };
}
