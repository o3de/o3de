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

#pragma once

// Landscape Canvas
#include <Editor/Nodes/Shapes/BaseShapeNode.h>

namespace LandscapeCanvas
{
    class CapsuleShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CapsuleShapeNode, AZ::SystemAllocator, 0);
        AZ_RTTI(CapsuleShapeNode, "{537C20E6-D319-4CEE-BAA5-75C4B52E5BE0}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        CapsuleShapeNode() = default;
        explicit CapsuleShapeNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
    };
}
