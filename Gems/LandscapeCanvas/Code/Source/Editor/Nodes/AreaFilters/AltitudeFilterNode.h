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
#include <Editor/Nodes/AreaFilters/BaseAreaFilterNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    class AltitudeFilterNode : public BaseAreaFilterNode
    {
    public:
        AZ_CLASS_ALLOCATOR(AltitudeFilterNode, AZ::SystemAllocator);
        AZ_RTTI(AltitudeFilterNode, "{42F4CF45-597B-4FB9-A21C-2B38A1F25FEA}", BaseAreaFilterNode);

        static void Reflect(AZ::ReflectContext* context);

        AltitudeFilterNode() = default;
        explicit AltitudeFilterNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override
        {
            return TITLE;
        }

    protected:
        void RegisterSlots() override;
    };
}
