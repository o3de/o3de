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
#include <Editor/Core/Core.h>
#include <Editor/Nodes/Gradients/BaseGradientNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    // The FastNoise gem is optional, so we need to keep track of its component type ID
    // ourselves since we can't rely on the headers being there.
    static const AZ::TypeId EditorFastNoiseGradientComponentTypeId = "{FD018DE5-5EB4-4219-9D0C-CB3C55DE656B}";

    class FastNoiseGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(FastNoiseGradientNode, AZ::SystemAllocator, 0);
        AZ_RTTI(FastNoiseGradientNode, "{38A4CDEA-082B-4769-922B-713BF77CEA28}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        FastNoiseGradientNode() = default;
        explicit FastNoiseGradientNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_GENERATOR_TITLE.toUtf8().constData(); }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::GradientGenerator; }
    };
}
