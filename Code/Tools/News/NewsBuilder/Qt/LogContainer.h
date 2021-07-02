/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#include <NewsShared/LogType.h>
#endif

namespace Ui
{
    class LogContainerWidget;
}

namespace News 
{
    //! A control for displaying log
    class LogContainer
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit LogContainer(QWidget* parent);
        ~LogContainer();

        void AddLog(QString text, LogType logType) const;

    private:
        QScopedPointer<Ui::LogContainerWidget> m_ui;
    };
} // namespace News
