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
#include <QObject>
#include <AzCore/Math/Color.h>
#include <QColor>
#include <AzCore/std/functional.h>
#endif

namespace AzQtComponents
{
    class ColorValidator;

    namespace Internal
    {
        /**
         * Internal controller class used by the ColorPicker dialog. It's not meant to
         * be used anywhere else, thus the lack of AZ_QT_COMPONENTS_API.
         */
        class AZ_QT_COMPONENTS_API ColorController : public QObject
        {
            Q_OBJECT
            Q_PROPERTY(AZ::Color color READ color WRITE setColor NOTIFY colorChanged)
            Q_PROPERTY(ColorValidator* validator READ validator WRITE setValidator)

        public:
            explicit ColorController(QObject* parent = nullptr);
            ~ColorController() override;

            ColorValidator* validator() const;

            const AZ::Color color() const;

            float red() const;
            float green() const;
            float blue() const;

            float hslHue() const;
            float hslSaturation() const;
            float lightness() const;

            float hsvHue() const;
            float hsvSaturation() const;
            float value() const;

            float alpha() const;

            static AZ::Color fromHsl(qreal h, qreal s, qreal l);
            static AZ::Color fromHsv(qreal h, qreal s, qreal v);

        Q_SIGNALS:
            void colorChanged(const AZ::Color& color);

            void redChanged(float red);
            void greenChanged(float green);
            void blueChanged(float blue);

            void hslHueChanged(float hue);
            void hslSaturationChanged(float saturation);
            void lightnessChanged(float lightness);

            void hsvHueChanged(float hue);
            void hsvSaturationChanged(float saturation);
            void valueChanged(float value);

            void alphaChanged(float alpha);

        public Q_SLOTS:
            void setValidator(ColorValidator* validator);

            void setColor(const AZ::Color& color);

            void setRed(float red);
            void setGreen(float green);
            void setBlue(float blue);

            void setHslHue(float hue);
            void setHslSaturation(float saturation);
            void setLightness(float saturation);

            void setHsvHue(float hue);
            void setHsvSaturation(float saturation);
            void setValue(float value);
            void setHSV(float hue, float saturation, float value);

            void setAlpha(float alpha);

        private:
            class ColorState;
            void emitRgbaChangedSignals(const ColorState& previousColor);
            void emitHslChangedSignals(const ColorState& previousColor);
            void emitHsvChangedSignals(const ColorState& previousColor);

            void validate();

            AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 'AzQtComponents::Internal::ColorController::m_state': class 'QScopedPointer<AzQtComponents::Internal::ColorController::ColorState,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'AzQtComponents::Internal::ColorController'
            QScopedPointer<ColorState> m_state;
            AZ_POP_DISABLE_WARNING
        };
    }
} // namespace AzQtComponents
