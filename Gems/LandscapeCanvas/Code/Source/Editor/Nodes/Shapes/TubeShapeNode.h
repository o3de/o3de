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
    class TubeShapeNode : public BaseShapeNode
    {
    public:
        AZ_CLASS_ALLOCATOR(TubeShapeNode, AZ::SystemAllocator, 0);
        AZ_RTTI(TubeShapeNode, "{2A0193AD-25AC-49CA-B90E-8C61A5C1CCC5}", BaseShapeNode);

        static void Reflect(AZ::ReflectContext* context);

        TubeShapeNode() = default;
        explicit TubeShapeNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
    };
}
