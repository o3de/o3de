/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

class QMenuBar;
class QWidget;

namespace AtomToolsFramework
{
    //! AtomToolsMainWindowRequestBus provides an interface to common main application window functions like adding docked windows,
    //! resizing the viewport, and other operations 
    class AtomToolsMainWindowRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        //! Bring main window to foreground
        virtual void ActivateWindow() = 0;

        //! Add dockable widget in main window
        //! @param name title of the dockable window
        //! @param widget docked window content
        //! @param area location of docked window corresponding to Qt::DockWidgetArea
        virtual bool AddDockWidget(const AZStd::string& name, QWidget* widget, uint32_t area) = 0;

        //! Destroy dockable widget in main window
        //! @param name title of the dockable window
        virtual void RemoveDockWidget(const AZStd::string& name) = 0;

        //! Show or hide dockable widget in main window
        //! @param name title of the dockable window
        virtual void SetDockWidgetVisible(const AZStd::string& name, bool visible) = 0;

        //! Determine visibility of dockable widget in main window
        //! @param name title of the dockable window
        virtual bool IsDockWidgetVisible(const AZStd::string& name) const = 0;

        //! Get a list of registered docked widget names
        virtual AZStd::vector<AZStd::string> GetDockWidgetNames() const = 0;

        //! Prepare to update menus
        virtual void QueueUpdateMenus(bool rebuildMenus) = 0;

        //! Resizes the main window to achieve a requested size for the viewport render target.
        //! (This indicates the size of the render target, not the desktop-scaled QT widget size).
        virtual void ResizeViewportRenderTarget([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height){};

        //! Forces the viewport's render target to use the given resolution, ignoring the size of the viewport widget.
        virtual void LockViewportRenderTargetSize([[maybe_unused]] uint32_t width, [[maybe_unused]] uint32_t height){};

        //! Releases the viewport's render target resolution lock, allowing it to match the viewport widget again.
        virtual void UnlockViewportRenderTargetSize(){};
    };

    using AtomToolsMainWindowRequestBus = AZ::EBus<AtomToolsMainWindowRequests>;

    class AtomToolsMainMenuRequests : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::MultipleAndOrdered;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Crc32 BusIdType;

        struct BusHandlerOrderCompare
        {
            bool operator()(AtomToolsMainMenuRequests* left, AtomToolsMainMenuRequests* right) const
            {
                return left->GetMainMenuPriority() < right->GetMainMenuPriority();
            }
        };

        //! Determines the order in which the implementing classes modify the main menu
        virtual AZ::s32 GetMainMenuPriority() const { return 0; };

        //! Close menu bar of all actions then recreates and repopulates menus
        virtual void CreateMenus(QMenuBar* menuBar) = 0;

        //! Update menu state based on the application state
        virtual void UpdateMenus(QMenuBar* menuBar) = 0;
    };

    using AtomToolsMainMenuRequestBus = AZ::EBus<AtomToolsMainMenuRequests>;
} // namespace AtomToolsFramework
