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

#include <QColor>
#include <QPoint>
#include <QRect>
#include <QSlider>
#include <QStyle>
#endif
class QSettings;
class QStyleOptionSlider;

namespace AzQtComponents
{
    class Style;

    //! Extends the slider component to add extra styling and behavior options.
    class CustomSlider
        : public QSlider
    {
        Q_OBJECT

    public:
        explicit CustomSlider(Qt::Orientation orientation, QWidget* parent);

        //! Initialize option with the values from this Slider.
        void initStyleOption(QStyleOptionSlider& option);

    protected:
        void mousePressEvent(QMouseEvent* ev) override;
        void mouseReleaseEvent(QMouseEvent* ev) override;
        void wheelEvent(QWheelEvent* ev) override;

    Q_SIGNALS:
        //! Triggered when the slider starts or stops being moved by the user.
        //! @param moving True if the user just started holding the slider handle, false if it was released.
        void moveSlider(bool moving);
    };

    //! Base class for Slider components. Wraps a CustomSlider to provide additional styling options and functionality.
    class AZ_QT_COMPONENTS_API Slider
        : public QWidget
    {
        Q_OBJECT

        //! Orientation of the slider. Horizontal sliders increase left to right, vertical ones are bottom to top.
        Q_PROPERTY(Qt::Orientation orientation READ orientation WRITE setOrientation)
        //! Horizontal offset for the tooltip displaying the value on hover, in pixels.
        Q_PROPERTY(int toolTipOffsetX READ toolTipOffsetX WRITE setToolTipOffsetX)
        //! Vertical offset for the tooltip displaying the value on hover, in pixels.
        Q_PROPERTY(int toolTipOffsetY READ toolTipOffsetY WRITE setToolTipOffsetY)

    public:
        //! Style configuration for slider borders.
        struct Border
        {
            int thickness;                          //!< Border thickness, in pixels.
            QColor color;                           //!< Border color.
            qreal radius;                           //!< Border radius, in pixels.
        };

        //! Style configuration for gradient slider controls.
        struct GradientSliderConfig
        {
            int thickness;                          //!< Slider thickness, in pixels.
            int length;                             //!< Slider length, in pixels.
            Border grooveBorder;                    //!< Styling settings for the slider groove border.
            Border handleBorder;                    //!< Styling settings for the slider handle border.
        };

        //! Style configuration for slider controls.
        struct SliderConfig
        {
            //! Style configuration for slider handles.
            struct HandleConfig
            {
                QColor color;                       //!< Color for the handle of an active slider.
                QColor colorDisabled;               //!< Color for the handle of a disabled slider.
                int size;                           //!< Size of the handle draggable area square, in pixels.
                int sizeMinusMargin;                //!< Diameter of the handle circle when not hovered, in pixels.
                int hoverSize;                      //!< Diameter of the handle circle when hovered, in pixels.
            };

            //! Style configuration for slider grooves.
            struct GrooveConfig
            {
                QColor color;                       //!< Groove color.
                QColor colorHovered;                //!< Hovered groove color.
                int width;                          //!< Groove width, in pixels.
                int midMarkerHeight;                //!< Height of the midpoint marker, in pixels.
            };

            HandleConfig handle;                    //!< Handle style configuration.
            GrooveConfig grove;                     //!< Groove style configuration.
        };

        //! Style configuration for the Slider class.
        struct Config
        {
            GradientSliderConfig gradientSlider;    //!< Style configuration for Gradient Sliders.
            SliderConfig slider;                    //!< Style configuration for Sliders.
            QPoint horizontalToolTipOffset;         //!< Default tooltip offset for horizontally oriented sliders.
            QPoint verticalToolTipOffset;           //!< Default tooltip offset for vertically oriented sliders.
        };

        Slider(QWidget* parent = nullptr);
        Slider(Qt::Orientation orientation, QWidget* parent = nullptr);

        //! Sets the slider orientation.
        void setOrientation(Qt::Orientation orientation);
        //! Gets the slider orientation.
        Qt::Orientation orientation() const;

        //! Sets a custom tooltip offset for this Slider.
        void setToolTipOffset(const QPoint& toolTipOffset) { m_toolTipOffset = toolTipOffset; }
        //! Gets the tooltip offset for this Slider.
        QPoint toolTipOffset() const { return m_toolTipOffset; }

        //! Sets the X component for the tooltip offset for this Slider.
        void setToolTipOffsetX(int toolTipOffsetX) { m_toolTipOffset.setX(toolTipOffsetX); }
        //! Gets the X component of the tooltip offset for this Slider.
        int toolTipOffsetX() const { return m_toolTipOffset.x(); }

        //! Sets the Y component for the tooltip offset for this Slider.
        void setToolTipOffsetY(int toolTipOffsetY) { m_toolTipOffset.setY(toolTipOffsetY); }
        //! Gets the Y component of the tooltip offset for this Slider.
        int toolTipOffsetY() const { return m_toolTipOffset.y(); }

        //! Apply custom formatting to the hover tooltip.
        //! @param prefix Text to prefix to the slider value being hovered (for example, "$10").
        //! @param postFix Text to postfix to the slider value being hovered (for example, "10%").
        void setToolTipFormatting(const QString& prefix, const QString& postFix);

        //! Shows a hover tooltip with the right positioning on top of a slider
        static void showHoverToolTip(const QString& toolTipText, const QPoint& globalPosition, QSlider* slider, QWidget* toolTipParentWidget, int width, int height, const QPoint& toolTipOffset);

        //! Returns the slider value at the position specified.
        static int valueFromPosition(QSlider* slider, const QPoint& pos, int width, int height, int bottom);

        //! Applies the "Midpoint" styling to a Slider.
        //! "Midpoint" styling shows a tick halfway through the slider, and has the hover lines originate from it.
        static void applyMidPointStyle(Slider* slider);

        //! Initializes the static variables used by the Slider class.
        //! @param verticalToolTipOffset Offset for the hover tooltip on sliders with the vertical orientation.
        //! @param horizontalToolTipOffset Offset for the hover tooltip on sliders with the horizontal orientation.
        static void initStaticVars(const QPoint& verticalToolTipOffset, const QPoint& horizontalToolTipOffset);

        //! Sets the Slider style configuration.
        //! @param settings The settings object to load the configuration from.
        //! @return The new configuration of the Slider.
        static Config loadConfig(QSettings& settings);
        //! Gets the default Slider style configuration.
        static Config defaultConfig();

        //! Sets the flag that determines whether the handle is being dragged, stopping event detection.
        void sliderIsInMoving(bool b);

        //! Returns whether the handle is being dragged.
        bool IsSliderBeingMoved() const;

        //! Returns the recommended minimum size of the underlying QSlider.
        QSize minimumSizeHint() const override;
        //! Returns the recommended size of the underlying QSlider.
        QSize sizeHint() const override;

        //! Exposes the setFocusProxy function from the underlying Slider.
        void setFocusProxy(QWidget* proxy);
        //! Exposes the focusProxy function from the underlying Slider.
        QWidget* focusProxy() const;

        //! Exposes the setTracking function from the underlying Slider.
        void setTracking(bool enable);
        //! Exposes the hasTracking function from the underlying Slider.
        bool hasTracking() const;

    Q_SIGNALS:
        //! Triggered when the user presses the slider handle.
        void sliderPressed();
        //! Triggered when the slider handle position changes via dragging.
        void sliderMoved(int position);
        //! Triggered when the user releases the slider handle.
        void sliderReleased();
        //! Exposes the actionTriggered signal from the underlying QSlider.
        //! This signal is triggered when a slider action is executed. Actions are SliderSingleStepAdd, SliderSingleStepSub,
        //! SliderPageStepAdd, SliderPageStepSub, SliderToMinimum, SliderToMaximum, and SliderMove.
        void actionTriggered(int action);

    protected:
        virtual QString hoverValueText(int sliderValue) const = 0;

        bool eventFilter(QObject* watched, QEvent* event) override;

        void ShowValueToolTip(int sliderPos);

        CustomSlider* m_slider = nullptr;
        QString m_toolTipPrefix;
        QString m_toolTipPostfix;

    private:
        friend class Style;

        // methods used by Style
        static int sliderThickness(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);
        static int sliderLength(const Style* style, const QStyleOption* option, const QWidget* widget, const Config& config);

        static QRect sliderHandleRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config, bool isHovered = true);
        static QRect sliderGrooveRect(const Style* style, const QStyleOptionSlider* option, const QWidget* widget, const Config& config);

        static bool polish(Style* style, QWidget* widget, const Config& config);
        static bool drawSlider(const Style* style, const QStyleOption* option, QPainter* painter, const QWidget* widget, const Config& config);

        static void drawMidPointMarker(QPainter* painter, const Config& config, Qt::Orientation orientation, const QRect& grooveRect, const QColor& midMarkerColor);
        static void drawMidPointGrooveHighlights(QPainter* painter, const Config& config, const QRect& grooveRect, const QRect& handleRect, const QPoint& mousePos, Qt::Orientation orientation);
        static void drawGrooveHighlights(QPainter* painter, const Slider::Config& config, const QRect& grooveRect, const QRect& handleRect, const QPoint& mousePos, Qt::Orientation orientation);
        static void drawHandle(const QStyleOption* option, QPainter* painter, const Config& config, const QRect& handleRect, const QColor& handleColor);

        static int styleHintAbsoluteSetButtons();

        int valueFromPos(const QPoint &pos) const;

        QRect getVisibleGrooveRect() const;
        int getVisibleGrooveLength() const;
        int getVisibleGrooveStart() const;
        int getDistanceFromVisibleGrooveStart(const QPoint& point) const;
        bool isPointInVisibleGrooveArea(const QPoint& point) const;

        QPoint m_mousePos;

        QPoint m_toolTipOffset;
        bool m_moveSlider = false;
    };

    //! Control to display a slider for integer value input.
    class AZ_QT_COMPONENTS_API SliderInt
        : public Slider
    {
        Q_OBJECT

        //! Minimum value. Will be to the left for horizontal sliders, or at the bottom for vertical ones.
        Q_PROPERTY(int minimum READ minimum WRITE setMinimum)
        //! Maximum value. Will be to the right for horizontal sliders, or at the top for vertical ones.
        Q_PROPERTY(int maximum READ maximum WRITE setMaximum)
        //! The current value of the slider.
        Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true)

    public:
        SliderInt(QWidget* parent = nullptr);
        SliderInt(Qt::Orientation orientation, QWidget* parent = nullptr);

        //! Sets the current value.
        void setValue(int value);
        //! Gets the current value.
        int value() const;

        //! Sets the minimum value selectable with this slider.
        void setMinimum(int min);
        //! Gets the minimum value selectable with this slider.
        int minimum() const;

        //! Sets the maximum value selectable with this slider.
        void setMaximum(int max);
        //! Gets the maximum value selectable with this slider.
        int maximum() const;

        //! Sets the minimum and maximum values.
        void setRange(int min, int max);

    Q_SIGNALS:
        //! Triggered when the slider's value has changed.
        //! The tracking setting determines whether this signal is caused by user interaction.
        void valueChanged(int value);
        //! Triggered when the minimum or maximum values are changed.
        void rangeChanged(int min, int max);

    protected:
        QString hoverValueText(int sliderValue) const override;
    };

    //! Control to display a slider for double value input.
    class AZ_QT_COMPONENTS_API SliderDouble
        : public Slider
    {
        Q_OBJECT

        //! Minimum value. Will be to the left in horizontal sliders, or at the bottom in vertical ones.
        Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
        //! Maximum value. Will be to the right in horizontal sliders, or at the top in vertical ones.
        Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
        //! The number of steps used when changing the value via the arrow keys.
        Q_PROPERTY(int numSteps READ numSteps WRITE setNumSteps)
        //! The current value for the slider.
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged USER true)
        //! The decimal precision the value is returned with.
        Q_PROPERTY(int decimals READ decimals WRITE setDecimals)
        //! An optional non-linear scale setting using a power curve.
        //! Defaults to 0.5, which is a linear curve. Lowering or raising the midpoint value
        //! will shift the scale to have higher precision at the lower or higher end, respectively.
        Q_PROPERTY(double curveMidpoint READ curveMidpoint WRITE setCurveMidpoint)

    public:
        SliderDouble(QWidget* parent = nullptr);
        SliderDouble(Qt::Orientation orientation, QWidget* parent = nullptr);

        //! Sets the current value.
        void setValue(double value);
        //! Gets the current value.
        double value() const;

        //! Sets the minimum value selectable with this slider.
        void setMinimum(double min);
        //! Gets the minimum value selectable with this slider.
        double minimum() const;

        //! Sets the maximum value selectable with this slider.
        void setMaximum(double max);
        //! Gets the maximum value selectable with this slider.
        double maximum() const;

        //! Sets the minimum and maximum values.
        void setRange(double min, double max, int numSteps = 100);

        //! Sets the number of steps, which are used when changing value via the arrow keys.
        void setNumSteps(int steps);
        //! Gets the number of steps.
        int numSteps() const;

        //! Sets the decimal precision the value is returned with.
        void setDecimals(int decimals) { m_decimals = decimals; }
        //! Gets the decimal precision the value is returned with.
        int decimals() const { return m_decimals; }

        //! Sets a non-linear scale power curve.
        //! Defaults to 0.5, which is a linear curve. Lowering or raising the midpoint value
        //! will shift the scale to have higher precision at the lower or higher end, respectively.
        void setCurveMidpoint(double midpoint);
        //! Gets the current curve midpoint setting.
        double curveMidpoint() const { return m_curveMidpoint; }

    Q_SIGNALS:
        //! Triggered when the slider's value has been changed.
        //! The tracking setting determines whether this signal is emitted during user interaction.
        void valueChanged(double value);
        //! Triggered when the minimum, maximum or steps values are changed.
        void rangeChanged(double min, double max, int numSteps);

    protected:
        QString hoverValueText(int sliderValue) const override;

        double calculateRealSliderValue(int value) const;

        double convertToSliderValue(double value) const;
        double convertFromSliderValue(double value) const;
        double convertPowerCurveValue(double value, bool fromSlider) const;

    private:
        double m_minimum = 0.0;
        double m_maximum = 1.0;
        int m_numSteps = 100;
        int m_decimals;
        double m_curveMidpoint = 0.5;
    };

} // namespace AzQtComponents
