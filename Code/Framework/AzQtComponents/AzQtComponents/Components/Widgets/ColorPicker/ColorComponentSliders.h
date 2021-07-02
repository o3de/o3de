/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <AzQtComponents/AzQtComponentsAPI.h>
#include <AzQtComponents/Components/Widgets/GradientSlider.h>
#include <QWidget>
#endif

namespace AzQtComponents
{
    class SpinBox;

    class AZ_QT_COMPONENTS_API ColorComponentEdit
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)

    public:
        explicit ColorComponentEdit(const QString& label, int softMaximum, int hardMaximum, QWidget* parent = nullptr);
        ~ColorComponentEdit() override;

        void setColorFunction(GradientSlider::ColorFunction colorFunction);
        void setToolTipFunction(GradientSlider::ToolTipFunction toolTipFunction);

        qreal value() const;
        int decimals() const { return m_slider->decimals(); }
        int maximum() const { return m_slider->maximum(); }

        void setValue(qreal value);
        QColor colorAt(qreal position) const;

    public Q_SLOTS:
        void updateGradient();

    private Q_SLOTS:
        void spinValueChanged(int value);
        void sliderValueChanged(int value);

    Q_SIGNALS:
        void valueChangeBegan();
        void valueChanged(qreal value);
        void valueChangeEnded();

    private:
        qreal m_value;

        SpinBox* m_spinBox;
        GradientSlider* m_slider;
    };

    class AZ_QT_COMPONENTS_API HSLSliders
        : public QWidget
    {
        Q_OBJECT

        Q_PROPERTY(Mode mode READ mode WRITE setMode NOTIFY modeChanged)
        Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
        Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
        Q_PROPERTY(qreal lightness READ lightness WRITE setLightness NOTIFY lightnessChanged)

    public:
        enum class Mode
        {
            Hsl,
            Hs
        };
        Q_ENUM(Mode)

        explicit HSLSliders(QWidget* parent = nullptr);

        Mode mode() const;

        qreal hue() const;
        qreal saturation() const;
        qreal lightness() const;
        qreal defaultLForHsMode() const;

    public Q_SLOTS:
        void setMode(Mode mode);
        void setHue(qreal hue);
        void setSaturation(qreal saturation);
        void setLightness(qreal lightness);
        void setDefaultLForHsMode(qreal value);

    Q_SIGNALS:
        void valueChangeBegan();
        void modeChanged(Mode mode);
        void hueChanged(qreal hue);
        void saturationChanged(qreal saturation);
        void lightnessChanged(qreal lightness);
        void valueChangeEnded();

    private:
        Mode m_mode = Mode::Hsl;
        ColorComponentEdit* m_hueSlider;
        ColorComponentEdit* m_saturationSlider;
        ColorComponentEdit* m_lightnessSlider;

        qreal m_defaultLForHsMode;
    };

    class AZ_QT_COMPONENTS_API HSVSliders
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(qreal hue READ hue WRITE setHue NOTIFY hueChanged)
        Q_PROPERTY(qreal saturation READ saturation WRITE setSaturation NOTIFY saturationChanged)
        Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)

    public:
        enum class Mode
        {
            Hsv,
            Hs
        };
        Q_ENUM(Mode)

        explicit HSVSliders(QWidget* parent = nullptr);

        Mode mode() const;

        qreal hue() const;
        qreal saturation() const;
        qreal value() const;
        qreal defaultVForHsMode() const;

    public Q_SLOTS:
        void setMode(Mode mode);
        void setHue(qreal hue);
        void setSaturation(qreal saturation);
        void setValue(qreal value);
        void setDefaultVForHsMode(qreal value);

    Q_SIGNALS:
        void valueChangeBegan();
        void modeChanged(Mode mode);
        void hueChanged(qreal hue);
        void saturationChanged(qreal saturation);
        void valueChanged(qreal value);
        void valueChangeEnded();

    private:
        Mode m_mode = Mode::Hsv;
        ColorComponentEdit* m_hueSlider;
        ColorComponentEdit* m_saturationSlider;
        ColorComponentEdit* m_valueSlider;

        qreal m_defaultVForHsMode;
    };

    class AZ_QT_COMPONENTS_API RGBSliders
        : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(qreal red READ red WRITE setRed NOTIFY redChanged)
        Q_PROPERTY(qreal green READ green WRITE setGreen NOTIFY greenChanged)
        Q_PROPERTY(qreal blue READ blue WRITE setBlue NOTIFY blueChanged)

    public:
        explicit RGBSliders(QWidget* parent = nullptr);

        qreal red() const;
        qreal green() const;
        qreal blue() const;

    public Q_SLOTS:
        void setRed(qreal red);
        void setGreen(qreal green);
        void setBlue(qreal blue);

    Q_SIGNALS:
        void valueChangeBegan();
        void redChanged(qreal red);
        void greenChanged(qreal green);
        void blueChanged(qreal blue);
        void valueChangeEnded();

    private:
        ColorComponentEdit* m_redSlider;
        ColorComponentEdit* m_greenSlider;
        ColorComponentEdit* m_blueSlider;
    };
} // namespace AzQtComponents
