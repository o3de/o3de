/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ::Render
{
    // RenderActorSettings is a subset of RenderOptions. The goal is eventually move those actor render related settings out of render options since
    // it will be shared between main editor and animation editor.
    class RenderActorSettings
    {
    public:
        AZ_RTTI(RenderActorSettings, "{240BDFE2-D7F5-4927-A8CA-D2945E41AFFD}");
        AZ_CLASS_ALLOCATOR(RenderActorSettings, AZ::SystemAllocator, 0)

        virtual ~RenderActorSettings() = default;

        float m_vertexNormalsScale = 1.0f;
        float m_faceNormalsScale = 1.0f;
        float m_tangentsScale = 1.0f;
        float m_wireframeScale = 1.0f;

        AZ::Color m_vertexNormalsColor{ 0.0f, 1.0f, 0.0f, 1.0f };
        AZ::Color m_faceNormalsColor{ 0.5f, 0.5f, 1.0f, 1.0f };
        AZ::Color m_tangentsColor{ 1.0f, 0.0f, 0.0f, 1.0f };
        AZ::Color m_mirroredBitangentsColor{ 1.0f, 1.0f, 0.0f, 1.0f };
        AZ::Color m_bitangentsColor{ 1.0f, 1.0f, 1.0f, 1.0f };
        AZ::Color m_wireframeColor{ 0.0f, 0.0f, 0.0f, 1.0f };
        AZ::Color m_staticAABBColor{ 0.0f, 0.7f, 0.7f, 1.0f };
        AZ::Color m_lineSkeletonColor{ 0.33333f, 1.0f, 0.0f, 1.0f };
        AZ::Color m_skeletonColor{ 0.19f, 0.58f, 0.19f, 1.0f };
    };
} // namespace AZ::Render
