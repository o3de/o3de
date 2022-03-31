/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <ACES/Aces.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#endif

namespace MaterialCanvas
{
    struct MaterialCanvasViewportSettings final
    {
        AZ_RTTI(MaterialCanvasViewportSettings, "{16150503-A314-4765-82A3-172670C9EA90}");
        AZ_CLASS_ALLOCATOR(MaterialCanvasViewportSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool m_enableGrid = true;
        bool m_enableShadowCatcher = true;
        bool m_enableAlternateSkybox = false;
        float m_fieldOfView = 90.0f;
        AZ::Render::DisplayMapperOperationType m_displayMapperOperationType = AZ::Render::DisplayMapperOperationType::Aces;
    };
} // namespace MaterialCanvas
