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
