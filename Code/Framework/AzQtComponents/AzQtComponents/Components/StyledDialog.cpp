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
