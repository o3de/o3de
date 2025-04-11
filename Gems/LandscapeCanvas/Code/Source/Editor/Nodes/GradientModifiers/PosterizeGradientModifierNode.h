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
#include <Editor/Nodes/GradientModifiers/BaseGradientModifierNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class PosterizeGradientModifierNode : public BaseGradientModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(PosterizeGradientModifierNode, AZ::SystemAllocator);
        AZ_RTTI(PosterizeGradientModifierNode, "{92EC1905-BCCA-4614-9F64-B2AAF488DBA6}", BaseGradientModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        PosterizeGradientModifierNode() = default;
        explicit PosterizeGradientModifierNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }
    };
}
