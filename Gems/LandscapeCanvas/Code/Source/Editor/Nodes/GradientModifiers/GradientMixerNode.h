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
#include <Editor/Core/Core.h>
#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class GradientMixerNode : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(GradientMixerNode, AZ::SystemAllocator, 0);
        AZ_RTTI(GradientMixerNode, "{D5AEAA23-12EB-4E55-B396-BEE13724FBDC}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        GradientMixerNode() = default;
        explicit GradientMixerNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override;
        const char* GetSubTitle() const override;
        const BaseNodeType GetBaseNodeType() const override;

    protected:
        virtual void RegisterSlots() override;
    };
}
