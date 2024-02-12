/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorValidator.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/Palette.h> // needed for Q_DECLARE_METATYPE(AZ::Color);
#include <AzQtComponents/Utilities/ColorUtilities.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Casting/numeric_cast.h>
#include <QSignalBlocker>
#include <QScopedValueRollback>
#include <QVector>

namespace AzQtComponents
{

    namespace Internal
    {
        // The internal ColorState class stores and calculates everything in doubles, for the highest accuracy
        // The ColorController returns floats, as those are easiest to work with and most GPUs have float
        // accuracy for color channels.
        class ColorController::ColorState
        {
        public:
            ColorState()
                : m_colorValidator(nullptr)
            {
            }

            ~ColorState() = default;

            ColorState& operator=(const ColorState&) = default;


            AZ::Color rgb() const
            {
                return AZ::Color(aznumeric_cast<float>(m_red), aznumeric_cast<float>(m_green), aznumeric_cast<float>(m_blue), aznumeric_cast<float>(m_alpha));
            }

            double red() const
            {
                return m_red;
            }

            double green() const
            {
                return m_green;
            }

            double blue() const
            {
                return m_blue;
            }

            double alpha() const
            {
                return m_alpha;
            }

            double hue() const
            {
                // This is a holdover from the QColor origins of ColorController (QColor expects hue to be in [0, 1.0] but we store degrees)
                return m_hue / 360.0;
            }

            double hslS() const
            {
                return m_hsl.saturation;
            }

            double hslL() const
            {
                return m_hsl.lightness;
            }

            double hsvS() const
            {
                return m_hsv.saturation;
            }

            double hsvV() const
            {
                return m_hsv.value;
            }

            void setRGB(const AZ::Color& color)
            {
                m_red = color.GetR();
                m_green = color.GetG();
                m_blue = color.GetB();
                m_alpha = color.GetA();
                propagateRgb();
            }

            void setRed(double r)
            {
                m_red = r;
                propagateRgb();
            }

            void setGreen(double g)
            {
                m_green = g;
                propagateRgb();
            }

            void setBlue(double b)
            {
                m_blue = b;
                propagateRgb();
            }

            void setAlpha(double a)
            {
                m_alpha = a;
                // alpha doesn't affect any of the other color spaces, so don't recalculate them
            }

            void setHslH(double h)
            {
                // This is a holdover from the QColor origins of ColorController (QColor expects hue to be in [0, 1.0] but we store degrees)
                m_hue = h * 360.0;
                propagateHsl();
            }

            void setHslS(double s)
            {
                m_hsl.saturation = s;
                propagateHsl();
            }

            void setHslL(double l)
            {
                m_hsl.lightness = l;
                propagateHsl();
            }

            void setHsl(double h, double s, double l)
            {
                m_hue = h * 360.0;
                m_hsl.saturation = s;
                m_hsl.lightness = l;
                propagateHsl();
            }

            void setHsvH(double h)
            {
                // This is a holdover from the QColor origins of ColorController (QColor expects hue to be in [0, 1.0] but we store degrees)
                m_hue = h * 360.0;
                propagateHsv();
            }

            void setHsvS(double s)
            {
                m_hsv.saturation = s;
                propagateHsv();
            }

            void setHsvV(double v)
            {
                m_hsv.value = v;
                propagateHsv();
            }

            void setHsv(double h, double s, double v)
            {
                m_hue = h * 360.0;
                m_hsv.saturation = s;
                m_hsv.value = v;
                propagateHsv();
            }

            void setValidator(ColorValidator* validator)
            {
                m_colorValidator = validator;
            }

            ColorValidator* validator() const
            {
                return m_colorValidator;
            }

        private:
            void propagateRgb()
            {
                // See https://en.wikipedia.org/wiki/HSL_and_HSV#General_approach through https://en.wikipedia.org/wiki/HSL_and_HSV#Lightness
                const double r = AZ::GetClamp(m_red, 0.0, 1.0);
                const double g = AZ::GetClamp(m_green, 0.0, 1.0);
                const double b = AZ::GetClamp(m_blue, 0.0, 1.0);

                // Compute Hue and Chroma
                const double M = std::max({ r, g, b });
                const double m = std::min({ r, g, b });
                const double C = M - m;

                double Hprime = 0.0f;
                if (qFuzzyIsNull(C))
                {
                    Hprime = 0.0;
                }
                else if (M == r)
                {
                    Hprime = std::fmod((g - b) / C, 6.0);
                }
                else if (M == g)
                {
                    Hprime = (b - r) / C + 2.0;
                }
                else if (M == b)
                {
                    Hprime = (r - g) / C + 4.0;
                }
                m_hue = 60.0 * Hprime;
                m_hue = AZ::GetClamp(m_hue, -360.0, 360.0);

                // if it's negative, shift it around the circle one more time to be positive
                if (m_hue < 0.0)
                {
                    m_hue += 360.0;
                }

                // Compute Lightness
                m_hsv.value = M;
                m_hsv.value = AZ::GetClamp(m_hsv.value, 0.0, 1.0);

                m_hsl.lightness = (M + m) / 2.0;
                m_hsl.lightness = AZ::GetClamp(m_hsl.lightness, 0.0, 12.5);

                // Compute saturation
                if (qFuzzyIsNull(m_hsv.value))
                {
                    m_hsv.saturation = 0.0;
                }
                else
                {
                    m_hsv.saturation = C / m_hsv.value;
                }
                m_hsv.saturation = AZ::GetClamp(m_hsv.saturation, 0.0, 1.0);

                if (qFuzzyIsNull(m_hsl.lightness))
                {
                    m_hsl.saturation = 0.0;
                }
                else if (m_hsl.lightness >= 1.0)
                {
                    m_hsl.saturation = 1.0;
                }
                else
                {
                    m_hsl.saturation = C / (1.0 - std::abs((m_hsl.lightness * 2.0) - 1.0));
                }
                m_hsl.saturation = AZ::GetClamp(m_hsl.saturation, 0.0, 1.0);
            }

            void propagateToRgb(double C, double m)
            {
                // See https://en.wikipedia.org/wiki/HSL_and_HSV#Converting_to_RGB

                double Hprime = std::fmod(m_hue, 360.0) / 60.0;
                double X = C * (1.0 - std::abs(std::fmod(Hprime, 2.0) - 1.0));
                if (Hprime <= 1.0)
                {
                    m_red = (C + m);
                    m_green = (X + m);
                    m_blue = (m);
                }
                else if (Hprime <= 2.0)
                {
                    m_red = (X + m);
                    m_green = (C + m);
                    m_blue = (m);
                }
                else if (Hprime <= 3.0)
                {
                    m_red = (m);
                    m_green = (C + m);
                    m_blue = (X + m);
                }
                else if (Hprime <= 4.0)
                {
                    m_red = (m);
                    m_green = (X + m);
                    m_blue = (C + m);
                }
                else if (Hprime <= 5.0)
                {
                    m_red = (X + m);
                    m_green = (m);
                    m_blue = (C + m);
                }
                else if (Hprime < 6.0)
                {
                    m_red = (C + m);
                    m_green = (m);
                    m_blue = (X + m);
                }
            }

            void propagateHsl()
            {
                // See https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSL
                double L = std::min(1.0, m_hsl.lightness);
                double C = (1.0 - std::abs(2.0 * L - 1.0)) * m_hsl.saturation;
                double m = L - (0.5 * C);
                propagateToRgb(C, m);


                // See http://home.kpn.nl/vanadovv/color/ColorMath.html#hdir (function HSV_HSL)
                double lightness = 2.0 * m_hsl.lightness;
                double saturation = m_hsl.saturation;
                if (lightness <= 1.0)
                {
                    saturation *= lightness;
                }
                else
                {
                    saturation *= 2.0 - lightness;
                }
                double value = (lightness + saturation) / 2.0;
                saturation = qFuzzyIsNull(lightness + saturation) ? 0 : (2.0 * saturation) / (lightness + saturation);

                m_hsv.saturation = AZ::GetClamp(saturation, 0.0, 1.0);
                m_hsv.value = AZ::GetClamp(value, 0.0, 12.5);
            }

            void propagateHsv()
            {
                // See https://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV
                double C = m_hsv.value * m_hsv.saturation;
                double m = m_hsv.value - C;
                propagateToRgb(C, m);

                // See http://home.kpn.nl/vanadovv/color/ColorMath.html#hdir (function HSL_HSV)
                double lightness = (2.0 - m_hsv.saturation) * m_hsv.value;
                double saturation = m_hsv.saturation * m_hsv.value;
                if (lightness <= 1.0)
                {
                    saturation = (qFuzzyIsNull(lightness)) ? 0.0 : saturation / lightness;
                }
                else
                {
                    double two_minus_lightness = 2.0 - lightness;
                    saturation = (qFuzzyIsNull(two_minus_lightness)) ? 0.0 : saturation / two_minus_lightness;
                }
                lightness /= 2.0;

                m_hsl.saturation = AZ::GetClamp(saturation, 0.0, 1.0);
                m_hsl.lightness = AZ::GetClamp(lightness, 0.0, 12.5);
            }

            double m_red = 0.0;
            double m_green = 0.0;
            double m_blue = 0.0;
            double m_alpha = 1.0;

            double m_hue = 0.0;
            struct
            {
                double saturation = 0.0;
                double lightness = 0.0;
            } m_hsl;

            struct
            {
                double saturation = 0.0;
                double value = 0.0;
            } m_hsv;

            ColorValidator* m_colorValidator;
        };

        ColorController::ColorController(QObject* parent)
            : QObject(parent)
            , m_state(new ColorState())
        {
        }

        ColorController::~ColorController()
        {
        }

        const AZ::Color ColorController::color() const
        {
            return m_state->rgb();
        }

        float ColorController::red() const
        {
            return aznumeric_caster(m_state->red());
        }

        float ColorController::green() const
        {
            return aznumeric_caster(m_state->green());
        }

        float ColorController::blue() const
        {
            return aznumeric_caster(m_state->blue());
        }

        float ColorController::hslHue() const
        {
            return aznumeric_caster(m_state->hue());
        }

        float ColorController::hslSaturation() const
        {
            return aznumeric_caster(m_state->hslS());
        }

        float ColorController::lightness() const
        {
            return aznumeric_caster(m_state->hslL());
        }

        float ColorController::hsvHue() const
        {
            return aznumeric_caster(m_state->hue());
        }

        float ColorController::hsvSaturation() const
        {
            return aznumeric_caster(m_state->hsvS());
        }

        float ColorController::value() const
        {
            return aznumeric_caster(m_state->hsvV());
        }

        float ColorController::alpha() const
        {
            return aznumeric_caster(m_state->alpha());
        }

        AZ::Color ColorController::fromHsl(qreal h, qreal s, qreal l)
        {
            ColorState state;
            state.setHsl(h, s, l);
            return state.rgb();
        }

        AZ::Color ColorController::fromHsv(qreal h, qreal s, qreal v)
        {
            ColorState state;
            state.setHsv(h, s, v);
            return state.rgb();
        }


        void ColorController::setValidator(ColorValidator* validator)
        {
            m_state->setValidator(validator);
        }

        ColorValidator* ColorController::validator() const
        {
            return m_state->validator();
        }

        void ColorController::setColor(const AZ::Color& color)
        {
            if (AreClose(m_state->rgb(), color))
            {
                return;
            }

            const ColorState previousState = *m_state;
            m_state->setRGB(color);

            validate();

            emitRgbaChangedSignals(previousState);
            emitHslChangedSignals(previousState);
            emitHsvChangedSignals(previousState);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setRed(float red)
        {
            if (qFuzzyCompare(red, this->red()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setRed(red);

            validate();

            emit redChanged(this->red());
            emitHslChangedSignals(previousColor);
            emitHsvChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setGreen(float green)
        {
            if (qFuzzyCompare(green, this->green()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setGreen(green);

            validate();

            emit greenChanged(this->green());
            emitHslChangedSignals(previousColor);
            emitHsvChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setBlue(float blue)
        {
            if (qFuzzyCompare(blue, this->blue()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setBlue(blue);

            validate();

            emit blueChanged(this->blue());
            emitHslChangedSignals(previousColor);
            emitHsvChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setHslHue(float hue)
        {
            if (qFuzzyCompare(hue, hslHue()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setHslH(hue);

            validate();

            emit hslHueChanged(hslHue());
            emitRgbaChangedSignals(previousColor);
            emitHsvChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setHslSaturation(float saturation)
        {
            if (qFuzzyCompare(saturation, hslSaturation()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setHslS(saturation);

            validate();

            emit hslSaturationChanged(hslSaturation());
            emitRgbaChangedSignals(previousColor);
            emitHsvChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setLightness(float lightness)
        {
            if (qFuzzyCompare(lightness, this->lightness()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setHslL(lightness);

            validate();

            emit lightnessChanged(this->lightness());
            emitRgbaChangedSignals(previousColor);
            emitHsvChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setHsvHue(float hue)
        {
            if (qFuzzyCompare(hue, hsvHue()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setHsvH(hue);

            validate();

            emit hsvHueChanged(hsvHue());
            emitRgbaChangedSignals(previousColor);
            emitHslChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setHsvSaturation(float saturation)
        {
            if (qFuzzyCompare(saturation, hsvSaturation()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setHsvS(saturation);

            validate();

            emit hsvSaturationChanged(hsvSaturation());
            emitRgbaChangedSignals(previousColor);
            emitHslChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setValue(float value)
        {
            if (qFuzzyCompare(value, this->value()))
            {
                return;
            }

            const ColorState previousColor = *m_state;
            m_state->setHsvV(value);

            validate();

            emit valueChanged(this->value());
            emitRgbaChangedSignals(previousColor);
            emitHslChangedSignals(previousColor);
            emit colorChanged(m_state->rgb());
        }

        void ColorController::setHSV(float hue, float saturation, float value)
        {
            const bool hChanged = !qFuzzyCompare(hue, hsvHue());
            const bool sChanged = !qFuzzyCompare(saturation, hsvSaturation());
            const bool vChanged = !qFuzzyCompare(value, this->value());
            if (hChanged || sChanged || vChanged)
            {
                const ColorState previousColor = *m_state;
                m_state->setHsv(hue, saturation, value);
                validate();

                if (hChanged)
                {
                    emit hsvHueChanged(hsvHue());
                }
                if (sChanged)
                {
                    emit hsvSaturationChanged(hsvSaturation());
                }
                if (vChanged)
                {
                    emit valueChanged(this->value());
                }

                emitRgbaChangedSignals(previousColor);
                emitHslChangedSignals(previousColor);
                emit colorChanged(m_state->rgb());
            }
        }

        void ColorController::setAlpha(float alpha)
        {
            if (qFuzzyCompare(alpha, this->alpha()))
            {
                return;
            }

            m_state->setAlpha(alpha);

            validate();

            emit alphaChanged(this->alpha());
            emit colorChanged(m_state->rgb());
        }

        void ColorController::emitRgbaChangedSignals(const ColorState& previousColor)
        {
            if (!qFuzzyCompare(previousColor.red(), m_state->red()))
            {
                emit redChanged(red());
            }

            if (!qFuzzyCompare(previousColor.green(), m_state->green()))
            {
                emit greenChanged(green());
            }

            if (!qFuzzyCompare(previousColor.blue(), m_state->blue()))
            {
                emit blueChanged(blue());
            }

            if (!qFuzzyCompare(previousColor.alpha(), m_state->alpha()))
            {
                emit alphaChanged(alpha());
            }
        }

        void ColorController::emitHslChangedSignals(const ColorState& previousColor)
        {
            if (!qFuzzyCompare(previousColor.hue(), m_state->hue()))
            {
                emit hslHueChanged(hslHue());
            }

            if (!qFuzzyCompare(previousColor.hslS(), m_state->hslS()))
            {
                emit hslSaturationChanged(hslSaturation());
            }

            if (!qFuzzyCompare(previousColor.hslL(), m_state->hslL()))
            {
                emit lightnessChanged(lightness());
            }
        }

        void ColorController::emitHsvChangedSignals(const ColorState& previousColor)
        {
            if (!qFuzzyCompare(previousColor.hue(), m_state->hue()))
            {
                emit hsvHueChanged(hsvHue());
            }

            if (!qFuzzyCompare(previousColor.hsvS(), m_state->hsvS()))
            {
                emit hsvSaturationChanged(hsvSaturation());
            }

            if (!qFuzzyCompare(previousColor.hsvV(), m_state->hsvV()))
            {
                emit valueChanged(value());
            }
        }

        void ColorController::validate()
        {
            ColorValidator* validator = m_state->validator();
            if (validator)
            {
                // don't emit signals while the validator adjusts; it'll be recursive and cause problems
                QSignalBlocker blocker(this);

                if (!validator->isValid(this))
                {
                    validator->adjust(this);

                    validator->warn();
                }
                else
                {
                    validator->acceptColor();
                }
            }
        }

    } // namespace Internal
} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorController.cpp"
