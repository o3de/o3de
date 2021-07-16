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
#include <QString>
#include <QVector>
#endif

QT_FORWARD_DECLARE_CLASS(QHBoxLayout)
QT_FORWARD_DECLARE_CLASS(QLabel)
QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QWidget)

namespace O3DE::ProjectManager
{
    class FilterTagWidget
        : public QFrame
    {
        Q_OBJECT

    public:
        FilterTagWidget(const QString& text, QWidget* parent = nullptr);
        QString text() const;

    signals:
        void RemoveClicked();

    private:
        QLabel* m_textLabel = nullptr;
        QPushButton* m_closeButton = nullptr;
    };

    // Horizontally expanding filter tag container widget
    class FilterTagWidgetContainer
        : public QWidget
    {
        Q_OBJECT

    public:
        FilterTagWidgetContainer(QWidget* parent = nullptr);
        void Reinit(const QVector<QString>& tags);

    signals:
        void TagRemoved(QString tagName);

    private:
        QWidget* m_widget = nullptr;
        QHBoxLayout* m_layout = nullptr;
    };
} // namespace O3DE::ProjectManager
