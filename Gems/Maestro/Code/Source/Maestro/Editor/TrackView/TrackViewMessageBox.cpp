/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TrackViewMessageBox.h"

#include <CryEdit.h>
#include <IConsole.h>
#include <ISystem.h>

QMessageBox::StandardButton TrackViewMessageBox::Information(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    if (!HasUserInteraction())
    {
        AZ_Info("TrackViewMessageBox", "[%s] %s", title.toStdString().c_str(), text.toStdString().c_str());
        return defaultButton;
    }
    return QMessageBox::information(parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton TrackViewMessageBox::Question(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    if (!HasUserInteraction())
    {
        AZ_Info("TrackViewMessageBox", "[%s] %s", title.toStdString().c_str(), text.toStdString().c_str());
        return defaultButton;
    }
    return QMessageBox::question(parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton TrackViewMessageBox::Warning(
        QWidget* parent, const QString& title,
        const QString& text,
        QMessageBox::StandardButtons buttons,
        QMessageBox::StandardButton defaultButton)
{
    if (!HasUserInteraction())
    {
        AZ_Warning("TrackViewMessageBox", false, "[%s] %s", title.toStdString().c_str(), text.toStdString().c_str());
        return defaultButton;
    }
    return QMessageBox::warning(parent, title, text, buttons, defaultButton);
}

QMessageBox::StandardButton TrackViewMessageBox::Critical(
    QWidget* parent,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButtons buttons,
    QMessageBox::StandardButton defaultButton)
{
    if (!HasUserInteraction())
    {
        AZ_Error("TrackViewMessageBox", false, "[%s] %s", title.toStdString().c_str(), text.toStdString().c_str())
        return defaultButton;
    }
    return QMessageBox::critical(parent, title, text, buttons, defaultButton);
}

bool TrackViewMessageBox::HasUserInteraction()
{
    if (gEnv && gEnv->pConsole)
    {
        ICVar* pCVar = gEnv->pConsole->GetCVar("sys_no_crash_dialog");
        if (pCVar && pCVar->GetIVal() != 0)
        {
            return false;
        }
    }

    return CCryEditApp::instance()->IsInRegularEditorMode();
}
