/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzQtComponents/AzQtComponentsAPI.h>

#include <QDockWidget>
#include <QPoint>

namespace AzQtComponents
{
    class TitleBar;

    class AZ_QT_COMPONENTS_API StyledDockWidget
        : public QDockWidget
    {
        Q_OBJECT

    public:
        explicit StyledDockWidget(QWidget* parent = nullptr);
        explicit StyledDockWidget(const QString& name, QWidget* parent = nullptr);
        ~StyledDockWidget();

        static void drawFrame(QPainter& p, QRect rect, bool drawTop = true);
        void createCustomTitleBar();
        TitleBar* customTitleBar() const;
        bool isSingleFloatingChild();

    Q_SIGNALS:
        void undock();
        void aboutToClose();

    protected:
        void closeEvent(QCloseEvent* event) override;
        bool nativeEvent(const QByteArray& eventType, void* message, qintptr* result) override;
        bool event(QEvent* event) override;
        void showEvent(QShowEvent* event) override;
        void paintEvent(QPaintEvent*) override;

    private:
        void onFloatingChanged(bool floating);
        void init();
    };
} // namespace AzQtComponents
