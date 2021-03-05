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

#include <QtWidgets/QLineEdit>

namespace ScriptCanvasEditor
{
    class GenericStringValidator
        : public QValidator
    {
    public:
        AZ_CLASS_ALLOCATOR(GenericStringValidator, AZ::SystemAllocator, 0);
        GenericStringValidator(const EditCtrl::StringValidatorCB& stringValidatorCB)
            : m_stringValidatorCB(stringValidatorCB)
        {}

        QValidator::State validate(QString& input, int& pos) const override
        {
            return m_stringValidatorCB ? m_stringValidatorCB(input, pos) : QValidator::State::Acceptable;
        }

    private:
        EditCtrl::StringValidatorCB m_stringValidatorCB;
    };

    template<typename T>
    GenericLineEditHandler<T>::GenericLineEditHandler(const EditCtrl::PropertyToStringCB<T>& propertyToStringCB, const EditCtrl::StringToPropertyCB<T>& stringToPropertyCB,
        const EditCtrl::StringValidatorCB& stringValidatorCB)
        : m_propertyToStringCB(propertyToStringCB)
        , m_stringToPropertyCB(stringToPropertyCB)
        , m_stringValidatorCB(stringValidatorCB)
    {
    }

    template<typename T>
    QWidget* GenericLineEditHandler<T>::CreateGUI(QWidget* pParent)
    {
        auto newCtrl = aznew GenericLineEditCtrl<T>(pParent);
        if (m_stringValidatorCB)
        {
            newCtrl->m_pLineEdit->setValidator(aznew GenericStringValidator(m_stringValidatorCB));
        }
        connect(newCtrl, &GenericLineEditCtrl<T>::valueChanged, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
        });
        return newCtrl;
    }

    template<typename T>
    void GenericLineEditHandler<T>::ConsumeAttribute(GenericLineEditCtrlBase* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrReader, const char* debugName)
    {
        (void)debugName;
        if (attrib == ScriptCanvas::Attributes::StringToProperty)
        {
            EditCtrl::StringToPropertyCB<T> value;
            if (attrReader->Read<EditCtrl::StringToPropertyCB<T>>(value))
            {
                auto genericGUI = azrtti_cast<GenericLineEditCtrl<T>*>(GUI);
                genericGUI->m_stringToPropertyCB = value;
            }
            else
            {
                AZ_WarningOnce("Script Canvas", false, "Failed to read 'StringToProperty' attribute from property '%s'. Expected a function<bool(string&, const %s&)>.", debugName, AZ::AzTypeInfo<typename GenericLineEditHandler::property_t>::Name());
            }
        }
        else if (attrib == ScriptCanvas::Attributes::PropertyToString)
        {
            EditCtrl::PropertyToStringCB<T> value;
            if (attrReader->Read<EditCtrl::PropertyToStringCB<T>>(value))
            {
                auto genericGUI = azrtti_cast<GenericLineEditCtrl<T>*>(GUI);
                genericGUI->m_propertyToStringCB = value;
            }
            else
            {
                AZ_WarningOnce("Script Canvas", false, "Failed to read 'PropertyToString' attribute from property '%s'. Expected a function<bool(%s&, string_view)>.", debugName, AZ::AzTypeInfo<typename GenericLineEditHandler::property_t>::Name());
            }
        }
    }

    template<typename T>
    void GenericLineEditHandler<T>::WriteGUIValuesIntoProperty(size_t, GenericLineEditCtrlBase* GUI, typename GenericLineEditHandler::property_t& instance, AzToolsFramework::InstanceDataNode*)
    {
        // Invoke the ctrl string -> T override if it exist otherwise attempt to invoke the handler string -> T override
        auto genericGUI = azrtti_cast<GenericLineEditCtrl<T>*>(GUI);
        if (genericGUI->m_stringToPropertyCB)
        {
            genericGUI->m_stringToPropertyCB(instance, genericGUI->value());
        }
        else if (m_stringToPropertyCB)
        {
            m_stringToPropertyCB(instance, GUI->value());
        }
    }

    template<typename T>
    bool GenericLineEditHandler<T>::ReadValuesIntoGUI(size_t, GenericLineEditCtrlBase* GUI, const typename GenericLineEditHandler::property_t& instance, AzToolsFramework::InstanceDataNode*)
    {
        // Invoke the ctrl T -> string override if it exist otherwise attempt to invoke the handler T -> string override
        auto genericGUI = azrtti_cast<GenericLineEditCtrl<T>*>(GUI);
        if (genericGUI->m_propertyToStringCB)
        {
            AZStd::string val;
            genericGUI->m_propertyToStringCB(val, instance);
            genericGUI->setValue(val);
            return true;
        }
        else if (m_propertyToStringCB)
        {
            AZStd::string val;
            m_propertyToStringCB(val, instance);
            GUI->setValue(val);
            return true;
        }
        return false;
    }
}