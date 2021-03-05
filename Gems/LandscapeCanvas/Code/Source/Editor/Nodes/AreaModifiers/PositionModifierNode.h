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
#include <Editor/Nodes/AreaModifiers/BaseAreaModifierNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class PositionModifierNode : public BaseAreaModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(PositionModifierNode, AZ::SystemAllocator, 0);
        AZ_RTTI(PositionModifierNode, "{3613E5F4-BBFF-4FC5-90B5-902B3FFE7F8D}", BaseAreaModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        PositionModifierNode() = default;
        explicit PositionModifierNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override
        {
            return TITLE.toUtf8().constData();
        }

    protected:
        void RegisterSlots() override;
    };
}
