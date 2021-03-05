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

    class ThresholdGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ThresholdGradientModifierNode, AZ::SystemAllocator, 0);
        AZ_RTTI(ThresholdGradientModifierNode, "{3AFD613A-E5E4-4359-9E20-335F202AAB99}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        ThresholdGradientModifierNode() = default;
        explicit ThresholdGradientModifierNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
    };
}
