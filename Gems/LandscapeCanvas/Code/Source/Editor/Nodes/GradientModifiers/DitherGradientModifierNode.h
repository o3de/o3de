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

// Qt
#include <QString>

// Landscape Canvas
#include <Editor/Nodes/GradientModifiers/BaseGradientModifierNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class DitherGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(DitherGradientModifierNode, AZ::SystemAllocator, 0);
        AZ_RTTI(DitherGradientModifierNode, "{14B9E63F-5ECA-49AA-B50F-7238B1442E3D}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        DitherGradientModifierNode() = default;
        explicit DitherGradientModifierNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
    };
}
