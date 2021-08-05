/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QFrame>
#endif

QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)

namespace O3DE::ProjectManager
{
    class ScreenHeader 
        : public QFrame 
    {
        Q_OBJECT // AUTOMOC

    public:
        ScreenHeader(QWidget* parent = nullptr);

        void setTitle(const QString& text);
        void setSubTitle(const QString& text);

        QPushButton* backButton();

    private:
        QLabel* m_title;
        QLabel* m_subTitle;
        QPushButton* m_backButton;
    };
} // namespace O3DE::ProjectManager
