/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyIntSliderCtrl.hxx"
#include "DHQSlider.hxx"
#include "PropertyQTConstants.h"
#include <AzQtComponents/Components/Widgets/SpinBox.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QtWidgets/QHBoxLayout>
#include <QSignalBlocker>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyIntSliderCtrl::PropertyIntSliderCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        pLayout->setContentsMargins(0, 0, 0, 0);
        m_sliderCombo = new AzQtComponents::SliderCombo(this);
        pLayout->addWidget(m_sliderCombo);
        setFocusProxy(m_sliderCombo);

        connect(m_sliderCombo, &AzQtComponents::SliderCombo::valueChanged, this, &PropertyIntSliderCtrl::onValueChange);
        connect(m_sliderCombo, &AzQtComponents::SliderCombo::editingFinished, this, &PropertyIntSliderCtrl::editingFinished);
    }

    PropertyIntSliderCtrl::~PropertyIntSliderCtrl()
    {
    }

    void PropertyIntSliderCtrl::onValueChange()
    {
        emit valueChanged(value());
    }

    void PropertyIntSliderCtrl::setMinimum(AZ::s64 min)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setMinimum((int)min);
    }

    void PropertyIntSliderCtrl::setMaximum(AZ::s64 max)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setMaximum((int)max);
    }

    void PropertyIntSliderCtrl::setStep(AZ::s64 val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->spinbox()->setSingleStep((int)val);
    }

    void PropertyIntSliderCtrl::setValue(AZ::s64 val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setValue((int)val);
    }

    void PropertyIntSliderCtrl::setMultiplier(AZ::s64 val)
    {
        m_multiplier = val;
    }

    void PropertyIntSliderCtrl::setSoftMinimum(AZ::s64 val)
    {
        m_sliderCombo->setSoftMinimum((int)val);
    }

    void PropertyIntSliderCtrl::setSoftMaximum(AZ::s64 val)
    {
        m_sliderCombo->setSoftMaximum((int)val);
    }

    void PropertyIntSliderCtrl::setPrefix(QString val)
    {
        m_sliderCombo->spinbox()->setPrefix(val);
    }

    void PropertyIntSliderCtrl::setSuffix(QString val)
    {
        m_sliderCombo->spinbox()->setSuffix(val);
    }

    AZ::s64 PropertyIntSliderCtrl::value() const
    {
        return (AZ::s64)m_sliderCombo->value();
    }

    AZ::s64 PropertyIntSliderCtrl::softMinimum() const
    {
        return m_sliderCombo->softMinimum();
    }

    AZ::s64 PropertyIntSliderCtrl::softMaximum() const
    {
        return m_sliderCombo->softMaximum();
    }

    AZ::s64 PropertyIntSliderCtrl::minimum() const
    {
        return (AZ::s64)m_sliderCombo->minimum();
    }

    AZ::s64 PropertyIntSliderCtrl::maximum() const
    {
        return (AZ::s64)m_sliderCombo->maximum();
    }

    AZ::s64 PropertyIntSliderCtrl::step() const
    {
        return (AZ::s64)m_sliderCombo->spinbox()->singleStep();
    }

    AZ::s64 PropertyIntSliderCtrl::multiplier() const
    {
        return m_multiplier;
    }

    QWidget* PropertyIntSliderCtrl::GetFirstInTabOrder()
    {
        return m_sliderCombo->spinbox();
    }
    QWidget* PropertyIntSliderCtrl::GetLastInTabOrder()
    {
        return m_sliderCombo->slider();
    }

    void PropertyIntSliderCtrl::UpdateTabOrder()
    {
        setTabOrder(GetFirstInTabOrder(), GetLastInTabOrder());
    }

    void RegisterIntSliderHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::s8>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::u8>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::s16>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::u16>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::s32>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::u32>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::s64>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSliderHandler<AZ::u64>());
    }

}

#include "UI/PropertyEditor/moc_PropertyIntSliderCtrl.cpp"
