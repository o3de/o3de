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
#include <Editor/Nodes/Areas/BaseAreaNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class AreaBlenderNode : public BaseAreaNode
    {
    public:
        AZ_CLASS_ALLOCATOR(AreaBlenderNode, AZ::SystemAllocator, 0);
        AZ_RTTI(AreaBlenderNode, "{07EFA0EA-F5E1-44A0-8620-D5A75F2D2BED}", BaseAreaNode);

        static void Reflect(AZ::ReflectContext* context);

        AreaBlenderNode() = default;
        explicit AreaBlenderNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }

    protected:
        void RegisterSlots() override;
    };
}
