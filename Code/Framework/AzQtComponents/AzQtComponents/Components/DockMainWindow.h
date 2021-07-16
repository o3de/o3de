/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QMainWindow>
#endif


namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API DockMainWindow
        : public QMainWindow
    {
        Q_OBJECT
    public:
        explicit DockMainWindow(QWidget* parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
        QMenu* createPopupMenu() override;

        /// Set if this main window is owned by a fancy docking instance
        void SetFancyDockingOwner(QWidget* instance);
        /// Return whether or not this main window is configured with fancy docking
        bool HasFancyDocking();
    };
} // namespace AzQtComponents
