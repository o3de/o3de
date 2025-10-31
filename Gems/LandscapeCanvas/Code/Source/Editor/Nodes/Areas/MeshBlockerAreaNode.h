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
#include <Editor/Nodes/Areas/BaseAreaNode.h>

namespace AZ
{
    class ReflectContext;
}

namespace LandscapeCanvas
{

    class MeshBlockerAreaNode : public BaseAreaNode
    {
    public:
        AZ_CLASS_ALLOCATOR(MeshBlockerAreaNode, AZ::SystemAllocator);
        AZ_RTTI(MeshBlockerAreaNode, "{27CE0BEC-5AE3-4574-860E-C24D142D10F5}", BaseAreaNode);

        static void Reflect(AZ::ReflectContext* context);

        MeshBlockerAreaNode() = default;
        explicit MeshBlockerAreaNode(GraphModel::GraphPtr graph);

        static const char* TITLE;
        const char* GetTitle() const override { return TITLE; }

    protected:
        void RegisterSlots() override;
    };
}
