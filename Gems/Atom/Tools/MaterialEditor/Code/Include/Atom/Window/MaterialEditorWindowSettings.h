/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/UserSettings/UserSettings.h>
#endif

namespace MaterialEditor
{
    struct MaterialEditorWindowSettings
        : public AZ::UserSettings
    {
        AZ_RTTI(MaterialEditorWindowSettings, "{BB9DEB77-B7BE-4DF5-9FDD-6D9F3136C4EA}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(MaterialEditorWindowSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        AZStd::vector<char> m_mainWindowState;
        AZStd::unordered_set<AZ::u32> m_inspectorCollapsedGroups;
    };
} // namespace MaterialEditor
