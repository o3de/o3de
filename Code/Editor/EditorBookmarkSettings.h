/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>


#include <AzCore/Serialization/Json/JsonSystemComponent.h>

struct BookmarkConfig
{
    AZ_CLASS_ALLOCATOR(BookmarkConfig, AZ::SystemAllocator, 0);
    AZ_RTTI(BookmarkConfig, "{522A38D9-6FFF-4B96-BECF-B4D0F7ABCD25}");

    static void Reflect(AZ::ReflectContext* context);

    float m_xPos;
    float m_yPos;
    float m_zPos;
};

class EditorBookmarkSettings
{

public:

    AZ_CLASS_ALLOCATOR(EditorBookmarkSettings, AZ::SystemAllocator, 0);
    AZ_RTTI(EditorBookmarkSettings, "{27D332DD-2CAF-443A-8A09-3D023CF2474B}");

    EditorBookmarkSettings();
    virtual ~EditorBookmarkSettings();

    static void Reflect(AZ::ReflectContext* context);

    void SaveBookmarkSettingsFile();

private:
    void Setup();

private:
    BookmarkConfig m_bookmarkConfig;
    AZStd::unique_ptr <AZ::SettingsRegistryImpl> m_bookmarSettings;
    AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
    AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
};
