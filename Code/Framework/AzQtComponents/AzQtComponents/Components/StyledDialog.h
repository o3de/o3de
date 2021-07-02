/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>

// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QDialog>
AZ_POP_DISABLE_WARNING
#endif

namespace AzQtComponents
{
    class WindowDecorationWrapper;

    class AZ_QT_COMPONENTS_API StyledDialog
        : public QDialog
    {
        Q_OBJECT

    public:
        explicit StyledDialog(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

        void enableSaveRestoreGeometry(const QString& key);
        bool restoreGeometryFromSettings();
    };
} // namespace AzQtComponents

