/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <cmath>
#include "PropertyDoubleSliderCtrl.hxx"
#include "PropertyQTConstants.h"
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING
#include <AzCore/Math/MathUtils.h>
#include <AzQtComponents/Components/Widgets/SpinBox.h>
#include <QSignalBlocker>

namespace AzToolsFramework
{
    PropertyDoubleSliderCtrl::PropertyDoubleSliderCtrl(QWidget* pParent)
        : QWidget(pParent)
    {
        QHBoxLayout* pLayout = new QHBoxLayout(this);
        pLayout->setContentsMargins(0,0,0,0);
        m_sliderCombo = new AzQtComponents::SliderDoubleCombo(this);
        pLayout->addWidget(m_sliderCombo);
        setFocusProxy(m_sliderCombo);

        connect(m_sliderCombo, &AzQtComponents::SliderDoubleCombo::valueChanged, this, &PropertyDoubleSliderCtrl::onValueChange);
        connect(m_sliderCombo, &AzQtComponents::SliderDoubleCombo::editingFinished, this, &PropertyDoubleSliderCtrl::editingFinished);
    }

    void PropertyDoubleSliderCtrl::onValueChange()
    {
        emit valueChanged(value());
    }

    void PropertyDoubleSliderCtrl::setMinimum(double min)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setMinimum(min);
    }

    void PropertyDoubleSliderCtrl::setMaximum(double max)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setMaximum(max);
    }

    void PropertyDoubleSliderCtrl::setRange(double min, double max)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setRange(min, max);
    }

    void PropertyDoubleSliderCtrl::setStep(double val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->spinbox()->setSingleStep(val);
    }

    void PropertyDoubleSliderCtrl::setValue(double val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setValue(val);
    }

    void PropertyDoubleSliderCtrl::setMultiplier(double val)
    {
        m_multiplier = val;
    }

    void PropertyDoubleSliderCtrl::setSoftMinimum(double val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setSoftMinimum(val);
    }

    void PropertyDoubleSliderCtrl::setSoftMaximum(double val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setSoftMaximum(val);
    }

    void PropertyDoubleSliderCtrl::setPrefix(QString val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->spinbox()->setPrefix(val);
    }

    void PropertyDoubleSliderCtrl::setSuffix(QString val)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->spinbox()->setSuffix(val);
    }

    void PropertyDoubleSliderCtrl::setDecimals(int decimals)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setDecimals(decimals);
    }

    void PropertyDoubleSliderCtrl::setDisplayDecimals(int displayDecimals)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->spinbox()->setDisplayDecimals(displayDecimals);
    }

    void PropertyDoubleSliderCtrl::setCurveMidpoint(double midpoint)
    {
        QSignalBlocker block(m_sliderCombo);
        m_sliderCombo->setCurveMidpoint(midpoint);
    }

    double PropertyDoubleSliderCtrl::value() const
    {
        return m_sliderCombo->value();
    }

    double PropertyDoubleSliderCtrl::softMinimum() const
    {
        return m_sliderCombo->softMinimum();
    }

    double PropertyDoubleSliderCtrl::softMaximum() const
    {
        return m_sliderCombo->softMaximum();
    }

    double PropertyDoubleSliderCtrl::minimum() const
    {
        return m_sliderCombo->minimum();
    }

    double PropertyDoubleSliderCtrl::maximum() const
    {
        return m_sliderCombo->maximum();
    }

    double PropertyDoubleSliderCtrl::step() const
    {
        return m_sliderCombo->spinbox()->singleStep();
    }

    QWidget* PropertyDoubleSliderCtrl::GetFirstInTabOrder()
    {
        return m_sliderCombo->spinbox();
    }
    QWidget* PropertyDoubleSliderCtrl::GetLastInTabOrder()
    {
        return m_sliderCombo->slider();
    }

    void PropertyDoubleSliderCtrl::UpdateTabOrder()
    {
        setTabOrder(GetFirstInTabOrder(), GetLastInTabOrder());
    }

    void PropertyDoubleSliderCtrl::ClearSavedState()
    {
        m_sliderCombo->resetLimits();
    }

    // a common function to eat attribs, for all int handlers:
    void doublePropertySliderHandler::ConsumeAttributeCommon(PropertyDoubleSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        GUI->blockSignals(true);
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
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Min' attribute from property '%s' into Slider", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setMaximum(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Max' attribute from property '%s' into Slider", debugName);
            }
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
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Step' attribute from property '%s' into Slider", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::SoftMin)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setSoftMinimum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::SoftMax)
        {
            if (attrValue->Read<double>(value))
            {
                GUI->setSoftMaximum(value);
            }
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
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Decimals' attribute from property '%s' into Slider", debugName);
            }
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
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'DisplayDecimals' attribute from property '%s' into Slider", debugName);
            }
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
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Suffix' attribute from property '%s' into Slider", debugName);
            }
            return;
        }
        else if (attrib == AZ::Edit::Attributes::SliderCurveMidpoint)
        {
            double midpointValue = 0;
            if (attrValue->Read<double>(midpointValue))
            {
                GUI->setCurveMidpoint(midpointValue);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'SliderCurveMidpoint' attribute from property '%s' into Slider", debugName);
            }
        }

        // Verify that the bounds are within the acceptable range of the double/float slider.
        if (!std::isfinite(GUI->maximum()) || !std::isfinite(GUI->minimum()))
        {
            AZ_WarningOnce("AzToolsFramework", false, "Property '%s' : 0x%08x  in double/float Slider has a minimum or maximum that is infinite", debugName, attrib);
        }

        GUI->blockSignals(false);
    }


    QWidget* doublePropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        PropertyDoubleSliderCtrl* newCtrl = aznew PropertyDoubleSliderCtrl(pParent);
        connect(newCtrl, &PropertyDoubleSliderCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            });
        connect(newCtrl, &PropertyDoubleSliderCtrl::editingFinished, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setRange(0, 1);

        return newCtrl;
    }

    QWidget* floatPropertySliderHandler::CreateGUI(QWidget* pParent)
    {
        PropertyDoubleSliderCtrl* newCtrl = aznew PropertyDoubleSliderCtrl(pParent);
        connect(newCtrl, &PropertyDoubleSliderCtrl::valueChanged, this, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            });
        connect(newCtrl, &PropertyDoubleSliderCtrl::editingFinished, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        // note:  Qt automatically disconnects objects from each other when either end is destroyed, no need to worry about delete.

        // set defaults:
        newCtrl->setRange(0, 1);

        return newCtrl;
    }

    void doublePropertySliderHandler::ConsumeAttribute(PropertyDoubleSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        doublePropertySliderHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
    }

    void floatPropertySliderHandler::ConsumeAttribute(PropertyDoubleSliderCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        doublePropertySliderHandler::ConsumeAttributeCommon(GUI, attrib, attrValue, debugName);
    }

    void doublePropertySliderHandler::WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSliderCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        double val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    void floatPropertySliderHandler::WriteGUIValuesIntoProperty(size_t index, PropertyDoubleSliderCtrl* GUI, property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        double val = GUI->value();
        instance = static_cast<property_t>(val);
    }

    bool doublePropertySliderHandler::ReadValuesIntoGUI(size_t index, PropertyDoubleSliderCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        GUI->blockSignals(true);
        GUI->setValue(instance);
        GUI->blockSignals(false);
        return false;
    }

    bool floatPropertySliderHandler::ReadValuesIntoGUI(size_t index, PropertyDoubleSliderCtrl* GUI, const property_t& instance, InstanceDataNode* node)
    {
        AZ_UNUSED(index);
        AZ_UNUSED(node);
        GUI->blockSignals(true);
        GUI->setValue(instance);
        GUI->blockSignals(false);
        return false;
    }

    bool doublePropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyDoubleSliderCtrl* propertyControl = qobject_cast<PropertyDoubleSliderCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<int>::lowest())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<int>::max())
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

    bool floatPropertySliderHandler::ModifyTooltip(QWidget* widget, QString& toolTipString)
    {
        PropertyDoubleSliderCtrl* propertyControl = qobject_cast<PropertyDoubleSliderCtrl*>(widget);
        AZ_Assert(propertyControl, "Invalid class cast - this is not the right kind of widget!");
        if (propertyControl)
        {
            if (!toolTipString.isEmpty())
            {
                toolTipString += "\n";
            }
            toolTipString += "[";
            if (propertyControl->minimum() <= std::numeric_limits<int>::lowest())
            {
                toolTipString += "-" + tr(PropertyQTConstant_InfinityString);
            }
            else
            {
                toolTipString += QString::number(propertyControl->minimum());
            }
            toolTipString += ", ";
            if (propertyControl->maximum() >= std::numeric_limits<int>::max())
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

    void RegisterDoubleSliderHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew doublePropertySliderHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew floatPropertySliderHandler());
    }

}

#include "UI/PropertyEditor/moc_PropertyDoubleSliderCtrl.cpp"
