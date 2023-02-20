/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/Widgets/ColorPicker/ColorRGBAEdit.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>

#include <QDoubleValidator>
#include <QGridLayout>
#include <QLabel>

namespace AzQtComponents
{

ColorRGBAEdit::ColorRGBAEdit(QWidget* parent)
    : QWidget(parent)
    , m_mode(Mode::Rgba)
    , m_readOnly(false)
    , m_red(0.0)
    , m_green(0.0)
    , m_blue(0.0)
    , m_alpha(1.0)
{
    m_layout = new QGridLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

    auto doubleChanged = static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged);

    m_redSpin = createComponentSpinBox();
    m_redSpin->setValue(m_red);
    connect(m_redSpin, doubleChanged, this, &ColorRGBAEdit::redValueChanged);
    connect(m_redSpin, &DoubleSpinBox::valueChangeBegan, this, &ColorRGBAEdit::valueChangeBegan);
    connect(m_redSpin, &DoubleSpinBox::valueChangeEnded, this, &ColorRGBAEdit::valueChangeEnded);
    m_layout->addWidget(m_redSpin, 0, 0);

    auto redLabel = new QLabel(tr("R"), this);
    redLabel->setAlignment(Qt::AlignHCenter);
    m_layout->addWidget(redLabel, 1, 0);

    m_greenSpin = createComponentSpinBox();
    m_greenSpin->setValue(m_green);
    connect(m_greenSpin, doubleChanged, this, &ColorRGBAEdit::greenValueChanged);
    connect(m_greenSpin, &DoubleSpinBox::valueChangeBegan, this, &ColorRGBAEdit::valueChangeBegan);
    connect(m_greenSpin, &DoubleSpinBox::valueChangeEnded, this, &ColorRGBAEdit::valueChangeEnded);
    m_layout->addWidget(m_greenSpin, 0, 1);

    auto greenLabel = new QLabel(tr("G"), this);
    greenLabel->setAlignment(Qt::AlignHCenter);
    m_layout->addWidget(greenLabel, 1, 1);

    m_blueSpin = createComponentSpinBox();
    m_blueSpin->setValue(m_blue);
    connect(m_blueSpin, doubleChanged, this, &ColorRGBAEdit::blueValueChanged);
    connect(m_blueSpin, &DoubleSpinBox::valueChangeBegan, this, &ColorRGBAEdit::valueChangeBegan);
    connect(m_blueSpin, &DoubleSpinBox::valueChangeEnded, this, &ColorRGBAEdit::valueChangeEnded);
    m_layout->addWidget(m_blueSpin, 0, 2);

    auto blueLabel = new QLabel(tr("B"), this);
    blueLabel->setAlignment(Qt::AlignHCenter);
    m_layout->addWidget(blueLabel, 1, 2);

    m_alphaSpin = createComponentSpinBox();
    m_alphaSpin->setValue(m_alpha);
    connect(m_alphaSpin, doubleChanged, this, &ColorRGBAEdit::alphaValueChanged);
    connect(m_alphaSpin, &DoubleSpinBox::valueChangeBegan, this, &ColorRGBAEdit::valueChangeBegan);
    connect(m_alphaSpin, &DoubleSpinBox::valueChangeEnded, this, &ColorRGBAEdit::valueChangeEnded);
    m_layout->addWidget(m_alphaSpin, 0, 3);

    m_alphaLabel = new QLabel(tr("A"), this);
    m_alphaLabel->setAlignment(Qt::AlignHCenter);
    m_layout->addWidget(m_alphaLabel, 1, 3);
}

ColorRGBAEdit::~ColorRGBAEdit()
{
}

ColorRGBAEdit::Mode ColorRGBAEdit::mode() const
{
    return m_mode;
}

bool ColorRGBAEdit::isReadOnly() const
{
    return m_readOnly;
}

qreal ColorRGBAEdit::red() const
{
    return m_red;
}

qreal ColorRGBAEdit::green() const
{
    return m_green;
}

qreal ColorRGBAEdit::blue() const
{
    return m_blue;
}

qreal ColorRGBAEdit::alpha() const
{
    return m_alpha;
}

void ColorRGBAEdit::setHorizontalSpacing(int spacing)
{
    m_layout->setHorizontalSpacing(spacing);
}

void ColorRGBAEdit::setMode(Mode mode)
{
    if (mode == m_mode)
    {
        return;
    }

    m_mode = mode;

    m_alphaSpin->setVisible(m_mode == Mode::Rgba);
    m_alphaLabel->setVisible(m_mode == Mode::Rgba);
    if (m_mode == Mode::Rgb)
    {
        setAlpha(1.0);
    }

    emit modeChanged(mode);
}

void ColorRGBAEdit::setReadOnly(bool readOnly)
{
    if (readOnly == m_readOnly)
    {
        return;
    }

    m_readOnly = readOnly;

    m_redSpin->setReadOnly(readOnly);
    m_greenSpin->setReadOnly(readOnly);
    m_blueSpin->setReadOnly(readOnly);
    m_alphaSpin->setReadOnly(readOnly);

    emit readOnlyChanged(readOnly);
}

void ColorRGBAEdit::setRed(qreal red)
{
    if (qFuzzyCompare(red, m_red) || m_redSpin->isEditing())
    {
        return;
    }

    m_red = red;

    const QSignalBlocker b(m_redSpin);
    m_redSpin->setValue(m_red);
}

void ColorRGBAEdit::setGreen(qreal green)
{
    if (qFuzzyCompare(green, m_green) || m_greenSpin->isEditing())
    {
        return;
    }
    m_green = green;

    const QSignalBlocker b(m_greenSpin);
    m_greenSpin->setValue(m_green);
}

void ColorRGBAEdit::setBlue(qreal blue)
{
    if (qFuzzyCompare(blue, m_blue) || m_blueSpin->isEditing())
    {
        return;
    }
    m_blue = blue;

    const QSignalBlocker b(m_blueSpin);
    m_blueSpin->setValue(m_blue);
}

void ColorRGBAEdit::setAlpha(qreal alpha)
{
    if (qFuzzyCompare(alpha, m_alpha) || m_alphaSpin->isEditing())
    {
        return;
    }
    m_alpha = alpha;

    const QSignalBlocker b(m_alphaSpin);
    m_alphaSpin->setValue(m_alpha);
}

DoubleSpinBox* ColorRGBAEdit::createComponentSpinBox()
{
    auto spinBox = new DoubleSpinBox(this);
    spinBox->setRange(0.0, 12.5);
    spinBox->setSingleStep(1.0 / 255.0);
    spinBox->setDecimals(8);
    spinBox->setDisplayDecimals(3);
    spinBox->setFixedWidth(52);
    spinBox->setOptions(DoubleSpinBox::SHOW_ONE_DECIMAL_PLACE_ALWAYS);
    return spinBox;
}

void ColorRGBAEdit::redValueChanged(qreal value)
{
    if (qFuzzyCompare(value, m_red))
    {
        return;
    }
    m_red = value;
    emit redChanged(value);
}

void ColorRGBAEdit::greenValueChanged(qreal value)
{
    if (qFuzzyCompare(value, m_green))
    {
        return;
    }
    m_green = value;
    emit greenChanged(value);
}

void ColorRGBAEdit::blueValueChanged(qreal value)
{
    if (qFuzzyCompare(value, m_blue))
    {
        return;
    }
    m_blue = value;
    emit blueChanged(value);
}

void ColorRGBAEdit::alphaValueChanged(qreal value)
{
    if (qFuzzyCompare(value, m_alpha))
    {
        return;
    }
    m_alpha = value;
    emit alphaChanged(value);
}

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_ColorRGBAEdit.cpp"
