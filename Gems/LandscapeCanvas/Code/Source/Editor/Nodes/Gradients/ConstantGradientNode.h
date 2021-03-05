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

    class ConstantGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(ConstantGradientNode, AZ::SystemAllocator, 0);
        AZ_RTTI(ConstantGradientNode, "{3D45B989-815F-461E-943D-F5139C1C6F06}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        ConstantGradientNode() = default;
        explicit ConstantGradientNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE.toUtf8().constData(); }
    };
}
