/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/RTTI/RTTIMacros.h>

#include <AzCore/UserSettings/UserSettings.h>

//=============================================================================
namespace AZ
{
    class ReflectContext;
}

class ComponentPaletteSettings
    : public AZ::UserSettings
{
public:
    AZ_CLASS_ALLOCATOR(ComponentPaletteSettings, AZ::SystemAllocator);
    AZ_RTTI(ComponentPaletteSettings, "{BAC3BABA-6DF1-4EEE-AFF1-6A84AD1820A1}", AZ::UserSettings);

    AZStd::vector<AZ::Uuid> m_favorites;

    void SetFavorites(AZStd::vector<AZ::Uuid>&& componentIds)
    {
        m_favorites = AZStd::move(componentIds);
    }

    void RemoveFavorites(const AZStd::vector<AZ::Uuid>& componentIds)
    {
        for (const AZ::Uuid& componentId : componentIds)
        {
            auto favoriteIterator = AZStd::find(m_favorites.begin(), m_favorites.end(), componentId);
            AZ_Assert(favoriteIterator != m_favorites.end(), "Component Palette Favorite not found.");

            if (favoriteIterator != m_favorites.end())
            {
                m_favorites.erase(favoriteIterator);
            }
        }
    }

    static const char* GetSettingsFile()
    {
        static const char* settingsFile("@user@/editor/componentpalette.usersettings");
        return settingsFile;
    }

    static void Reflect(AZ::ReflectContext* context);
};

