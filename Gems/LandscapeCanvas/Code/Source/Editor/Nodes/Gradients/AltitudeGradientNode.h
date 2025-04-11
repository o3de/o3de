/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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

    class AltitudeGradientNode : public BaseGradientNode
    {
    public:
        AZ_CLASS_ALLOCATOR(AltitudeGradientNode, AZ::SystemAllocator);
        AZ_RTTI(AltitudeGradientNode, "{343A6869-079C-4DEA-A15B-06E8B166CE03}", BaseGradientNode);

        static void Reflect(AZ::ReflectContext* context);

        AltitudeGradientNode() = default;
        explicit AltitudeGradientNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
        const char* GetSubTitle() const override { return LandscapeCanvas::GRADIENT_TITLE; }

    protected:
        void RegisterSlots() override;
    };
}
