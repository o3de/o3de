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

namespace MaterialEditor
{
    struct MaterialViewportSettings final
    {
        AZ_RTTI(MaterialViewportSettings, "{16150503-A314-4765-82A3-172670C9EA90}");
        AZ_CLASS_ALLOCATOR(MaterialViewportSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool m_enableGrid = true;
        bool m_enableShadowCatcher = true;
        bool m_enableAlternateSkybox = false;
        float m_fieldOfView = 90.0f;
        AZ::Render::DisplayMapperOperationType m_displayMapperOperationType = AZ::Render::DisplayMapperOperationType::Aces;
    };
} // namespace MaterialEditor
