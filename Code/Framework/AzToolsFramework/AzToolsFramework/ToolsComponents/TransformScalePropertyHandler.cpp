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

#include "AzToolsFramework_precompiled.h"
#include <ToolsComponents/TransformScalePropertyHandler.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

namespace AzToolsFramework
{
    void RegisterTransformScaleHandler()
    {
        PropertyTypeRegistrationMessages::Bus::Broadcast(&PropertyTypeRegistrationMessages::RegisterPropertyType, aznew Components::TransformScalePropertyHandler());
    }

    namespace Components
    {
        AZ::u32 TransformScalePropertyHandler::GetHandlerName(void) const
        {
            return TransformScaleHandler;
        }

        QWidget* TransformScalePropertyHandler::CreateGUI(QWidget* parent)
        {
            AzQtComponents::DoubleSpinBox* newCtrl = new AzQtComponents::DoubleSpinBox(parent);
            connect(newCtrl, QOverload<double>::of(&AzQtComponents::DoubleSpinBox::valueChanged), newCtrl, [newCtrl]()
                {
                    AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
                });

            newCtrl->setMinimum(AZ::MinTransformScale);
            newCtrl->setMaximum(AZ::MaxTransformScale);

            return newCtrl;
        }

        void TransformScalePropertyHandler::ConsumeAttribute(AzQtComponents::DoubleSpinBox* GUI, AZ::u32 attrib,
            AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
        {
            if (attrib == AZ::Edit::Attributes::Suffix)
            {
                AZStd::string label;
                if (attrValue->Read<AZStd::string>(label))
                {
                    GUI->setSuffix(label.c_str());
                }
            }
        }

        void TransformScalePropertyHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AzQtComponents::DoubleSpinBox* GUI,
            AZ::Vector3& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            const float value = aznumeric_cast<float>(GUI->value());
            const float currentMaxElement = instance.GetMaxElement();
            if (currentMaxElement != 0.0f)
            {
                instance *= value / currentMaxElement;
            }
            else
            {
                instance = AZ::Vector3(value);
            }
        }

        bool TransformScalePropertyHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AzQtComponents::DoubleSpinBox* GUI,
            const AZ::Vector3& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(GUI);
            GUI->setValue(instance.GetMaxElement());
            return true;
        }
    } // namespace Components
} // namespace AzToolsFramework
