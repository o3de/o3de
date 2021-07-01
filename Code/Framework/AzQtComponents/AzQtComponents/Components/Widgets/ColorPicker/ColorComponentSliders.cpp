/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorComponentSliders.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <AzQtComponents/Components/Widgets/ColorPicker/ColorController.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Casting/numeric_cast.h>

#include <QIntValidator>
#include <QLabel>
#include <QSpinBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QKeyEvent>

namespace AzQtComponents
{
    namespace
    {
        int GetRequiredWidth(const QWidget* widget)
        {
            const QString labels[] = {
                QObject::tr("H"),
                QObject::tr("S"),
                QObject::tr("L"),
                QObject::tr("V"),
                QObject::tr("R"),
                QObject::tr("G"),
                QObject::tr("B")
            };

            widget->ensurePolished();
            QFontMetrics metrics(widget->font());

            int widest = 0;
            for (const QString& label : labels)
            {
                widest = std::max(widest, metrics.horizontalAdvance(label));
            }

            return widest;
        }

        template <typename SliderType>
        QString createToolTipText(QString prefix, qreal position, SliderType* slider, bool includeRGB = true)
        {
            if (includeRGB)
            {
                QColor rgb = slider->colorAt(position);
                return QStringLiteral("%0: %1\nRGB: %2, %3, %4").arg(prefix).arg(qRound(position * slider->maximum())).arg(rgb.red()).arg(rgb.green()).arg(rgb.blue());
            }
            else
            {
                return QStringLiteral("%0: %1").arg(prefix).arg(qRound(position * slider->maximum()));
            }
        }
    }

ColorComponentEdit::ColorComponentEdit(const QString& labelText, int softMaximum, int hardMaximum, QWidget* parent)
    : QWidget(parent)
    , m_value(0)
    , m_spinBox(new SpinBox(this))
    , m_slider(new GradientSlider(this))
{
    auto layout = new QHBoxLayout(this);

    layout->setContentsMargins(0, 0, 0, 0);

    auto label = new QLabel(labelText, this);
    label->setFixedWidth(GetRequiredWidth(this));
    label->setAlignment(Qt::AlignCenter);
    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(label);

    m_spinBox->setRange(0, hardMaximum);
    m_spinBox->setFixedWidth(32);
    connect(m_spinBox, static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this, &ColorComponentEdit::spinValueChanged);
    connect(m_spinBox, &SpinBox::valueChangeBegan, this, &ColorComponentEdit::valueChangeBegan);
    connect(m_spinBox, &SpinBox::valueChangeEnded, this, &ColorComponentEdit::valueChangeEnded);
    layout->addWidget(m_spinBox);

    layout->addSpacing(2);

    // Ignore mouse wheel events for these sliders so the user doesn't inadvertently
    // change the value
    m_slider->SetIgnoreWheelEvents(true);

    m_slider->setMinimum(0);
    m_slider->setMaximum(softMaximum);
    connect(m_slider, &QSlider::valueChanged, this, &ColorComponentEdit::sliderValueChanged);
    connect(m_slider, &QSlider::sliderPressed, this, &ColorComponentEdit::valueChangeBegan);
    connect(m_slider, &QSlider::sliderReleased, this, &ColorComponentEdit::valueChangeEnded);
    layout->addWidget(m_slider);

    m_slider->setFocusPolicy(Qt::ClickFocus);
}

ColorComponentEdit::~ColorComponentEdit()
{
}

void ColorComponentEdit::setColorFunction(GradientSlider::ColorFunction colorFunction)
{
    m_slider->setColorFunction(colorFunction);
}

void ColorComponentEdit::setToolTipFunction(GradientSlider::ToolTipFunction toolTipFunction)
{
    m_slider->setToolTipFunction(toolTipFunction);
}

qreal ColorComponentEdit::value() const
{
    return m_value;
}

void ColorComponentEdit::setValue(qreal value)
{
    if (qFuzzyCompare(value, m_value))
    {
        return;
    }

    m_value = value;

    const QSignalBlocker sliderBlocker(m_slider);
    const QSignalBlocker spinBoxBlocker(m_spinBox);

    int sliderValue = qRound(m_value*m_slider->maximum());
    m_slider->setValue(sliderValue);
    m_spinBox->setValue (sliderValue);

    emit valueChanged(m_value);
}

QColor ColorComponentEdit::colorAt(qreal position) const
{
    return m_slider->colorAt(position);
}

void ColorComponentEdit::updateGradient()
{
    m_slider->updateGradient();
}

void ColorComponentEdit::spinValueChanged(int value)
{
    m_value = static_cast<qreal>(value) / m_slider->maximum();

    const QSignalBlocker b(m_slider);
    m_slider->setValue(value);

    emit valueChanged(m_value);
}

void ColorComponentEdit::sliderValueChanged(int value)
{
    m_value = static_cast<qreal>(value) / m_slider->maximum();

    const QSignalBlocker b(m_spinBox);
    m_spinBox->setValue(static_cast<int>(value));

    emit valueChanged(m_value);
}

HSLSliders::HSLSliders(QWidget* widget)
    : QWidget(widget)
    , m_mode(Mode::Hsl)
    , m_defaultLForHsMode(0.85)
{
    auto layout = new QVBoxLayout(this);

    m_hueSlider = new ColorComponentEdit(tr("H"), 360, 360, this);
    layout->addWidget(m_hueSlider);

    m_saturationSlider = new ColorComponentEdit(tr("S"), 100, 100, this);
    layout->addWidget(m_saturationSlider);

    m_lightnessSlider = new ColorComponentEdit(tr("L"), 100, 1250, this);
    layout->addWidget(m_lightnessSlider);

    m_hueSlider->setColorFunction([this](qreal position) {
        using namespace AzQtComponents::Internal;

        return toQColor(ColorController::fromHsl(position, m_saturationSlider->value(), m_lightnessSlider->value()));
    });
    m_hueSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Hue", position, m_hueSlider);
    });

    m_saturationSlider->setColorFunction([this](qreal position) {
        using namespace AzQtComponents::Internal;

        return toQColor(ColorController::fromHsl(m_hueSlider->value(), position, m_lightnessSlider->value()));
    });
    m_saturationSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Saturation", position, m_saturationSlider);
    });

    m_lightnessSlider->setColorFunction([this](qreal position) {
        using namespace AzQtComponents::Internal;

        return toQColor(ColorController::fromHsl(m_hueSlider->value(), m_saturationSlider->value(), position));
    });
    m_lightnessSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Lightness", position, m_lightnessSlider);
    });

    connect(m_hueSlider, &ColorComponentEdit::valueChanged, this, &HSLSliders::hueChanged);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_lightnessSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeBegan, this, &HSLSliders::valueChangeBegan);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeEnded, this, &HSLSliders::valueChangeEnded);

    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, this, &HSLSliders::saturationChanged);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_lightnessSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeBegan, this, &HSLSliders::valueChangeBegan);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeEnded, this, &HSLSliders::valueChangeEnded);

    connect(m_lightnessSlider, &ColorComponentEdit::valueChanged, this, &HSLSliders::lightnessChanged);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChangeBegan, this, &HSLSliders::valueChangeBegan);
    connect(m_lightnessSlider, &ColorComponentEdit::valueChangeEnded, this, &HSLSliders::valueChangeEnded);

}

HSLSliders::Mode HSLSliders::mode() const
{
    return m_mode;
}

qreal HSLSliders::hue() const
{
    return m_hueSlider->value();
}

qreal HSLSliders::saturation() const
{
    return m_saturationSlider->value();
}

qreal HSLSliders::lightness() const
{
    return m_lightnessSlider->value();
}

qreal HSLSliders::defaultLForHsMode() const
{
    return m_defaultLForHsMode;
}

void HSLSliders::setMode(Mode mode)
{
    if (mode == m_mode)
    {
        return;
    }

    m_mode = mode;

    m_lightnessSlider->setVisible(m_mode == Mode::Hsl);
    if (m_mode == Mode::Hs)
    {
        setLightness(m_defaultLForHsMode);
    }

    emit modeChanged(mode);
}

void HSLSliders::setHue(qreal hue)
{
    const QSignalBlocker b(m_hueSlider);
    m_hueSlider->setValue(hue);
    m_saturationSlider->updateGradient();
    m_lightnessSlider->updateGradient();
}

void HSLSliders::setSaturation(qreal saturation)
{
    const QSignalBlocker b(m_saturationSlider);
    m_saturationSlider->setValue(saturation);
    m_hueSlider->updateGradient();
    m_lightnessSlider->updateGradient();
}

void HSLSliders::setLightness(qreal lightness)
{
    const QSignalBlocker b(m_lightnessSlider);
    m_lightnessSlider->setValue(lightness);
    m_hueSlider->updateGradient();
    m_saturationSlider->updateGradient();
}

void HSLSliders::setDefaultLForHsMode(qreal value)
{
    m_defaultLForHsMode = value;
}

HSVSliders::HSVSliders(QWidget* widget)
    : QWidget(widget)
    , m_defaultVForHsMode(0.85)
{
    auto layout = new QVBoxLayout(this);

    m_hueSlider = new ColorComponentEdit(tr("H"), 360, 360, this);
    layout->addWidget(m_hueSlider);

    m_saturationSlider = new ColorComponentEdit(tr("S"), 100, 100, this);
    layout->addWidget(m_saturationSlider);

    m_valueSlider = new ColorComponentEdit(tr("V"), 100, 1250, this);
    layout->addWidget(m_valueSlider);

    m_hueSlider->setColorFunction([this](qreal position) {
        using namespace AzQtComponents::Internal;

        return toQColor(ColorController::fromHsv(position, m_saturationSlider->value(), m_valueSlider->value()));
    });
    m_hueSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Hue", position, m_hueSlider);
    });

    m_saturationSlider->setColorFunction([this](qreal position) {
        using namespace AzQtComponents::Internal;

        return toQColor(ColorController::fromHsv(m_hueSlider->value(), position, m_valueSlider->value()));
    });
    m_saturationSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Saturation", position, m_saturationSlider);
    });

    m_valueSlider->setColorFunction([this](qreal position) {
        using namespace AzQtComponents::Internal;

        return toQColor(ColorController::fromHsv(m_hueSlider->value(), m_saturationSlider->value(), position));
    });
    m_valueSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Value", position, m_valueSlider);
    });

    connect(m_hueSlider, &ColorComponentEdit::valueChanged, this, &HSVSliders::hueChanged);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChanged, m_valueSlider, &ColorComponentEdit::updateGradient);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeBegan, this, &HSVSliders::valueChangeBegan);
    connect(m_hueSlider, &ColorComponentEdit::valueChangeEnded, this, &HSVSliders::valueChangeEnded);

    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, this, &HSVSliders::saturationChanged);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChanged, m_valueSlider, &ColorComponentEdit::updateGradient);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeBegan, this, &HSVSliders::valueChangeBegan);
    connect(m_saturationSlider, &ColorComponentEdit::valueChangeEnded, this, &HSVSliders::valueChangeEnded);

    connect(m_valueSlider, &ColorComponentEdit::valueChanged, this, &HSVSliders::valueChanged);
    connect(m_valueSlider, &ColorComponentEdit::valueChanged, m_hueSlider, &ColorComponentEdit::updateGradient);
    connect(m_valueSlider, &ColorComponentEdit::valueChanged, m_saturationSlider, &ColorComponentEdit::updateGradient);
    connect(m_valueSlider, &ColorComponentEdit::valueChangeBegan, this, &HSVSliders::valueChangeBegan);
    connect(m_valueSlider, &ColorComponentEdit::valueChangeEnded, this, &HSVSliders::valueChangeEnded);
}

HSVSliders::Mode HSVSliders::mode() const
{
    return m_mode;
}

qreal HSVSliders::hue() const
{
    return m_hueSlider->value();
}

qreal HSVSliders::saturation() const
{
    return m_saturationSlider->value();
}

qreal HSVSliders::value() const
{
    return m_valueSlider->value();
}

qreal HSVSliders::defaultVForHsMode() const
{
    return m_defaultVForHsMode;
}

void HSVSliders::setMode(Mode mode)
{
    if (mode == m_mode)
    {
        return;
    }

    m_mode = mode;

    m_valueSlider->setVisible(m_mode == Mode::Hsv);
    if (m_mode == Mode::Hs)
    {
        setValue(m_defaultVForHsMode);
    }

    emit modeChanged(mode);
}

void HSVSliders::setHue(qreal hue)
{
    const QSignalBlocker b(m_hueSlider);
    m_hueSlider->setValue(hue);
    m_saturationSlider->updateGradient();
    m_valueSlider->updateGradient();
}

void HSVSliders::setSaturation(qreal saturation)
{
    const QSignalBlocker b(m_saturationSlider);
    m_saturationSlider->setValue(saturation);
    m_hueSlider->updateGradient();
    m_valueSlider->updateGradient();
}

void HSVSliders::setValue(qreal value)
{
    const QSignalBlocker b(m_valueSlider);
    m_valueSlider->setValue(value);
    m_hueSlider->updateGradient();
    m_saturationSlider->updateGradient();
}

void HSVSliders::setDefaultVForHsMode(qreal value)
{
    m_defaultVForHsMode = value;
}

RGBSliders::RGBSliders(QWidget* widget)
    : QWidget(widget)
{
    auto layout = new QVBoxLayout(this);

    m_redSlider = new ColorComponentEdit(tr("R"), 255, 3187, this);
    layout->addWidget(m_redSlider);

    m_greenSlider = new ColorComponentEdit(tr("G"), 255, 3187, this);
    layout->addWidget(m_greenSlider);

    m_blueSlider = new ColorComponentEdit(tr("B"), 255, 3187, this);
    layout->addWidget(m_blueSlider);

    m_redSlider->setColorFunction([this](qreal position) {
        return toQColor(aznumeric_cast<float>(position), aznumeric_cast<float>(m_greenSlider->value()), aznumeric_cast<float>(m_blueSlider->value()));
    });
    m_redSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Red", position, m_redSlider, /*includeRGB=*/false);
    });

    m_greenSlider->setColorFunction([this](qreal position) {
        return toQColor(aznumeric_cast<float>(m_redSlider->value()), aznumeric_cast<float>(position), aznumeric_cast<float>(m_blueSlider->value()));
    });
    m_greenSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Green", position, m_greenSlider, /*includeRGB=*/false);
    });

    m_blueSlider->setColorFunction([this](qreal position) {
        return toQColor(aznumeric_cast<float>(m_redSlider->value()), aznumeric_cast<float>(m_greenSlider->value()), aznumeric_cast<float>(position));
    });
    m_blueSlider->setToolTipFunction([this](qreal position) {
        return createToolTipText("Blue", position, m_blueSlider, /*includeRGB=*/false);
    });

    connect(m_redSlider, &ColorComponentEdit::valueChanged, this, &RGBSliders::redChanged);
    connect(m_redSlider, &ColorComponentEdit::valueChanged, m_greenSlider, &ColorComponentEdit::updateGradient);
    connect(m_redSlider, &ColorComponentEdit::valueChanged, m_blueSlider, &ColorComponentEdit::updateGradient);
    connect(m_redSlider, &ColorComponentEdit::valueChangeBegan, this, &RGBSliders::valueChangeBegan);
    connect(m_redSlider, &ColorComponentEdit::valueChangeEnded, this, &RGBSliders::valueChangeEnded);

    connect(m_greenSlider, &ColorComponentEdit::valueChanged, this, &RGBSliders::greenChanged);
    connect(m_greenSlider, &ColorComponentEdit::valueChanged, m_redSlider, &ColorComponentEdit::updateGradient);
    connect(m_greenSlider, &ColorComponentEdit::valueChanged, m_blueSlider, &ColorComponentEdit::updateGradient);
    connect(m_greenSlider, &ColorComponentEdit::valueChangeBegan, this, &RGBSliders::valueChangeBegan);
    connect(m_greenSlider, &ColorComponentEdit::valueChangeEnded, this, &RGBSliders::valueChangeEnded);

    connect(m_blueSlider, &ColorComponentEdit::valueChanged, this, &RGBSliders::blueChanged);
    connect(m_blueSlider, &ColorComponentEdit::valueChanged, m_redSlider, &ColorComponentEdit::updateGradient);
    connect(m_blueSlider, &ColorComponentEdit::valueChanged, m_greenSlider, &ColorComponentEdit::updateGradient);
    connect(m_blueSlider, &ColorComponentEdit::valueChangeBegan, this, &RGBSliders::valueChangeBegan);
    connect(m_blueSlider, &ColorComponentEdit::valueChangeEnded, this, &RGBSliders::valueChangeEnded);
}

qreal RGBSliders::red() const
{
    return m_redSlider->value();
}

qreal RGBSliders::green() const
{
    return m_greenSlider->value();
}

qreal RGBSliders::blue() const
{
    return m_blueSlider->value();
}

void RGBSliders::setRed(qreal red)
{
    const QSignalBlocker b(m_redSlider);
    m_redSlider->setValue(red);
    m_greenSlider->updateGradient();
    m_blueSlider->updateGradient();
}

void RGBSliders::setGreen(qreal green)
{
    const QSignalBlocker b(m_greenSlider);
    m_greenSlider->setValue(green);
    m_redSlider->updateGradient();
    m_blueSlider->updateGradient();
}

void RGBSliders::setBlue(qreal blue)
{
    const QSignalBlocker b(m_blueSlider);
    m_blueSlider->setValue(blue);
    m_redSlider->updateGradient();
    m_greenSlider->updateGradient();
}

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorComponentSliders.cpp"
