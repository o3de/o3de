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
    class SphereShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SphereShapeNode, AZ::SystemAllocator, 0);
        AZ_RTTI(SphereShapeNode, "{9ED3D9AF-E82D-4291-874A-B0758879CF39}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        SphereShapeNode() = default;
        explicit SphereShapeNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
    };
}
