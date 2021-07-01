/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFramework_precompiled.h"
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/LegacyFramework/UIFrameworkAPI.h>
#include <QtWidgets/qaction.h>
#include "CustomMenusAPI.h"

#include <QMenu>

namespace LegacyFramework
{
    namespace CustomMenusCommon
    {
        const AZ::Crc32 WorldEditor::Application = AZ_CRC_CE("World Editor");
        const AZ::Crc32 WorldEditor::File = AZ_CRC("World Editor - File", 0x4b085508);
        const AZ::Crc32 WorldEditor::Debug = AZ_CRC("World Editor - Debug", 0x7f0e4892);
        const AZ::Crc32 WorldEditor::Edit = AZ_CRC("World Editor - Edit", 0x46a2bd02);
        const AZ::Crc32 WorldEditor::Build = AZ_CRC("World Editor - Build", 0xae0bfdee);

        const AZ::Crc32 Driller::Application = AZ_CRC_CE("Driller");
        const AZ::Crc32 Driller::DrillerMenu = AZ_CRC("Driller - File", 0x5a98bdd8);
        const AZ::Crc32 Driller::Channels = AZ_CRC("Driller - Debug", 0xf9cc0aae);

        const AZ::Crc32 LUAEditor::Application = AZ_CRC_CE("LUAEditor");
        const AZ::Crc32 LUAEditor::File = AZ_CRC("LUAEditor - File", 0xcf589de3);
        const AZ::Crc32 LUAEditor::Edit = AZ_CRC("LUAEditor - Edit", 0xc2f275e9);
        const AZ::Crc32 LUAEditor::View = AZ_CRC("LUAEditor - View", 0xbd3a007d);
        const AZ::Crc32 LUAEditor::Debug = AZ_CRC("LUAEditor - Debug", 0x485223aa);
        const AZ::Crc32 LUAEditor::SourceControl = AZ_CRC("LUAEditor - SourceControl", 0x7a3d336c);
        const AZ::Crc32 LUAEditor::Options = AZ_CRC("LUAEditor - Options", 0x2f44057c);

        const AZ::Crc32 Viewport::Layout = AZ_CRC("Viewport - Layout", 0x0d57aea6);
        const AZ::Crc32 Viewport::Grid = AZ_CRC("Viewport - Grid", 0x005b038a);
        const AZ::Crc32 Viewport::View = AZ_CRC("Viewport - View", 0xd0867133);
    }

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

        EBUS_EVENT(LegacyFramework::CustomMenusRegistration::Bus, RegisterMenuEntries);
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
            EBUS_EVENT(AzToolsFramework::FrameworkMessages::Bus, RegisterActionToHotkey, hotkeyId, action);
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
