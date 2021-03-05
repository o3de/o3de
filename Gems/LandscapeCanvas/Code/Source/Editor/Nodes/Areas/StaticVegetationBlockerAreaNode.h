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

    class StaticVegetationBlockerAreaNode : public BaseAreaNode
    {
    public:
        AZ_CLASS_ALLOCATOR(StaticVegetationBlockerAreaNode, AZ::SystemAllocator, 0);
        AZ_RTTI(StaticVegetationBlockerAreaNode, "{828A64EE-C78A-4A67-8A88-99B39E5C90D8}", BaseAreaNode);

        static void Reflect(AZ::ReflectContext* context);

        StaticVegetationBlockerAreaNode() = default;
        explicit StaticVegetationBlockerAreaNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override { return TITLE.toUtf8().constData(); }
    };
}
