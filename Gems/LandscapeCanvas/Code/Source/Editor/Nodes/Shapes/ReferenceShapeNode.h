/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Nodes/BaseNode.h>

namespace LandscapeCanvas
{
    class ReferenceShapeNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ReferenceShapeNode, AZ::SystemAllocator);
        AZ_RTTI(ReferenceShapeNode, "{DD8E2150-A80C-4740-9EA5-26B7BC3C1993}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        ReferenceShapeNode() = default;
        explicit ReferenceShapeNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override;
        const char* GetSubTitle() const override;
        const BaseNodeType GetBaseNodeType() const override;

    protected:
        void RegisterSlots() override;
    };
}
