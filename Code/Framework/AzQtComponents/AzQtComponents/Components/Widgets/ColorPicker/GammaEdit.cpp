/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzQtComponents/Components/Widgets/ColorPicker/GammaEdit.h>
#include <AzQtComponents/Components/Widgets/CheckBox.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QDoubleValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>

namespace AzQtComponents
{

GammaEdit::GammaEdit(QWidget* parent)
    : QWidget(parent)
    , m_gamma(1.0)
{
    auto layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto label = new QLabel(tr("Gamma"), this);
    layout->addWidget(label);

    m_edit = new DoubleSpinBox(this);
    m_edit->setDisplayDecimals(2);
    m_edit->setRange(0, 1);
    m_edit->setSingleStep(0.1);
    m_edit->setValue(1.0);
    m_edit->setFixedWidth(50);
    m_edit->setEnabled(false);
    connect(m_edit, static_cast<void(DoubleSpinBox::*)(double)>(&DoubleSpinBox::valueChanged), this, &GammaEdit::valueChanged);
    layout->addWidget(m_edit);

    m_toggleSwitch = new QCheckBox(this);
    CheckBox::applyToggleSwitchStyle(m_toggleSwitch);
    m_toggleSwitch->setChecked(false);
    connect(m_toggleSwitch, &QAbstractButton::toggled, this, &GammaEdit::toggled);
    connect(m_toggleSwitch, &QAbstractButton::toggled, m_edit, &QWidget::setEnabled);
    layout->addWidget(m_toggleSwitch);

    layout->addStretch();
}

GammaEdit::~GammaEdit()
{
}

bool GammaEdit::isChecked() const
{
    return m_toggleSwitch->isChecked();
}

qreal GammaEdit::gamma() const
{
    return m_gamma;
}

void GammaEdit::setChecked(bool enabled)
{
    m_toggleSwitch->setChecked(enabled);
}

void GammaEdit::setGamma(qreal gamma)
{
    if (qFuzzyCompare(gamma, m_gamma))
    {
        return;
    }

    m_gamma = gamma;

    const QSignalBlocker b(m_edit);
    m_edit->setValue(m_gamma);

    emit gammaChanged(gamma);
}

void GammaEdit::valueChanged(double gamma)
{
    if (qFuzzyCompare(gamma, m_gamma))
    {
        return;
    }
    m_gamma = gamma;
    emit gammaChanged(gamma);
}

} // namespace AzQtComponents

#include "Components/Widgets/ColorPicker/moc_GammaEdit.cpp"
