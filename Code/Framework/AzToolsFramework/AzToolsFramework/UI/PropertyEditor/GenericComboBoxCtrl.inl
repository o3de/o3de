/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QLayoutItem::align': class 'QFlags<Qt::AlignmentFlag>' needs to have dll-interface to be used by clients of class 'QLayoutItem'
#include <QtWidgets/QHBoxLayout>
AZ_POP_DISABLE_WARNING

#include <QPushButton>
#include <QLabel>

#include <AzToolsFramework/UI/PropertyEditor/PropertyQTConstants.h>
#include <AzToolsFramework/UI/PropertyEditor/DHQComboBox.hxx>

namespace
{
    template<typename T>
    struct Helper
    {
        static void ParseStringList(AzToolsFramework::GenericComboBoxCtrlBase* GUI, AzToolsFramework::PropertyAttributeReader* attrReader, AZStd::vector< AZStd::pair<T, AZStd::string> >& valueList)
        {
            AZ_UNUSED(GUI);
            AZ_UNUSED(attrReader);
            AZ_UNUSED(valueList);

            // Do nothing by default
        }
    };

    template<>
    struct Helper<AZStd::string>
    {
        static void ParseStringList(AzToolsFramework::GenericComboBoxCtrlBase* GUI, AzToolsFramework::PropertyAttributeReader* attrReader, AZStd::vector< AZStd::pair<AZStd::string, AZStd::string> >& valueList)
        {
            AZ_WarningOnce("AzToolsFramework", false, "Using Deprecated Attribute StringList. Please switch to GenericValueList");

            auto genericGUI = azrtti_cast<AzToolsFramework::GenericComboBoxCtrl<AZStd::string>*>(GUI);

            if (genericGUI)
            {
                AZStd::vector<AZStd::string> values;
                if (attrReader->Read<AZStd::vector<AZStd::string>>(values))
                {
                    valueList.reserve(values.size());
                    for (const AZStd::string& currentValue : values)
                    {
                        valueList.emplace_back(AZStd::make_pair(currentValue, currentValue));
                    }
                }
            }
        }
    };
}

namespace AzToolsFramework
{
    template<typename T>
    GenericComboBoxCtrl<T>::GenericComboBoxCtrl(QWidget* pParent)
        : GenericComboBoxCtrlBase(pParent)
    {
        // create the gui, it consists of a layout, and in that layout, a text field for the value
        // and then a slider for the value.
        auto* pLayout = new QHBoxLayout(this);
        m_pComboBox = aznew AzToolsFramework::DHQComboBox(this);

        m_editButton = new QToolButton();
        m_editButton->setAutoRaise(true);
        m_editButton->setToolTip(QString("Edit"));
        m_editButton->setIcon(QIcon(":/stylesheet/img/UI20/open-in-internal-app.svg"));
        m_editButton->setVisible(false);

        pLayout->setSpacing(4);
        pLayout->setContentsMargins(1, 0, 1, 0);

        pLayout->addWidget(m_pComboBox);
        pLayout->addWidget(m_editButton);

        m_pComboBox->setMinimumWidth(AzToolsFramework::PropertyQTConstant_MinimumWidth);
        m_pComboBox->setFixedHeight(AzToolsFramework::PropertyQTConstant_DefaultHeight);

        m_pComboBox->setFocusPolicy(Qt::StrongFocus);

        setLayout(pLayout);
        setFocusProxy(m_pComboBox);
        setFocusPolicy(m_pComboBox->focusPolicy());

        connect(m_pComboBox, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &GenericComboBoxCtrl<T>::onChildComboBoxValueChange);
        connect(m_editButton, &QToolButton::clicked, this, &GenericComboBoxCtrl<T>::OnEditButtonClicked);
    }

    template<typename T>
    const T& GenericComboBoxCtrl<T>::value() const
    {
        AZ_Error("AzToolsFramework", m_pComboBox->currentIndex() >= 0 &&
            m_pComboBox->currentIndex() < static_cast<int>(m_values.size()), "Out of range combo box index %d", m_pComboBox->currentIndex());
        return m_values[m_pComboBox->currentIndex()].first;
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::setValue(const T& value)
    {
        QSignalBlocker signalBlocker(m_pComboBox);
        bool indexWasFound = m_values.empty(); // Combo box might just be empty don't warn in that case
        for (size_t genericValIndex = 0; genericValIndex < m_values.size(); ++genericValIndex)
        {
            if (m_values[genericValIndex].first == value)
            {
                m_pComboBox->setCurrentIndex(static_cast<int>(genericValIndex));
                indexWasFound = true;
                break;
            }
        }
        if (!indexWasFound)
        {
            m_pComboBox->setCurrentIndex(-1);
        }

        AZ_Warning("AzToolsFramework", indexWasFound == true, "GenericValue could not be found in the combo box");

    }

    template<typename T>
    void GenericComboBoxCtrl<T>::setInvalidValue()
    {
        m_pComboBox->setCurrentIndex(-1);
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::setElements(const AZStd::vector<AZStd::pair<T, AZStd::string>>& genericValues)
    {
        m_pComboBox->clear();
        m_values.clear();
        addElements(genericValues);
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::addElements(const AZStd::vector<AZStd::pair<T, AZStd::string>>& genericValues)
    {
        QSignalBlocker signalBlocker(m_pComboBox);
        for (auto&& genericValue : genericValues)
        {
            addElementImpl(genericValue);            
        }
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::addElement(const AZStd::pair<T, AZStd::string>& genericValue)
    {
        QSignalBlocker signalBlocker(m_pComboBox);
        addElementImpl(genericValue);
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::addElementImpl(const AZStd::pair<T, AZStd::string>& genericValue)
    {
        if (AZStd::find(m_values.begin(), m_values.end(), genericValue) == m_values.end())
        {
            m_values.push_back(genericValue);
            m_pComboBox->addItem(genericValue.second.data());
        }
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::clearElements()
    {
        QSignalBlocker signalBlocker(m_pComboBox);
        m_values.clear();
        m_pComboBox->clear();
    }

    template<typename T>
    inline void GenericComboBoxCtrl<T>::onChildComboBoxValueChange(int comboBoxIndex)
    {
        if (comboBoxIndex < 0)
        {
            return;
        }

        AZ_Error("AzToolsFramework", comboBoxIndex >= 0 &&
            comboBoxIndex < static_cast<int>(m_values.size()), "Out of range combo box index %d", comboBoxIndex);

        emit valueChanged(m_values[comboBoxIndex].second);
    }

    template<typename T>
    inline void GenericComboBoxCtrl<T>::SetWarning(const AZStd::string& warningText)
    {
        if (warningText.empty() && m_warningLabel == nullptr)
        {
            return;
        }

        PrepareWarningLabel();

        m_warningLabel->setToolTip(warningText.c_str());
        m_warningLabel->setVisible(!warningText.empty());
    }

    template<typename T>
    inline void GenericComboBoxCtrl<T>::PrepareWarningLabel()
    {
        if (m_warningLabel == nullptr)
        {
            const int warningIconHeight = 18;
            m_warningLabel = new QLabel(this);
            QPixmap warningIcon(":/PropertyEditor/Resources/warning.png");
            m_warningLabel->setPixmap(warningIcon.scaledToHeight(warningIconHeight, Qt::SmoothTransformation));

            m_warningLabel->setVisible(false);

            layout()->addWidget(m_warningLabel);
        }
    }

    template<typename T>
    QComboBox* GenericComboBoxCtrl<T>::GetComboBox()
    {
        return m_pComboBox;
    }

    template<typename T>
    QToolButton* GenericComboBoxCtrl<T>::GetEditButton()
    {
        return m_editButton;
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::SetEditButtonCallBack(GenericEditButtonCallback<T> function)
    {
        m_editButtonCallback = AZStd::move(function);
    }

    template<typename T>
    void GenericComboBoxCtrl<T>::OnEditButtonClicked()
    {
        if (m_editButtonCallback)
        {
            // A successful outcome returns the new value of the comboBox
            GenericEditResultOutcome<T> outcome = m_editButtonCallback(value());
            if (outcome.IsSuccess())
            {
                setValue(outcome.GetValue());
                emit valueChanged(m_values[m_pComboBox->currentIndex()].second);
            }
        }
    }

    template<typename T>
    inline QWidget* GenericComboBoxCtrl<T>::GetFirstInTabOrder()
    {
        return m_pComboBox;
    }

    template<typename T>
    inline QWidget* GenericComboBoxCtrl<T>::GetLastInTabOrder()
    {
        return m_editButton->isVisible() ? m_editButton : GetFirstInTabOrder();
    }

    template<typename T>
    inline void GenericComboBoxCtrl<T>::UpdateTabOrder()
    {
    }

    // Property handler
    template<typename T>
    QWidget* GenericComboBoxHandler<T>::CreateGUI(QWidget* pParent)
    {
        auto newCtrl = aznew GenericComboBoxCtrl<T>(pParent);
        connect(newCtrl, &GenericComboBoxCtrl<T>::valueChanged, this, [newCtrl]()
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestWrite, newCtrl);
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&PropertyEditorGUIMessages::Bus::Handler::OnEditingFinished, newCtrl);
        });
        return newCtrl;
    }

    template<typename T>
    void GenericComboBoxHandler<T>::ConsumeAttribute(ComboBoxCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrReader, const char* debugName)
    {
        AZ_UNUSED(debugName);

        auto genericGUI = azrtti_cast<GenericComboBoxCtrl<T>*>(GUI);        
        using ValueType = AZStd::pair<typename GenericComboBoxHandler::property_t, AZStd::string>;
        using ValueTypeContainer = AZStd::vector<ValueType>;

        if (attrib == AZ::Edit::Attributes::StringList)
        {
            ValueTypeContainer valueContainer;
            Helper<T>::ParseStringList(GUI, attrReader, valueContainer);
            genericGUI->setElements(valueContainer);
        }
        else if (attrib == AZ::Edit::Attributes::GenericValue)
        {
            ValueType value;
            if (attrReader->Read<ValueType>(value))
            {
                genericGUI->addElement(value);
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'GenericValue' attribute from property '%s' into generic combo box. Expected a pair<%s, string>.", debugName, AZ::AzTypeInfo<typename GenericComboBoxHandler::property_t>::Name());
            }
        }
        else if (attrib == AZ::Edit::Attributes::GenericValueList)
        {
            ValueTypeContainer values;
            bool readSuccess = attrReader->Read<ValueTypeContainer>(values);

            if (!readSuccess)
            {
                // If the type is integral type
                // try different combinations of pair<integral type, string>
                if constexpr (AZStd::is_integral_v<T>)
                {
                    auto ReadIntegralStringVectorPair = [&values, attrReader](auto typeIdentity)
                    {
                        using IntegralType = typename decltype(typeIdentity)::type;

                        // The AZStd::vector<T, AZStd::string> has been read in before the lambda was called and failed
                        // so skip re-reading it again
                        if constexpr (AZStd::is_same_v<T, IntegralType>)
                        {
                            return false;
                        }
                        else
                        {
                            using IntegralValueType = AZStd::pair<IntegralType, AZStd::string>;
                            using IntegralValueTypeContainer = AZStd::vector<IntegralValueType>;
                            IntegralValueTypeContainer integralValues;
                            bool readContainer{};
                            if (readContainer = attrReader->Read<IntegralValueTypeContainer>(integralValues); readContainer)
                            {
                                for (auto&& [value, description] : integralValues)
                                {
                                    values.emplace_back(static_cast<T>(value), AZStd::move(description));
                                }
                            }

                            return readContainer;
                        }
                    };

                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<char>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<unsigned char>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<signed char>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<short>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<unsigned short>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<int>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<unsigned int>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<long>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<unsigned long>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<AZ::s64>{});
                    readSuccess = readSuccess || ReadIntegralStringVectorPair(AZStd::type_identity<AZ::u64>{});
                }
            }

            if (readSuccess)
            {
                genericGUI->setElements(values);
            }
            else
            {
                AZ_WarningOnce(
                    "AzToolsFramework",
                    false,
                    "Failed to read 'GenericValueList' attribute from property '%s' into generic combo box. Expected a vector of pair<%s, "
                    "string>.",
                    debugName,
                    AZ::AzTypeInfo<typename GenericComboBoxHandler::property_t>::Name());
            }
        }
        else if (attrib == AZ::Edit::Attributes::PostChangeNotify)
        {
            if (auto notifyCallback = azrtti_cast<AZ::AttributeFunction<void(const T&)>*>(attrReader->GetAttribute()))
            {
                genericGUI->m_postChangeNotifyCB = notifyCallback;
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'PostChangeNotify' attribute from property '%s' into generic combo box.", debugName);
            }
        }
        else if (attrib == AZ::Edit::Attributes::ComboBoxEditable)
        {
            bool value;
            if (attrReader->Read<bool>(value))
            {
                genericGUI->m_pComboBox->setEditable(value);
            }
            else
            {
                // emit a warning!
                AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'ComboBoxEditable' attribute from property '%s' into generic combo box", debugName);
            }
        }
        else if (attrib == AZ_CRC_CE("Warning"))
        {
            AZStd::string warningText;
            if (attrReader->Read<AZStd::string>(warningText))
            {
                genericGUI->SetWarning(warningText);
            }
        }
        else if (attrib == AZ_CRC_CE("EditButtonVisible"))
        {
            bool visible = false;
            if (attrReader->Read<bool>(visible))
            {
                genericGUI->GetEditButton()->setVisible(visible);
            }
        }
        else if (attrib == AZ_CRC_CE("EditButtonCallback"))
        {
            if (auto* editButtonInvokable = azrtti_cast<AZ::AttributeInvocable<GenericEditResultOutcome<T>(T)>*>(attrReader->GetAttribute()))
            {
                genericGUI->SetEditButtonCallBack(editButtonInvokable->GetCallable());
            };
        }
        else if (attrib == AZ_CRC_CE("EditButtonToolTip"))
        {
            AZStd::string toolTip;
            if (attrReader->Read<AZStd::string>(toolTip))
            {
                genericGUI->GetEditButton()->setToolTip(toolTip.c_str());
            }
        }
    }

    template<typename T>
    void GenericComboBoxHandler<T>::WriteGUIValuesIntoProperty(size_t index, ComboBoxCtrl* GUI, typename GenericComboBoxHandler::property_t& instance, AzToolsFramework::InstanceDataNode* node)
    {
        auto genericGUI = azrtti_cast<GenericComboBoxCtrl<T>*>(GUI);
        const auto oldValue = instance;
        instance = genericGUI->value();
        InvokePostChangeNotify(index, genericGUI, oldValue, node);
        if (index == 0)
        {
            // Reset override in case it is set to "- Multiple selected -"
            genericGUI->m_pComboBox->SetHeaderOverride("");
        }
    }

    template<typename T>
    bool GenericComboBoxHandler<T>::ReadValuesIntoGUI(size_t idx, ComboBoxCtrl* GUI, const typename GenericComboBoxHandler::property_t& instance, AzToolsFramework::InstanceDataNode*)
    {
        auto genericGUI = azrtti_cast<GenericComboBoxCtrl<T>*>(GUI);
        if (idx == 0)
        {
            // store the value from the first entity so we can test other selected entities value
            m_firstEntityValue = instance;
            genericGUI->setValue(instance);
            genericGUI->m_pComboBox->SetHeaderOverride("");
        }
        else if (instance != m_firstEntityValue) // if this value is not the same as the first entity's value set multiple
        {
            genericGUI->setInvalidValue();
            genericGUI->m_pComboBox->SetHeaderOverride("- Multiple selected -");
        }
        return true;
    }

    template<typename T>
    void GenericComboBoxHandler<T>::RegisterWithPropertySystem(AZ::DocumentPropertyEditor::PropertyEditorSystemInterface* system)
    {
        system->RegisterNodeAttribute<AZ::DocumentPropertyEditor::Nodes::PropertyEditor>(
            AZ::DocumentPropertyEditor::Nodes::PropertyEditor::PropertyEditor::GenericValueList<T>);
        system->RegisterNodeAttribute<AZ::DocumentPropertyEditor::Nodes::PropertyEditor>(
            AZ::DocumentPropertyEditor::Nodes::PropertyEditor::PropertyEditor::GenericValue<T>);
    }

    template<typename T>
    void GenericComboBoxHandler<T>::InvokePostChangeNotify(size_t index, GenericComboBoxCtrl<T>* genericGUI, const typename GenericComboBoxHandler::property_t& oldValue, AzToolsFramework::InstanceDataNode* node) const
    {
        if (genericGUI->m_postChangeNotifyCB)
        {
            void* notifyInstance = nullptr;
            const AZ::Uuid handlerTypeId = genericGUI->m_postChangeNotifyCB->GetInstanceType();
            if (!handlerTypeId.IsNull())
            {
                do
                {
                    if (handlerTypeId == node->GetClassMetadata()->m_typeId || (node->GetClassMetadata()->m_azRtti && node->GetClassMetadata()->m_azRtti->IsTypeOf(handlerTypeId)))
                    {
                        notifyInstance = node->GetInstance(index);
                        break;
                    }
                    node = node->GetParent();
                } while (node);
            }

            genericGUI->m_postChangeNotifyCB->Invoke(notifyInstance, oldValue);
        }
    }
} // namespace AzToolsFramework
