/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

    class SurfaceMaskGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SurfaceMaskGradientNode, AZ::SystemAllocator, 0);
        AZ_RTTI(SurfaceMaskGradientNode, "{A14FC1D9-1D3D-4385-AD07-8E4170637290}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        SurfaceMaskGradientNode() = default;
        explicit SurfaceMaskGradientNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE.toUtf8().constData(); }
    };
}
