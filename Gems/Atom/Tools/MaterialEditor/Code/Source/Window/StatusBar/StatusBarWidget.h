/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option") // disable warnings spawned by QT
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace Ui
{
    class StatusBarWidget;
}

namespace MaterialEditor
{
    //! Status bar for MaterialEditor.
    class StatusBarWidget
        : public QWidget
    {
        Q_OBJECT
    public:
        StatusBarWidget(QWidget* parent = nullptr);
        ~StatusBarWidget();

        void UpdateStatusInfo(const QString& status);
        void UpdateStatusWarning(const QString& status);
        void UpdateStatusError(const QString& status);

    private:
        QScopedPointer<Ui::StatusBarWidget> m_ui;
    };
} // namespace MaterialEditor
