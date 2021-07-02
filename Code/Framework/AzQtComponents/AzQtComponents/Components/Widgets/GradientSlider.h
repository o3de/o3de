/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzCore/std/functional.h>
#include <QSlider>
#endif

namespace AzQtComponents
{
    class Style;

    class AZ_QT_COMPONENTS_API GradientSlider
        : public QSlider
    {
        Q_OBJECT
        Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
        Q_PROPERTY(int toolTipOffsetX READ toolTipOffsetX WRITE setToolTipOffsetX)
        Q_PROPERTY(int toolTipOffsetY READ toolTipOffsetY WRITE setToolTipOffsetY)

    public:
        using ColorFunction = AZStd::function<QColor(qreal)>;
        using ToolTipFunction = AZStd::function<QString(qreal)>;

        explicit GradientSlider(Qt::Orientation orientation, QWidget* parent = nullptr);
        explicit GradientSlider(QWidget* parent = nullptr);
        ~GradientSlider() override;

        void setColorFunction(ColorFunction colorFunction);
        void setToolTipFunction(ToolTipFunction toolTipFunction);
        void updateGradient();

        int decimals() const { return m_decimals; }
        void setDecimals(int decimals) { m_decimals = decimals; }

        QPoint toolTipOffset() const { return m_toolTipOffset; }
        void setToolTipOffset(const QPoint& toolTipOffset) { m_toolTipOffset = toolTipOffset; }

        int toolTipOffsetX() const { return m_toolTipOffset.x(); }
        void setToolTipOffsetX(int toolTipOffsetX) { m_toolTipOffset.setX(toolTipOffsetX); }

        int toolTipOffsetY() const { return m_toolTipOffset.y(); }
        void setToolTipOffsetY(int toolTipOffsetY) { m_toolTipOffset.setY(toolTipOffsetY); }

        bool GetIgnoreWheelEvents() const;
        void SetIgnoreWheelEvents(bool shouldIgnore);

        QColor colorAt(qreal position) const;

    protected:
        void resizeEvent(QResizeEvent* e) override;
        void keyPressEvent(QKeyEvent* e) override;
        void mouseMoveEvent(QMouseEvent* e) override;
        void wheelEvent(QWheelEvent* e) override;

    private:
        friend class Slider;

        static const char* s_gradientSliderClass;

        // methods called by Slider
        static int sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config);
        static int sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Slider::Config& config);
        static QRect sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config);
        static QRect sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Slider::Config& config);
        static bool drawSlider(const Style* style, const QStyleOptionSlider* option, QPainter* painter, const QWidget* widget, const Slider::Config& config);

        // internal methods
        void initGradientColors();
        QBrush gradientBrush() const;

        AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
        ColorFunction m_colorFunction;
        ToolTipFunction m_toolTipFunction;
        AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
        QLinearGradient m_gradient;
        QPoint m_toolTipOffset;
        int m_decimals = 7;
        bool m_ignoreWheelEvents = false;
    };
} // namespace AzQtComponents
