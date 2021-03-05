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
    class RotationModifierNode : public BaseAreaModifierNode
    {
    public:
        AZ_CLASS_ALLOCATOR(RotationModifierNode, AZ::SystemAllocator, 0);
        AZ_RTTI(RotationModifierNode, "{14B577ED-4135-4711-B9F3-016E106EA66B}", BaseAreaModifierNode);

        static void Reflect(AZ::ReflectContext* context);

        RotationModifierNode() = default;
        explicit RotationModifierNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override
        {
            return TITLE.toUtf8().constData();
        }

    protected:
        void RegisterSlots() override;
    };
}
