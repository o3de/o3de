/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/StyledDialog.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>
#include <QDebug>
#include <QResizeEvent>

namespace AzQtComponents
{
    StyledDialog::StyledDialog(QWidget* parent, Qt::WindowFlags f)
        : QDialog(new WindowDecorationWrapper(WindowDecorationWrapper::OptionAutoAttach |  WindowDecorationWrapper::OptionAutoTitleBarButtons, parent), f)
    {
    }

    void StyledDialog::enableSaveRestoreGeometry(const QString& key)
    {
        auto windowDecorator = qobject_cast<WindowDecorationWrapper*>(parent());
        if (windowDecorator != nullptr)
        {
            windowDecorator->enableSaveRestoreGeometry(key);
        }
    }

    bool StyledDialog::restoreGeometryFromSettings()
    {
        auto windowDecorator = qobject_cast<WindowDecorationWrapper*>(parent());
        if (windowDecorator != nullptr)
        {
            return windowDecorator->restoreGeometryFromSettings();
        }

        return false;
    }

} // namespace AzQtComponents

#include "Components/moc_StyledDialog.cpp"
