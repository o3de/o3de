/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyIntSpinCtrl.hxx"
#include "PropertyQTConstants.h"
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QtWidgets/QSlider>

AZ_PUSH_DISABLE_WARNING(4251 4244, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
                                                               // 4244: conversion from 'int' to 'float', possible loss of data
#include <QtWidgets/QHBoxLayout>
#include <QFocusEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyIntSpinCtrl::PropertyIntSpinCtrl(QWidget* pParent)
        : QWidget(pParent)
        , m_multiplier(1)

    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        setFocusPolicy(Qt::StrongFocus);
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pSpinBox = new AzQtComponents::SpinBox(this);

        m_pSpinBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_pSpinBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pSpinBox->setFixedHeight(PropertyQTConstant_DefaultHeight);

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(0, 0, 0, 0);

        pLayout->addWidget(m_pSpinBox);

        setLayout(pLayout);

        m_pSpinBox->setKeyboardTracking(false);
        m_pSpinBox->setFocusPolicy(Qt::StrongFocus);

        connect(m_pSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onChildSpinboxValueChange(int)));
        connect(m_pSpinBox, &QSpinBox::editingFinished, this, &PropertyIntSpinCtrl::editingFinished);
    };

    QWidget* PropertyIntSpinCtrl::GetFirstInTabOrder()
    {
        return m_pSpinBox;
    }
    QWidget* PropertyIntSpinCtrl::GetLastInTabOrder()
    {
        return m_pSpinBox;
    }

    void PropertyIntSpinCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }


    void PropertyIntSpinCtrl::setValue(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setValue((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::focusInEvent(QFocusEvent* e)
    {
        ((QAbstractSpinBox*)m_pSpinBox)->event(e);
        ((QAbstractSpinBox*)m_pSpinBox)->selectAll();
    }

    void PropertyIntSpinCtrl::setMinimum(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setMinimum((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setMaximum(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setMaximum((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setStep(AZ::s64 value)
    {
        m_pSpinBox->blockSignals(true);

        if ((value / m_multiplier) < 1)
        {
            value = m_multiplier;
        }

        m_pSpinBox->setSingleStep((int)(value / m_multiplier));
        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setMultiplier(AZ::s64 val)
    {
        m_pSpinBox->blockSignals(true);

        AZ::s64 singleStep = step();
        AZ::s64 currValue = value();
        AZ::s64 minVal = minimum();
        AZ::s64 maxVal = maximum();

        m_multiplier = val;

        if ((singleStep / m_multiplier) < 1)
        {
            singleStep = m_multiplier;
        }

        m_pSpinBox->setMinimum((int)(minVal / m_multiplier));
        m_pSpinBox->setMaximum((int)(maxVal / m_multiplier));

        m_pSpinBox->setSingleStep((int)(singleStep / m_multiplier));
        m_pSpinBox->setValue((int)(currValue / m_multiplier));


        m_pSpinBox->blockSignals(false);
    }

    void PropertyIntSpinCtrl::setPrefix(QString val)
    {
        m_pSpinBox->setPrefix(val);
    }

    void PropertyIntSpinCtrl::setSuffix(QString val)
    {
        m_pSpinBox->setSuffix(val);
    }

    AZ::s64 PropertyIntSpinCtrl::value() const
    {
        return (AZ::s64)m_pSpinBox->value() * m_multiplier;
    }

    AZ::s64 PropertyIntSpinCtrl::minimum() const
    {
        return (AZ::s64)m_pSpinBox->minimum() * m_multiplier;
    }

    AZ::s64 PropertyIntSpinCtrl::maximum() const
    {
        return (AZ::s64)m_pSpinBox->maximum() * m_multiplier;
    }

    AZ::s64 PropertyIntSpinCtrl::step() const
    {
        return (AZ::s64)m_pSpinBox->singleStep() * m_multiplier;
    }

    void PropertyIntSpinCtrl::onChildSpinboxValueChange(int newValue)
    {
        //  this->setValue(newValue); // addition validation?
        emit valueChanged((AZ::s64)(newValue * m_multiplier));
    }

    PropertyIntSpinCtrl::~PropertyIntSpinCtrl()
    {
    }

    void RegisterIntSpinBoxHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::s8>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::u8>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::s16>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::u16>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::s32>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::u32>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::s64>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<AZ::u64>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<long>());
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew IntSpinBoxHandler<unsigned long>());
    }

} //AzToolsFramework

#include "UI/PropertyEditor/moc_PropertyIntSpinCtrl.cpp"
