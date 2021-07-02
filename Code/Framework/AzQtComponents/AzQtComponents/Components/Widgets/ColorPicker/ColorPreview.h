/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzCore/Math/Color.h>
#include <QFrame>
#endif

namespace AzQtComponents
{
    class Swatch;

    class AZ_QT_COMPONENTS_API ColorPreview
        : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(AZ::Color currentColor READ currentColor WRITE setCurrentColor)
        Q_PROPERTY(AZ::Color selectedColor READ selectedColor WRITE setSelectedColor)

    public:
        explicit ColorPreview(QWidget* parent = nullptr);
        ~ColorPreview() override;

        AZ::Color currentColor() const;
        AZ::Color selectedColor() const;

    public Q_SLOTS:
        void setCurrentColor(const AZ::Color& color);
        void setSelectedColor(const AZ::Color& color);

    Q_SIGNALS:
        void colorSelected(const AZ::Color& color);
        void colorContextMenuRequested(const QPoint& pos, const AZ::Color& color);

    protected:
        QSize sizeHint() const override;
        void paintEvent(QPaintEvent* e) override;
        void mousePressEvent(QMouseEvent* e) override;
        void mouseReleaseEvent(QMouseEvent* e) override;
        void mouseMoveEvent(QMouseEvent* e) override;

    private:
        AZ::Color colorUnderPoint(const QPoint& p);

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        AZ::Color m_currentColor;
        AZ::Color m_selectedColor;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QPoint m_dragStartPosition;
        Swatch* m_draggedSwatch;
    };
} // namespace AzQtComponents
