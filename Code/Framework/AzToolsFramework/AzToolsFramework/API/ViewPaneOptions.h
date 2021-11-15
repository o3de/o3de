/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>

#include <QRect>
#include <QKeySequence>
#include <QString>

namespace AzToolsFramework
{
    struct ViewPaneOptions
    {
        AZ_TYPE_INFO(ViewPaneOptions, "{E9FB803A-2A47-4BCF-8A50-AB4C9D73AED2}");

        QRect paneRect = QRect(50, 50, 1000, 800);                      ///< default size/position of the view pane, if no previous state is saved
        Qt::DockWidgetArea preferedDockingArea = Qt::NoDockWidgetArea;  ///< default docking area to place the view pane in, if no previous state is saved
        bool isDeletable = true;                                        ///< set to false if you want the view pane to hide on close, instead of being deleted
        bool isStandard = false;                                        ///< for internal use; leave set to false
        bool showInMenu = true;                                         ///< set to false if you'd like to register a view pane and have it NOT appear under the Tools menu
        bool canHaveMultipleInstances = false;                          ///< ignored; left for backwards code compatibility
        int viewportType = -1;                                          ///< for internal use; leave set to -1
        bool isPreview = false;                                         ///< indicates if a view pane is still pre-release
        QKeySequence shortcut;                                          ///< default shortcut to allow the user to open the view pane
        int builtInActionId = -1;                                       ///< for internal use; leave set to -1
        bool isDockable = true;                                         ///< set to false if the view pane should not be dockable; this can be necessary in certain cases, such as with QOpenGLWidgets
        QString optionalMenuText;                                       ///< set this to the text you'd like to appear under the Tools menu; leave it blank to use the view pane name under the Tools menu instead
        bool isLegacy = false;                                          ///< set this to true if you are marking this as a legacy (and likely to be deprecated) viewpane
        bool isLegacyReplacement = false;                               ///< set this to true if this is a viewpane to replace an older viewpane
        QString saveKeyName;                                            ///< can be zero length; set this if you want to use a name other than the viewpane name set in RegisterViewPane.

        bool detachedWindow = false;                                    ///< set to true if the view pane should use a detached, non-dockable widget. This is to workaround a problem with QOpenGLWidget on macOS. Currently this has no effect on other platforms.
        bool isDisabledInSimMode = false;                               ///< set to true if the view pane should not be openable from level editor menu when editor is in simulation mode.

        bool showOnToolsToolbar = false;                                ///< set to true if the view pane should create a button on the tools toolbar to open/close the pane
        AZStd::string toolbarIcon;                                      ///< path to the icon to use for the toolbar button - only used if showOnToolsToolbar is set to true
    };

} // namespace AzToolsFramework

// Left in for backwards compatibility, so that any code forward declaring
// QtViewOptions will continue to work.
struct QtViewOptions : public AzToolsFramework::ViewPaneOptions
{
};
