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

    class SlopeGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SlopeGradientNode, AZ::SystemAllocator, 0);
        AZ_RTTI(SlopeGradientNode, "{180A74BE-2244-4DF1-B656-68C66CBD68E7}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        SlopeGradientNode() = default;
        explicit SlopeGradientNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE.toUtf8().constData(); }
    };
}
