/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyDoubleSpinCtrl.hxx"
#include "PropertyQTConstants.h"
#include <QSlider>
#include <QLineEdit>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QHBoxLayout>
AZ_POP_DISABLE_WARNING
#include <QTimer>
#include <cfloat>
#include <AzCore/Math/MathUtils.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option") // 4244: conversion from 'int' to 'float', possible loss of data
                                                               // 4251: 'QInputEvent::modState': class 'QFlags<Qt::KeyboardModifier>' needs to have dll-interface to be used by clients of class 'QInputEvent'
#include <QFocusEvent>
AZ_POP_DISABLE_WARNING

namespace AzToolsFramework
{
    PropertyDoubleSpinCtrl::PropertyDoubleSpinCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        m_pSpinBox = new AzQtComponents::DoubleSpinBox(this);
        m_pSpinBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_pSpinBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_pSpinBox->setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_pSpinBox->setFocusPolicy(Qt::StrongFocus);
        m_multiplier = 1;

        setLayout(pLayout);
        pLayout->setContentsMargins(0, 0, 0, 0);
        pLayout->addWidget(m_pSpinBox);
        m_pSpinBox->setKeyboardTracking(false);
        setFocusProxy(m_pSpinBox);
        setFocusPolicy(m_pSpinBox->focusPolicy());

        connect(m_pSpinBox, SIGNAL(valueChanged(double)), this, SLOT(onChildSpinboxValueChange(double)));
        connect(m_pSpinBox, &QDoubleSpinBox::editingFinished, this, &PropertyDoubleSpinCtrl::editingFinished);

        m_defaultDecimals = m_pSpinBox->decimals();
        m_defaultDisplayDecimals = m_pSpinBox->displayDecimals();
    }

    QWidget* PropertyDoubleSpinCtrl::GetFirstInTabOrder()
    {
        return m_pSpinBox;
    }
    QWidget* PropertyDoubleSpinCtrl::GetLastInTabOrder()
    {
        return m_pSpinBox;
    }

    void PropertyDoubleSpinCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    void PropertyDoubleSpinCtrl::setValue(double value)
    {
        value = AZ::ClampIfCloseMag(value, double(round(value)));

        bool notifyLater = false;
        m_pSpinBox->blockSignals(true);

        if ( (!AZ::IsCloseMag(value, m_pSpinBox->minimum())) && (value < m_pSpinBox->minimum()) )
        {
            // setValue will auto-clamp this, no need to do that here.
            notifyLater = true;
        }

        if ((!AZ::IsCloseMag(value, m_pSpinBox->maximum())) && (value > m_pSpinBox->maximum()))
        {
            // setValue will auto-clamp this, no need to do that here.
            notifyLater = true;
        }

        const double oldValue = m_pSpinBox->value();
        m_pSpinBox->setValue(value);
        m_pSpinBox->blockSignals(false);

        //the value didn't change, so don't send a notify
        if (!AZ::ClampIfCloseMag(value, oldValue) && notifyLater)
        {
            // Send a change notification for value being clamped to min/max
            float newValue = static_cast<float>(m_pSpinBox->value());
            // queue an invocation of value changed next tick after everything is good.)
            QTimer::singleShot(0, this, [this, newValue]() { Q_EMIT valueChanged(newValue); });
        }

    }

    void PropertyDoubleSpinCtrl::focusInEvent(QFocusEvent* e)
    {
        ((QAbstractSpinBox*)m_pSpinBox)->event(e);
        ((QAbstractSpinBox*)m_pSpinBox)->selectAll();
    }

    void PropertyDoubleSpinCtrl::focusOutEvent(QFocusEvent* e)
    {
        // needed to ensure that the correct selection of the spinbox text occurs
        ((QAbstractSpinBox*)m_pSpinBox)->event(e);
    }

    void PropertyDoubleSpinCtrl::setMinimum(double value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setMinimum(value);
        m_pSpinBox->blockSignals(false);
    }

    void PropertyDoubleSpinCtrl::setMaximum(double value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setMaximum(value);
        m_pSpinBox->blockSignals(false);
    }

    void PropertyDoubleSpinCtrl::setStep(double value)
    {
        m_pSpinBox->blockSignals(true);
        m_pSpinBox->setSingleStep(value);
        m_pSpinBox->blockSignals(false);
    }

    void PropertyDoubleSpinCtrl::setMultiplier(double val)
    {
        m_multiplier = val;
    }

    void PropertyDoubleSpinCtrl::setPrefix(QString val)
    {
        m_pSpinBox->setPrefix(val);
    }

    void PropertyDoubleSpinCtrl::setSuffix(QString val)
    {
        m_pSpinBox->setSuffix(val);
    }

    void PropertyDoubleSpinCtrl::setDecimals(int precision)
    {
        m_pSpinBox->setDecimals(precision);
    }

    void PropertyDoubleSpinCtrl::setDisplayDecimals(int displayDecimals)
    {
        m_pSpinBox->SetDisplayDecimals(displayDecimals);
    }

    double PropertyDoubleSpinCtrl::value() const
    {
        return m_pSpinBox->value();
    }

    double PropertyDoubleSpinCtrl::minimum() const
    {
        return m_pSpinBox->minimum();
    }

    double PropertyDoubleSpinCtrl::maximum() const
    {
        return m_pSpinBox->maximum();
    }

    double PropertyDoubleSpinCtrl::step() const
    {
        return m_pSpinBox->singleStep();
    }

    double PropertyDoubleSpinCtrl::multiplier() const
    {
        return m_multiplier;
    }

    int PropertyDoubleSpinCtrl::GetDefaultDecimals() const
    {
        return m_defaultDecimals;
    }

    int PropertyDoubleSpinCtrl::GetDefaultDisplayDecimals() const
    {
        return m_defaultDisplayDecimals;
    }

    void PropertyDoubleSpinCtrl::onChildSpinboxValueChange(double newValue)
    {
        newValue = AZ::ClampIfCloseMag(newValue, double(round(newValue)));

        emit valueChanged(newValue);
    }

    PropertyDoubleSpinCtrl::~PropertyDoubleSpinCtrl()
    {
    }

    // a common function to eat attribs, for all int handlers:
    void doublePropertySpinboxHandler::ConsumeAttributeCommon(PropertyDoubleSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        (void)debugName;
        double value;
        if (attrib == AZ::Edit::Attributes::Min)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setMinimum(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Min' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setMaximum(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Max' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::Step)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setStep(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Step' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ_CRC_CE("Multiplier"))
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setMultiplier(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Multiplier' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::Suffix)
        {
            AZStd::string result;
            if (attrValue->Read<AZStd::string>(result))
            {
                GUI->setSuffix(result.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Suffix' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ_CRC_CE("Prefix"))
        {
            AZStd::string result;
            if (attrValue->Read<AZStd::string>(result))
            {
                GUI->setPrefix(result.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Prefix' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::Decimals)
        {
            int intValue = 0;
            if (attrValue->Read<int>(intValue))
            {
                GUI->setDecimals(intValue);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Decimals' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::DisplayDecimals)
        {
            int intValue = 0;
            if (attrValue->Read<int>(intValue))
            {
                GUI->setDisplayDecimals(intValue);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'DisplayDecimals' attribute from property '%s' into Spin Box", debugName);
            }
            return;
        }
    }

    void doublePropertySpinboxHandler::ResetGUIToDefaultsCommon(PropertyDoubleSpinCtrl* GUI)
    {
        GUI->setStep(1);
        GUI->setMultiplier(1);
        GUI->setSuffix("");
        GUI->setPrefix("");
        GUI->setDecimals(GUI->GetDefaultDecimals());
        GUI->setDisplayDecimals(GUI->GetDefaultDisplayDecimals());
    }

    bool doublePropertySpinboxHandler::GetDefaultDecimals(PropertyDoubleSpinCtrl* GUI)
    {
        return GUI->GetDefaultDecimals();
    }

    bool doublePropertySpinboxHandler::GetDefaultDisplayDecimals(PropertyDoubleSpinCtrl* GUI)
    {
        return GUI->GetDefaultDisplayDecimals();
    }

    QWidget* doublePropertySpinboxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyDoubleSpinCtrl* newCtrl = aznew PropertyDoubleSpinCtrl(pParent);
        connect(newCtrl, &PropertyDoubleSpinCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            });
        connect(newCtrl, &PropertyDoubleSpinCtrl::editingFinished, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(-DBL_MAX);
        newCtrl->setMaximum(DBL_MAX);

        return newCtrl;
    }

    QWidget* floatPropertySpinboxHandler::CreateGUI(QWidget* pParent)
    {
        PropertyDoubleSpinCtrl* newCtrl = aznew PropertyDoubleSpinCtrl(pParent);
        connect(newCtrl, &PropertyDoubleSpinCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            });
        connect(newCtrl, &PropertyDoubleSpinCtrl::editingFinished, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setMinimum(std::numeric_limits<float>::lowest());
        newCtrl->setMaximum(std::numeric_limits<float>::max());

        return newCtrl;
    }

    bool doublePropertySpinboxHandler::ResetGUIToDefaults(PropertyDoubleSpinCtrl* GUI)
    {
        GUI->setMinimum(AZStd::numeric_limits<double>::lowest());
        GUI->setMaximum(AZStd::numeric_limits<double>::max());
        ResetGUIToDefaultsCommon(GUI);
        return true;
    }

    bool floatPropertySpinboxHandler::ResetGUIToDefaults(PropertyDoubleSpinCtrl* GUI)
    {
        GUI->setMinimum(AZStd::numeric_limits<float>::lowest());
        GUI->setMaximum(AZStd::numeric_limits<float>::max());
        doublePropertySpinboxHandler::ResetGUIToDefaultsCommon(GUI);
        return true;
    }

    void doublePropertySpinboxHandler::ConsumeAttribute(PropertyDoubleSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        doublePropertySpinboxHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        if ((GUI->maximum() > DBL_MAX) || (GUI->maximum() < -DBL_MAX) || (GUI->minimum() > DBL_MAX) || (GUI->minimum() < -DBL_MAX))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in double Spin Box exceeds Min/Max values", debugName, attrib);
        }
    }

    void floatPropertySpinboxHandler::ConsumeAttribute(PropertyDoubleSpinCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        doublePropertySpinboxHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);

        if ((GUI->maximum() > std::numeric_limits<float>::max()) || (GUI->maximum() < std::numeric_limits<float>::lowest()) || (GUI->minimum() > std::numeric_limits<float>::max()) || (GUI->minimum() < std::numeric_limits<float>::lowest()))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in float Spin Box exceeds Min/Max values", debugName, attrib);
        }
    }

    void doublePropertySpinboxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSpinCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        double val = GUI->value() / GUI->multiplier();
        instance = static_cast<property_t>(val);
    }

    void floatPropertySpinboxHandler::WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSpinCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        double val = GUI->value() / GUI->multiplier();
        instance = static_cast<property_t>(val);
    }

    bool doublePropertySpinboxHandler::ReadValuesIntoGUI(size_t index, PropertyDoubleSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        GUI->setValue(instance * GUI->multiplier());
        return false;
    }

    bool floatPropertySpinboxHandler::ReadValuesIntoGUI(size_t index, PropertyDoubleSpinCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        GUI->setValue(instance * GUI->multiplier());
        return false;
    }

    bool doublePropertySpinboxHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyDoubleSpinCtrl* propertyControl = qobject_cast<PropertyDoubleSpinCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<double>::lowest())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<double>::max())
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->maximum());
            }
            toolTipString += "]";
            return true;
        }
        return false;
    }

    bool floatPropertySpinboxHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyDoubleSpinCtrl* propertyControl = qobject_cast<PropertyDoubleSpinCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<float>::lowest())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<float>::max())
            {
                toolTipString += tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->maximum());
            }
            toolTipString += "]";
            return true;
        }
        return false;
    }

    void RegisterDoubleSpinBoxHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew doublePropertySpinboxHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew floatPropertySpinboxHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyDoubleSpinCtrl.cpp"
