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
#include <AzCore/UserSettings/UserSettings.h>
#endif

namespace OpenParticleSystemEditor
{
    struct OpenParticleViewportSettings
        : public AZ::UserSettings
    {
        AZ_RTTI(OpenParticleViewportSettings, "{EC0532F6-D6FA-4A05-A96B-4727B59EA4A4}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(OpenParticleViewportSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool m_enableGrid = true;
        bool m_enableAlternateSkybox = false;
        AZ::Render::DisplayMapperOperationType m_displayMapperOperationType = AZ::Render::DisplayMapperOperationType::Aces;
        AZStd::string m_selectedLightingPresetName = "Default";
    };
} // namespace OpenParticleSystemViewportSettings
