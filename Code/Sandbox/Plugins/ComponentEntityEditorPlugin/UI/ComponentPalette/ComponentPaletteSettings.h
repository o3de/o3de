/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once


#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettings.h>

//=============================================================================

class ComponentPaletteSettings
    : public AZ::UserSettings
{
public:
    AZ_CLASS_ALLOCATOR(ComponentPaletteSettings, AZ::SystemAllocator, 0);
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

    static void Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<ComponentPaletteSettings>()
                ->Version(1)
                ->Field("m_favorites", &ComponentPaletteSettings::m_favorites)
                ;
        }
    }
};

