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
#include <Editor/Nodes/Gradients/BaseGradientNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class RandomNoiseGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(RandomNoiseGradientNode, AZ::SystemAllocator, 0);
        AZ_RTTI(RandomNoiseGradientNode, "{DE6B5261-81AE-46DB-9DC3-35573C866909}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        RandomNoiseGradientNode() = default;
        explicit RandomNoiseGradientNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_GENERATOR_TITLE.toUtf8().constData(); }

        const BaseNodeType GetBaseNodeType() const override { return BaseNode::GradientGenerator; }
    };
}
