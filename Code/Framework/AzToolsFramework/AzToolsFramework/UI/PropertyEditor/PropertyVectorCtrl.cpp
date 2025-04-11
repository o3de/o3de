/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "PropertyVectorCtrl.hxx"
#include "PropertyQTConstants.h"
#include <AzQtComponents/Components/Widgets/SpinBox.h>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING
#include <QtWidgets/QLabel>
#include <AzCore/Math/Transform.h>
#include <cmath>

namespace AzToolsFramework
{
    AzQtComponents::VectorInput* VectorPropertyHandlerCommon::ConstructGUI(QWidget* parent) const
    {
        AzQtComponents::VectorInput* newCtrl = new AzQtComponents::VectorInput(parent, m_elementCount, m_elementsPerRow, m_customLabels.c_str());
        QObject::connect(newCtrl, &AzQtComponents::VectorInput::valueChanged, newCtrl, [newCtrl]()
            {
                PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Events::RequestWrite, newCtrl);
            });
        newCtrl->connect(newCtrl, &AzQtComponents::VectorInput::editingFinished, [newCtrl]()
        {
            PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });

        newCtrl->setMinimum(-std::numeric_limits<float>::max());
        newCtrl->setMaximum(std::numeric_limits<float>::max());

        return newCtrl;
    }

    void VectorPropertyHandlerCommon::ConsumeAttributes(AzQtComponents::VectorInput* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) const
    {
        if (attrib == AZ::Edit::Attributes::Suffix)
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->setSuffix(label.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::LabelForX)
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->setLabel(0, label.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::LabelForY)
        {
            AZStd::string label;
            if (attrValue->Read<AZStd::string>(label))
            {
                GUI->setLabel(1, label.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::StyleForX)
        {
            AZStd::string qss;
            if (attrValue->Read<AZStd::string>(qss))
            {
                GUI->setLabelStyle(0, qss.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::StyleForY)
        {
            AZStd::string qss;
            if (attrValue->Read<AZStd::string>(qss))
            {
                GUI->setLabelStyle(1, qss.c_str());
            }
        }
        else if (attrib == AZ::Edit::Attributes::Min)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->setMinimum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Max)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->setMaximum(value);
            }
        }
        else if (attrib == AZ::Edit::Attributes::Step)
        {
            double value;
            if (attrValue->Read<double>(value))
            {
                GUI->setStep(value);
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
                // debugName is unused in release...
                Q_UNUSED(debugName);

                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'Decimals' attribute from property '%s' into Vector", debugName);
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
                // debugName is unused in release...
                Q_UNUSED(debugName);

                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'DisplayDecimals' attribute from property '%s' into Vector", debugName);
            }
            return;
        }

        if (m_elementCount > 2)
        {
            if (attrib == AZ::Edit::Attributes::LabelForZ)
            {
                AZStd::string label;
                if (attrValue->Read<AZStd::string>(label))
                {
                    GUI->setLabel(2, label.c_str());
                }
            }
            else if (attrib == AZ::Edit::Attributes::StyleForZ)
            {
                AZStd::string qss;
                if (attrValue->Read<AZStd::string>(qss))
                {
                    GUI->setLabelStyle(2, qss.c_str());
                }
            }

            if (m_elementCount > 3)
            {
                if (attrib == AZ::Edit::Attributes::LabelForW)
                {
                    AZStd::string label;
                    if (attrValue->Read<AZStd::string>(label))
                    {
                        GUI->setLabel(3, label.c_str());
                    }
                }
                else if (attrib == AZ::Edit::Attributes::StyleForW)
                {
                    AZStd::string qss;
                    if (attrValue->Read<AZStd::string>(qss))
                    {
                        GUI->setLabelStyle(3, qss.c_str());
                    }
                }
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    void QuaternionPropertyHandler::WriteGUIValuesIntoProperty(size_t, AzQtComponents::VectorInput* GUI, AZ::Quaternion& instance, InstanceDataNode*)
    {
        AzQtComponents::VectorElement** elements = GUI->getElements();

        AZ::Vector3 eulerRotation(static_cast<float>(elements[0]->getValue()), static_cast<float>(elements[1]->getValue()), static_cast<float>(elements[2]->getValue()));
        AZ::Quaternion newValue;
        newValue.SetFromEulerDegrees(eulerRotation);

        instance = static_cast<AZ::Quaternion>(newValue);
    }

    bool QuaternionPropertyHandler::ReadValuesIntoGUI(size_t, AzQtComponents::VectorInput* GUI, const AZ::Quaternion& instance, InstanceDataNode*)
    {
        GUI->blockSignals(true);
        
        AZ::Vector3 eulerRotation = instance.GetEulerDegrees();

        for (int idx = 0; idx < m_common.GetElementCount(); ++idx)
        {
            GUI->setValuebyIndex(eulerRotation.GetElement(idx), idx);
        }

        GUI->blockSignals(false);
        return false;
    }

    //////////////////////////////////////////////////////////////////////////

    void RegisterVectorHandlers()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, new Vector2PropertyHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, new Vector3PropertyHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, new Vector4PropertyHandler());
        PropertyTypeRegistrationMessages::Bus::Broadcast(
            &PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, new QuaternionPropertyHandler());
    }

}
