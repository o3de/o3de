/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/InertiaPropertyHandler.h>
#include <AzCore/Math/Matrix3x3.h>

namespace PhysX
{
    namespace Editor
    {
        AZ::u32 InertiaPropertyHandler::GetHandlerName(void) const
        {
            return InertiaHandler;
        }

        QWidget* InertiaPropertyHandler::CreateGUI(QWidget* parent)
        {
            AzQtComponents::VectorInput* newCtrl = new AzQtComponents::VectorInput(parent, 3, -1, "");
            connect(newCtrl, &AzQtComponents::VectorInput::valueChanged, newCtrl, [newCtrl]()
            {
                AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            });

            newCtrl->setMinimum(0.0f);
            newCtrl->setMaximum(std::numeric_limits<float>::max());

            return newCtrl;
        }

        void InertiaPropertyHandler::ConsumeAttribute(AzQtComponents::VectorInput* GUI, AZ::u32 attrib,
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

        void InertiaPropertyHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, AzQtComponents::VectorInput* GUI,
            AZ::Matrix3x3& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            AzQtComponents::VectorElement** elements = GUI->getElements();

            AZ::Vector3 diagonalElements(aznumeric_cast<float>(elements[0]->getValue()), aznumeric_cast<float>(elements[1]->getValue()), aznumeric_cast<float>(elements[2]->getValue()));
            instance = AZ::Matrix3x3::CreateDiagonal(diagonalElements);
        }

        bool InertiaPropertyHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, AzQtComponents::VectorInput* GUI,
            const AZ::Matrix3x3& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
        {
            QSignalBlocker signalBlocker(GUI);
            AZ::Vector3 diagonalElements = instance.GetDiagonal();
            for (int idx = 0; idx < 3; ++idx)
            {
                GUI->setValuebyIndex(diagonalElements.GetElement(idx), idx);
            }
            return true;
        }
    } // namespace Editor
} // namespace PhysX
