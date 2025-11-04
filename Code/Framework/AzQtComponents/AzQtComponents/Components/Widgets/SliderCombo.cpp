/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/Slider.h>
#include <AzQtComponents/Components/Widgets/SliderCombo.h>
#include <AzQtComponents/Components/Style.h>
#include <AzQtComponents/Components/ConfigHelpers.h>
#include <AzQtComponents/Utilities/Conversions.h>

#include <QHBoxLayout>
#include <QSignalBlocker>
#include <QTimer>

namespace AzQtComponents
{

static void InitialiseSliderCombo(QWidget* sliderCombo, QHBoxLayout* layout, QAbstractSpinBox* spinbox, Slider* slider)
{
    if (!sliderCombo || !layout || !spinbox || !slider)
    {
        return;
    }

    // A 2:1 ratio between spinbox and slider gives the slider more room,
    // but leaves some space for the spin box to expand.
    static const int spinBoxStretch = 1;
    static const int sliderStretch = 2;

    static const int spinBoxMinWidth = 66;
    static const int sliderMinWidth = 30;

    layout->setContentsMargins(0, 0, 0, 0);
    spinbox->setMinimumWidth(spinBoxMinWidth);
    slider->setMinimumWidth(sliderMinWidth);
    // do not send valueChanged every time a character is typed
    spinbox->setKeyboardTracking(false);

    spinbox->setFocusPolicy(Qt::StrongFocus);
    slider->setFocusPolicy(Qt::StrongFocus);
    slider->setFocusProxy(spinbox);
    sliderCombo->setFocusPolicy(Qt::StrongFocus);
    sliderCombo->setFocusProxy(spinbox);

    QSizePolicy sizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(spinBoxStretch);
    spinbox->setSizePolicy(sizePolicy);

    sizePolicy.setHorizontalStretch(sliderStretch);
    slider->setSizePolicy(sizePolicy);
}

SliderCombo::SliderCombo(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    m_spinbox = new SpinBox(this);
    layout->addWidget(m_spinbox);

    m_slider = new SliderInt(this);
    layout->addWidget(m_slider);

    InitialiseSliderCombo(this, layout, m_spinbox, m_slider);

    connect(m_slider, &SliderInt::valueChanged, this, &SliderCombo::setValue);
    connect(m_spinbox, QOverload<int>::of(&SpinBox::valueChanged), this, &SliderCombo::setValue);
    connect(m_slider, &SliderInt::sliderReleased, this, &SliderCombo::editingFinished);
    connect(m_spinbox, &SpinBox::editingFinished, this, &SliderCombo::editingFinished);
    m_spinbox->setInitialValueWasSetting(false);
}

SliderCombo::~SliderCombo()
{
}

void SliderCombo::setValue(int value)
{
    const bool doEmit = m_value != value;
    m_value = value;

    updateSpinBox();
    updateSlider();

    if (doEmit)
    {
        Q_EMIT valueChanged();
    }
}

SliderInt* SliderCombo::slider() const
{
    return m_slider;
}

SpinBox* SliderCombo::spinbox() const
{
    return m_spinbox;
}


int SliderCombo::value() const
{
    return m_value;
}

void SliderCombo::setMinimum(int min)
{
    setRange(min, qMax(m_maximum, min));
}

int SliderCombo::minimum() const
{
    return m_minimum;
}

void SliderCombo::setMaximum(int max)
{
    setRange(qMin(m_minimum, max), max);
}

int SliderCombo::maximum() const
{
    return m_maximum;
}

void SliderCombo::setRange(int min, int max)
{
    max = qMax(min, max);
    if (min != m_minimum || max != m_maximum)
    {
        m_minimum = min;
        m_maximum = max;
        refreshUi();
    }
}

void SliderCombo::setSoftMinimum(int min)
{
    if (min >= std::numeric_limits<int>::lowest() && (min != m_softMinimum || !m_useSoftMinimum))
    {
        m_useSoftMinimum = true;
        m_softMinimum = min;

        refreshUi();
    }
}

int SliderCombo::softMinimum() const
{
    return m_softMinimum;
}

void SliderCombo::setSoftMaximum(int max)
{
    if (max <= std::numeric_limits<int>::max() && (max != m_softMaximum || !m_useSoftMaximum))
    {
        m_useSoftMaximum = true;
        m_softMaximum = max;

        refreshUi();
    }
}

int SliderCombo::softMaximum() const
{
    return m_softMaximum;
}

void SliderCombo::setSoftRange(int min, int max)
{
    if (max <= std::numeric_limits<int>::max() && (max != m_softMaximum || !m_useSoftMaximum) ||
        min >= std::numeric_limits<int>::lowest() && (min != m_softMinimum || !m_useSoftMinimum))
    {
        m_useSoftMinimum = true;
        m_useSoftMaximum = true;
        m_softMinimum = min;
        m_softMaximum = max;

        refreshUi();
    }
 }

void SliderCombo::focusInEvent(QFocusEvent* e)
{
    QWidget::focusInEvent(e);
    m_spinbox->setFocus();
    m_spinbox->selectAll();
}

void SliderCombo::updateSpinBox()
{
    m_spinbox->setInitialValueWasSetting(true);
    {
        const QSignalBlocker block(m_spinbox);
        m_spinbox->setValue(m_value);
    }
    if (m_spinbox->hasFocus())
    {
        m_spinbox->selectAll();
    }
}

void SliderCombo::updateSlider()
{
    const QSignalBlocker block(m_slider);
    m_slider->setValue(m_value);
}

void SliderCombo::refreshUi()
{
    const QSignalBlocker sb(m_slider);
    const QSignalBlocker sb2(m_spinbox);
    m_sliderMin = qMax(m_minimum, std::numeric_limits<int>::lowest());
    m_sliderMax = qMin(m_maximum, std::numeric_limits<int>::max());

    if (m_useSoftMinimum)
    {
        m_sliderMin = qMax(m_softMinimum, std::numeric_limits<int>::lowest());
    }
    if (m_useSoftMaximum)
    {
        m_sliderMax = qMin(m_softMaximum, std::numeric_limits<int>::max());
    }

    m_slider->setRange(m_sliderMin, m_sliderMax);
    m_spinbox->setRange(m_minimum, m_maximum);
}

SliderDoubleCombo::SliderDoubleCombo(QWidget* parent)
    : QWidget(parent)
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    m_spinbox = new DoubleSpinBox(this);
    layout->addWidget(m_spinbox);

    m_slider = new SliderDouble(this);
    layout->addWidget(m_slider);

    InitialiseSliderCombo(this, layout, m_spinbox, m_slider);

    connect(m_slider, &SliderDouble::valueChanged, this, &SliderDoubleCombo::setValueSlider);
    connect(m_spinbox, QOverload<double>::of(&DoubleSpinBox::valueChanged), this, &SliderDoubleCombo::setValue);
    connect(m_slider, &SliderDouble::sliderReleased, this, &SliderDoubleCombo::editingFinished);
    connect(m_spinbox, &DoubleSpinBox::editingFinished, this, &SliderDoubleCombo::editingFinished);
    m_spinbox->setInitialValueWasSetting(false);
}

SliderDoubleCombo::~SliderDoubleCombo()
{
}

bool m_fromSlider{ false };

void SliderDoubleCombo::setValueSlider(double value)
{
    const bool doEmit = m_value != value;
    m_value = value;
    updateSpinBox();
    updateSlider();

    if (doEmit)
    {
        // We don't want to update the slider from setValue as this
        // causes rounding errors in the tooltip hint.
        m_fromSlider = true;
        QTimer::singleShot( 10, []() { m_fromSlider = false; });

        Q_EMIT valueChanged();
    }
}

void SliderDoubleCombo::setValue(double value)
{
    const bool doEmit = m_value != value;
    m_value = value;

    updateSpinBox();
    if (!m_fromSlider)
    {
        updateSlider();

        if (doEmit)
        {
            Q_EMIT valueChanged();
        }
    }
}

SliderDouble* SliderDoubleCombo::slider() const
{
    return m_slider;
}

DoubleSpinBox* SliderDoubleCombo::spinbox() const
{
    return m_spinbox;
}

double SliderDoubleCombo::value() const
{
    return m_spinbox->value();
}

void SliderDoubleCombo::setMinimum(double min)
{
    setRange(min, qMax(m_maximum, min));
}

double SliderDoubleCombo::minimum() const
{
    return m_minimum;
}

void SliderDoubleCombo::setMaximum(double max)
{
    setRange(qMin(m_minimum, max), max);
}

double SliderDoubleCombo::maximum() const
{
    return m_maximum;
}

void SliderDoubleCombo::setRange(double min, double max)
{
    max = qMax(min, max);
    if (min != m_minimum || max != m_maximum)
    {
        m_minimum = min;
        m_maximum = max;
        refreshUi();
    }
}

void SliderDoubleCombo::setSoftMinimum(double min)
{
    if (min >= std::numeric_limits<int>::lowest() && (min != m_softMinimum || !m_useSoftMinimum))
    {
        m_useSoftMinimum = true;
        m_softMinimum = min;

        refreshUi();
    }
}

double SliderDoubleCombo::softMinimum() const
{
    return m_softMinimum;
}

void SliderDoubleCombo::setSoftMaximum(double max)
{
    if (max <= std::numeric_limits<int>::max() && (max != m_softMaximum || !m_useSoftMaximum))
    {
        m_useSoftMaximum = true;
        m_softMaximum = max;

        refreshUi();
    }
}

double SliderDoubleCombo::softMaximum() const
{
    return m_softMaximum;
}

void SliderDoubleCombo::setSoftRange(double min, double max)
{
    if (max <= std::numeric_limits<int>::max() && (max != m_softMaximum || !m_useSoftMaximum) ||
        min >= std::numeric_limits<int>::lowest() && (min != m_softMinimum || !m_useSoftMinimum))
    {
        m_useSoftMinimum = true;
        m_useSoftMaximum = true;
        m_softMinimum = min;
        m_softMaximum = max;

        refreshUi();
    }

}

void SliderDoubleCombo::resetLimits()
{
    m_useSoftMinimum = false;
    m_useSoftMaximum = false;
    m_minimum = 0.0;
    m_minimum = 100.0;
    m_softMinimum = m_minimum;
    m_softMaximum = m_minimum;
    refreshUi();
}

int SliderDoubleCombo::numSteps() const
{
    return m_slider->numSteps();
}

void SliderDoubleCombo::setNumSteps(int steps)
{
    m_slider->setNumSteps(steps);
}

int SliderDoubleCombo::decimals() const
{
    return m_slider->decimals();
}

void SliderDoubleCombo::setDecimals(int decimals)
{
    m_spinbox->setDecimals(decimals);
    m_slider->setDecimals(decimals);
}

double SliderDoubleCombo::curveMidpoint() const
{
    return m_slider->curveMidpoint();
}

void SliderDoubleCombo::setCurveMidpoint(double midpoint)
{
    m_slider->setCurveMidpoint(midpoint);
}

void SliderDoubleCombo::focusInEvent(QFocusEvent* e)
{
    QWidget::focusInEvent(e);
    m_spinbox->setFocus();
    m_spinbox->selectAll();
}

void SliderDoubleCombo::updateSpinBox()
{
    m_spinbox->setInitialValueWasSetting(true);
    {
        const QSignalBlocker block(m_spinbox);
        m_spinbox->setValue(m_value);
    }
    if (m_spinbox->hasFocus())
    {
        m_spinbox->selectAll();
    }
}

void SliderDoubleCombo::updateSlider()
{
    const QSignalBlocker block(m_slider);
    m_slider->setValue(m_value);
}

void SliderDoubleCombo::refreshUi()
{
    const QSignalBlocker sb(m_slider);
    const QSignalBlocker sb2(m_spinbox);
    m_sliderMin = qMax(m_minimum, static_cast<double>(std::numeric_limits<int>::lowest()));
    m_sliderMax = qMin(m_maximum, static_cast<double>(std::numeric_limits<int>::max()));

    if (m_useSoftMinimum)
    {
        m_sliderMin = qMax(m_softMinimum, static_cast<double>(std::numeric_limits<int>::lowest()));
    }
    if (m_useSoftMaximum)
    {
        m_sliderMax = qMin(m_softMaximum, static_cast<double>(std::numeric_limits<int>::max()));
    }

    m_slider->setRange(m_sliderMin, m_sliderMax);
    m_spinbox->setRange(m_minimum, m_maximum);
}

}

#include <Components/Widgets/moc_SliderCombo.cpp>
