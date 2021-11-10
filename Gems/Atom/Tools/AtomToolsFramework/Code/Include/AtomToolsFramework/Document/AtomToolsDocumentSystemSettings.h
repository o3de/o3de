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
#include <AzCore/UserSettings/UserSettings.h>
#endif

namespace AtomToolsFramework
{
    struct AtomToolsDocumentSystemSettings
        : public AZ::UserSettings
    {
        AZ_RTTI(AtomToolsDocumentSystemSettings, "{9E576D4F-A74A-4326-9135-C07284D0A3B9}", AZ::UserSettings);
        AZ_CLASS_ALLOCATOR(AtomToolsDocumentSystemSettings, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* context);

        bool m_showReloadDocumentPrompt = true;
    };
} // namespace AtomToolsFramework
