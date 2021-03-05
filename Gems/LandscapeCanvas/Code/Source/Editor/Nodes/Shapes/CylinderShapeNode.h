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
    class CylinderShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CylinderShapeNode, AZ::SystemAllocator, 0);
        AZ_RTTI(CylinderShapeNode, "{5F0DD9F7-BCB8-4428-A3BD-8FEC3141E2D0}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        CylinderShapeNode() = default;
        explicit CylinderShapeNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
    };
}
