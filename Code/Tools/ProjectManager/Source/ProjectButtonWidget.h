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
#include <BigTallButtonWidget.h>

#include <QFrame>
#include <QLabel>
#endif

QT_FORWARD_DECLARE_CLASS(QResizeEvent)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPixmap)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QAction)

namespace O3DE::ProjectManager
{
    /*class ProjectImageLabel
        : public QLabel
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit ProjectImageLabel(QPixmap* imagesPixmap, QWidget* parent = nullptr);
        ~ProjectImageLabel();

    public slots:
        //void resizeEvent(QResizeEvent* event) override;

    private:
        void ResizeImage();

        QPixmap* m_imagePixmap;
    };*/

    class ProjectButton
        : public QFrame
    {
        Q_OBJECT // AUTOMOC

    public:
        explicit ProjectButton(const QString& projectName, QWidget* parent = nullptr);
        ~ProjectButton() = default;

    private:
        QString m_projectName;
        QLabel* m_projectImageLabel;
        QPushButton* m_projectSettingsMenuButton;
        QAction* m_editProjectAction;
        QAction* m_editProjectGemsAction;
        QAction* m_copyProjectAction;
        QAction* m_removeProjectAction;
        QAction* m_deleteProjectAction;
    };
} // namespace O3DE::ProjectManager
