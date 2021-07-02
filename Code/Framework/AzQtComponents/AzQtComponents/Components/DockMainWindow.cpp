/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <QVariant>

#include <AzQtComponents/Components/DockMainWindow.h>


namespace AzQtComponents
{
    static const char* FancyDockingOwnerPropertyName = "fancydocking_owner";

    /**
     * Create a dock main window that extends the QMainWindow so we can construct
     * our own custom context popup menu
     */
    DockMainWindow::DockMainWindow(QWidget* parent, Qt::WindowFlags flags)
        : QMainWindow(parent, flags)
    {
        setCursor(Qt::ArrowCursor);
    }

    /**
     * Override of QMainWindow::createPopupMenu to not show any context menu when
     * right-clicking on the space between our dock widgets
     */
    QMenu* DockMainWindow::createPopupMenu()
    {
        return nullptr;
    }

    void DockMainWindow::SetFancyDockingOwner(QWidget* instance)
    {
        setProperty(FancyDockingOwnerPropertyName, QVariant::fromValue(instance));
    }

    bool DockMainWindow::HasFancyDocking()
    {
        return property(FancyDockingOwnerPropertyName).isValid();
    }

} // namespace AzQtComponents

#include "Components/moc_DockMainWindow.cpp"
