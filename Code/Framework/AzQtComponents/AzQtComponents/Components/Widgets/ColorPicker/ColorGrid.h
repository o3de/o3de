/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <QFrame>
#endif

namespace AzQtComponents
{
    class AZ_QT_COMPONENTS_API ColorGrid
        : public QFrame
    {
        Q_OBJECT
        Q_PROPERTY(qreal hue READ hue WRITE setHue)
        Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation)
        Q_PROPERTY(qreal value READ value WRITE setValue)

    public:
        explicit ColorGrid(QWidget* parent = nullptr);
        ~ColorGrid() override;

        qreal hue() const;
        qreal saturation() const;
        qreal value() const;
        qreal defaultVForHsMode() const;

        enum class Mode
        {
            SaturationValue,
            HueSaturation,
        };

        void setMode(Mode mode);
        Mode mode() const;

        void StopSelection();

    public Q_SLOTS:
        void setHue(qreal hue);
        void setSaturation(qreal saturation);
        void setValue(qreal value);
        void setDefaultVForHsMode(qreal value);

    Q_SIGNALS:
        void gridPressed();
        void hsvChanged(qreal hue, qreal saturation, qreal value);
        void gridReleased();

    protected:
        void paintEvent(QPaintEvent* e) override;
        void mouseMoveEvent(QMouseEvent* e) override;
        void mousePressEvent(QMouseEvent* e) override;
        void mouseReleaseEvent(QMouseEvent* e) override;
        void resizeEvent(QResizeEvent* e) override;

    private:
        void initPixmap();
        void handleLeftButtonEvent(const QPoint& p);

        struct HSV
        {
            qreal hue;
            qreal saturation;
            qreal value;
        };
        HSV positionToColor(const QPoint& pos) const;

        QPoint cursorCenter() const;

        Mode m_mode;

        qreal m_hue;
        qreal m_saturation;
        qreal m_value;
        qreal m_defaultVForHsMode;

        bool m_userIsSelecting = false;

        QPixmap m_pixmap;
    };
} // namespace AzQtComponents
