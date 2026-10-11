/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Editor/Nodes/BaseNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{
    //! A visual-only, type-inferred passthrough used to organize Landscape Canvas connections.
    class RerouteNode
        : public BaseNode
    {
    public:
        AZ_CLASS_ALLOCATOR(RerouteNode, AZ::SystemAllocator);
        AZ_RTTI(RerouteNode, "{39E9998E-1B2E-4C5F-AD1B-DC69C639A72A}", BaseNode);

        static void Reflect(AZ::ReflectContext* context);

        RerouteNode() = default;
        explicit RerouteNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        static const GraphModel::SlotName IN_SLOT_ID;
        static const GraphModel::SlotName OUT_SLOT_ID;

        const char* GetTitle() const override;
        bool IsVisualOnly() const override;

    protected:
        void RegisterSlots() override;
    };
} // namespace LandscapeCanvas
