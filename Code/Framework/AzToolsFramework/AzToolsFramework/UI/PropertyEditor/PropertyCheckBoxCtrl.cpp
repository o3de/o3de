/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PropertyCheckBoxCtrl.hxx"
#include "PropertyQTConstants.h"

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING


#include <AzQtComponents/Components/Widgets/CheckBox.h>

namespace AzToolsFramework
{
    PropertyCheckBoxCtrl::PropertyCheckBoxCtrl(QWidget* parent)
        : QWidget(parent)
    {
        // create the gui, it consists of a layout, and checkbox in that layout
        QHBoxLayout* layout = new QHBoxLayout(this);
        m_checkBox = new QCheckBox(this);

        layout->setContentsMargins(0, 0, 0, 0);

        layout->addWidget(m_checkBox);

        m_checkBox->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        m_checkBox->setMinimumWidth(PropertyQTConstant_MinimumWidth);
        m_checkBox->setFixedHeight(PropertyQTConstant_DefaultHeight);

        m_checkBox->setFocusPolicy(Qt::StrongFocus);

        AzQtComponents::CheckBox::applyToggleSwitchStyle(m_checkBox);

        setLayout(layout);
        setFocusProxy(m_checkBox);
        setFocusPolicy(m_checkBox->focusPolicy());

        connect(m_checkBox, SIGNAL(stateChanged(int)), this, SLOT(onStateChanged(int)));
    };

    void PropertyCheckBoxCtrl::setValue(bool value)
    {
        m_checkBox->blockSignals(true);
        m_checkBox->setCheckState(value ? Qt::Checked : Qt::Unchecked);
        m_checkBox->blockSignals(false);
    }

    bool PropertyCheckBoxCtrl::value() const
    {
        return m_checkBox->checkState() == Qt::Checked;
    }

    void PropertyCheckBoxCtrl::onStateChanged(int newValue)
    {
        emit valueChanged(newValue == Qt::Unchecked ? false : true);
    }

    QWidget* PropertyCheckBoxCtrl::GetFirstInTabOrder()
    {
        return m_checkBox;
    }
    QWidget* PropertyCheckBoxCtrl::GetLastInTabOrder()
    {
        return m_checkBox;
    }

    void PropertyCheckBoxCtrl::UpdateTabOrder()
    {
        // There's only one QT widget on this property.
    }

    void PropertyCheckBoxCtrl::SetCheckBoxToolTip(const char* description)
    {
        m_checkBox->setToolTip(QString::fromUtf8(description));
    }

    QWidget* CheckBoxHandlerCommon::CreateGUICommon(QWidget* parent)
    {
        PropertyCheckBoxCtrl* newCtrl = aznew PropertyCheckBoxCtrl(parent);
        connect(newCtrl, &PropertyCheckBoxCtrl::valueChanged, this, [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &PropertyEditorGUIMessages::RequestWrite, newCtrl);
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(
                    &PropertyEditorGUIMessages::OnEditingFinished, newCtrl);
            });
        return newCtrl;
    }

    void CheckBoxHandlerCommon::ResetValueCommon(PropertyCheckBoxCtrl* widget)
    {
        widget->setValue(false);
        widget->SetCheckBoxToolTip("");
    }

    void CheckBoxHandlerCommon::ConsumeAttributeCommon(
        PropertyCheckBoxCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* /*debugName*/)
    {
        if (attrib == AZ::Edit::Attributes::CheckboxTooltip)
        {
            AZStd::string tooltip;
            attrValue->Read<AZStd::string>(tooltip);
            if (!tooltip.empty())
            {
                widget->SetCheckBoxToolTip(tooltip.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::CheckboxDefaultValue)
        {
            bool value = false;
            if (attrValue->Read<bool>(value))
            {
                widget->setValue(value);
            }
        }
    }

    template<class ValueType>
    void PropertyCheckBoxHandlerCommon<ValueType>::ConsumeAttribute(
        PropertyCheckBoxCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(widget, attrib, attrValue, debugName);
    }

    QWidget* BoolPropertyCheckBoxHandler::CreateGUI(QWidget* parent)
    {
        return CreateGUICommon(parent);
    }

    void BoolPropertyCheckBoxHandler::WriteGUIValuesIntoProperty(
        size_t /*index*/, PropertyCheckBoxCtrl* widget, property_t& instance, InstanceDataNode* /*node*/)
    {
        bool val = widget->value();
        instance = static_cast<property_t>(val);
    }

    bool BoolPropertyCheckBoxHandler::ReadValuesIntoGUI(
        size_t /*index*/, PropertyCheckBoxCtrl* widget, const property_t& instance, InstanceDataNode* /*node*/)
    {
        bool val = instance;
        widget->setValue(val);
        return false;
    }

    QWidget* CheckBoxGenericHandler::CreateGUI(QWidget* parent)
    {
        return CreateGUICommon(parent);
    }

    void CheckBoxGenericHandler::ConsumeAttribute(
        PropertyCheckBoxCtrl* widget, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName)
    {
        ConsumeAttributeCommon(widget, attrib, attrValue, debugName);
    }

    void CheckBoxGenericHandler::WriteGUIValuesIntoProperty(
        size_t /*index*/, PropertyCheckBoxCtrl* /*widget*/, void* /*value*/, const AZ::Uuid& /*propertyType*/)
    {}

    bool CheckBoxGenericHandler::ReadValueIntoGUI(
        size_t /*index*/, PropertyCheckBoxCtrl* /*widget*/, void* /*value*/, const AZ::Uuid& /*propertyType*/)
    {
        return false;
    }

    void RegisterCheckBoxHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew BoolPropertyCheckBoxHandler());

        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::RegisterPropertyType, aznew CheckBoxGenericHandler());
    }
}

#include "UI/PropertyEditor/moc_PropertyCheckBoxCtrl.cpp"
