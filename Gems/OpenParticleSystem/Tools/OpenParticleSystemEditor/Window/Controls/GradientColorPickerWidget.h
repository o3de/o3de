/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <QWidgetAction>
#include <QGradientStops>
#include <QPolygon>
#include <QtMath>
#include <QMenu>
#include <QWidget>
#include <QGridLayout>
#include <QPropertyAnimation>
#if !defined(Q_MOC_RUN)
#include <AzCore/std/function/function_template.h>
#endif

namespace OpenParticleSystemEditor
{
    class GradientWidget;

    class GradientColorPickerWidget : public QWidget
    {
        Q_OBJECT
    public:
        GradientColorPickerWidget(QWidget* parent);
        ~GradientColorPickerWidget();

        unsigned int AddKey(float stop, QColor color);
        void RemoveKey(const QGradientStop& stop);
        void SetKeys(QGradientStops stops);
        QGradientStops GetKeys();
        QColor GetColorByTime(float x);

        virtual void paintEvent(QPaintEvent* event);
        void SetGradientChangedCB(AZStd::function<void(QGradientStops)> callback);
        void SetLocationChangedCB(AZStd::function<void(short, QColor)> callback);
        void SetDefaultLocationChangedCB(AZStd::function<void()> callback);
        void SetSelectedKeyPosition(int location);
        void AddKeyEnabled(bool enabled) { m_addKey = enabled; };

    protected:
        virtual void mouseMoveEvent(QMouseEvent* mouseEvent);
        virtual void mousePressEvent(QMouseEvent* mouseEvent);
        virtual void mouseReleaseEvent(QMouseEvent* mouseEvent);
        virtual void mouseDoubleClickEvent(QMouseEvent* mouseEvent);
        virtual void resizeEvent(QResizeEvent* resizeEvent);
        virtual void leaveEvent(QEvent* event);

        void ShowGradientMenu(int x);
        QColor GetHueAt(float x);

        AZStd::function<void(QGradientStops)> m_gradientChangedCB;
        AZStd::function<void(short, QColor)> m_locationChangedCB;
        AZStd::function<void()> m_defaultLocationChangedCB;

        enum class Gradients
        {
            ALPHA_RANGE = 0,
            HUE_RANGE,
            NUM_GRADIENTS
        };

        //////////////////////////////////////////////////////////////////////////
        // Gradient Key
        struct GradientKey
        {
            GradientKey();
            GradientKey(const GradientKey& other);
            GradientKey& operator=(const GradientKey& key)
            {
                if (&key == this)
                {
                    return *this;
                }
                m_rect = key.m_rect;
                m_size = key.m_size;
                m_stop = key.m_stop;
                m_selected = key.m_selected;
                m_hovered = key.m_hovered;
                return *this;
            }
            GradientKey(QGradientStop stop, QRect rect, QSize size = QSize(12, 12));
            bool PointInKey(const QPoint& p);
            void Paint(QPainter& painter, const QColor& inactive, const QColor& hovered, const QColor& selected);
            void SetRegion(QRect rect);

            QGradientStop GetStop() const { return m_stop; }
            void SetStop(QGradientStop val) { m_stop = val; }
            bool IsSelected() const { return m_selected; }
            void SetSelected(bool val) { m_selected = val; }
            bool IsHovered() const { return m_hovered; }
            void SetHovered(bool val) { m_hovered = val; }
            QPoint LocalPos();
            void SetTime(qreal val);
        protected:
            QPolygonF CreatePolygon();
            QRect m_rect;
            QSize m_size;
            QGradientStop m_stop;
            bool m_selected;
            bool m_hovered;
        };

        bool RestrictedKey() const;
        GradientKey* GetSelectedKey();
        void OnGradientChanged();
        void OnPickColor(GradientKey* key);
        QRect GetRegion();

        GradientWidget* m_gradientWidget = nullptr;
        QVector<GradientKey> m_keys;
        QGridLayout m_layout;
        QColor m_colorInactive;
        QColor m_colorHovered;
        QColor m_colorSelected;

        bool m_leftDown;
        bool m_rightDown;
        bool m_needsRepainting;
        const QSize m_gradientSize;
        const float m_percentage = 100.0f;
        bool m_addKey;
    };
} // namespace OpenParticleSystemEditor
