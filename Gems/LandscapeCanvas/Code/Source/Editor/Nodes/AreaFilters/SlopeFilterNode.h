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
#include <Editor/Nodes/AreaFilters/BaseAreaFilterNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class SlopeFilterNode : public BaseAreaFilterNode
    {
    public:
        AZ_CLASS_ALLOCATOR(SlopeFilterNode, AZ::SystemAllocator, 0);
        AZ_RTTI(SlopeFilterNode, "{00DF204E-6915-488C-801F-E9E72568C8FF}", BaseAreaFilterNode);

        static void Reflect(AZ::ReflectContext* context);

        SlopeFilterNode() = default;
        explicit SlopeFilterNode(GraphModel::GraphPtr graph);

        static const QString TITLE;
        const char* GetTitle() const override
        {
            return TITLE.toUtf8().constData();
        }
    };
}
