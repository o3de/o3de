/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <QtWidgets/qaction.h>
#include "CustomMenusAPI.h"

#include <QMenu>

namespace LegacyFramework
{
    class CustomMenusComponent
        : public AZ::Component
        , private LegacyFramework::CustomMenusMessages::Bus::Handler
    {
    public:
        AZ_COMPONENT(CustomMenusComponent, "{34A1245B-AA6B-41BE-8CFD-50141877081A}")

        void Init() override {}
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* reflection);

    private:
        void RegisterMenu(AZ::Crc32 menuId, QMenu* menu) override;
        void AddMenuEntry(AZ::Crc32 menuId, AZ::Crc32 entryId, const QString& menuText, AZ::Crc32 hotkeyId, MenuSelectedType callback) override;
        void AddMenuItem(QMenu* menuItem, AZ::Crc32 entryId, const QString& menuText, AZ::Crc32 hotkeyId, MenuSelectedType callback);

        struct MenuEntry
        {
            QString m_menuText;
            MenuSelectedType m_callback;
            AZ::Crc32 m_hotkeyId;
        };

        AZStd::unordered_map<AZ::Crc32, QMenu*> m_registeredMenus;
        AZStd::unordered_map<AZ::Crc32, AZStd::unordered_map<AZ::Crc32, MenuEntry> > m_customMenusEntries;
    };
}

namespace LegacyFramework
{
    void CustomMenusComponent::Activate()
    {
        CustomMenusMessages::Bus::Handler::BusConnect();

        LegacyFramework::CustomMenusRegistration::Bus::Broadcast(
            &LegacyFramework::CustomMenusRegistration::Bus::Events::RegisterMenuEntries);
    }

    void CustomMenusComponent::Deactivate()
    {
        CustomMenusMessages::Bus::Handler::BusDisconnect();
    }

    void CustomMenusComponent::AddMenuItem(QMenu* menuItem, AZ::Crc32 entryId, const QString& menuText, AZ::Crc32 hotkeyId, MenuSelectedType callback)
    {
        auto action = new QAction(menuText, menuItem);
        menuItem->addAction(action);
        QObject::connect(action, &QAction::triggered, action, [entryId, callback](bool)
            {
                callback(entryId);
            });

        if (hotkeyId != AZ::Crc32())
        {
            AzToolsFramework::FrameworkMessages::Bus::Broadcast(
                &AzToolsFramework::FrameworkMessages::Bus::Events::RegisterActionToHotkey, hotkeyId, action);
        }
    }

    void CustomMenusComponent::RegisterMenu(AZ::Crc32 menuId, QMenu* menu)
    {
        AZ_Assert(menu, "tried to register a nullptr as a menu");

        if (!menu)
        {
            return;
        }

        m_registeredMenus[menuId] = menu;

        auto menuEntries = m_customMenusEntries.find(menuId);
        if (menuEntries != m_customMenusEntries.end())
        {
            for (const auto& menuEntry : menuEntries->second)
            {
                AddMenuItem(menu, menuEntry.first, menuEntry.second.m_menuText, menuEntry.second.m_hotkeyId, menuEntry.second.m_callback);
            }
        }
    }

    void CustomMenusComponent::AddMenuEntry(AZ::Crc32 menuId, AZ::Crc32 entryId, const QString& menuText, AZ::Crc32 hotkeyId, MenuSelectedType callback)
    {
        auto registeredMenu = m_registeredMenus.find(menuId);
        if (registeredMenu != m_registeredMenus.end())
        {
            AddMenuItem(registeredMenu->second, entryId, menuText, hotkeyId, callback);
        }
        auto& menuEntries = m_customMenusEntries[menuId];
        menuEntries[entryId] = MenuEntry {
            menuText, callback, hotkeyId
        };
    }

    void CustomMenusComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context);
        if (serialize)
        {
            serialize->Class<CustomMenusComponent, AZ::Component>()
                ->Version(1)
            ;
        }
    }
}
