/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef PROPERTY_ENUMCOMBOBOX_CTRL
#define PROPERTY_ENUMCOMBOBOX_CTRL

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Casting/numeric_cast.h>
#include "PropertyEditorAPI.h"
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>

#include <AzToolsFramework/UI/PropertyEditor/GenericComboBoxCtrl.h>

#include <QWidget>
#endif

class QComboBox;
class QLineEdit;
class QPushButton;

namespace AzToolsFramework
{
    class PropertyEnumComboBoxCtrl
        : public QWidget
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(PropertyEnumComboBoxCtrl, AZ::SystemAllocator);

        PropertyEnumComboBoxCtrl(QWidget* pParent = NULL);
        virtual ~PropertyEnumComboBoxCtrl();

        AZ::s64 value() const;
        void addEnumValue(AZStd::pair<AZ::s64, AZStd::string>& val);
        void setEnumValues(AZStd::vector<AZStd::pair<AZ::s64, AZStd::string> >& vals);

        QWidget* GetFirstInTabOrder();
        QWidget* GetLastInTabOrder();
        void UpdateTabOrder();

    signals:
        void valueChanged(AZ::s64 newValue);

    public slots:
        void setValue(AZ::s64 val);

    protected slots:
        void onChildComboBoxValueChange(int comboBoxIndex);

    private:
        QComboBox* m_pComboBox;
        AZStd::vector< AZStd::pair<AZ::s64, AZStd::string>  > m_enumValues;
    };

    template <class ValueType>
    class GenericEnumPropertyComboBoxHandler
        : public GenericComboBoxHandler<ValueType>
    {
        virtual void ConsumeParentAttribute(GenericComboBoxCtrlBase* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override
        {
            // Simply re-route to ConsumeAttribute since no special logic is needed.
            ConsumeAttribute(GUI, attrib, attrValue, debugName);
        }

        virtual void ConsumeAttribute(GenericComboBoxCtrlBase* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override
        {
            (void)debugName;

            auto genericGUI = azrtti_cast<GenericComboBoxCtrl<ValueType>*>(GUI);

            // Forward EnumValue fields from the registered enum specified by EnumType
            if (attrib == AZ::Edit::InternalAttributes::EnumType)
            {
                AZ::Uuid typeId;
                if (attrValue->Read<AZ::Uuid>(typeId))
                {
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

                    if (serializeContext)
                    {
                        if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                        {
                            const AZ::Edit::ElementData* data = editContext->GetEnumElementData(typeId);
                            if (data)
                            {
                                for (const AZ::AttributePair& attributePair : data->m_attributes)
                                {
                                    PropertyAttributeReader reader(attrValue->GetInstance(), attributePair.second);
                                    ConsumeAttribute(GUI, attributePair.first, &reader, debugName);
                                }
                            }
                        }
                    }
                }
            }
            else if (attrib == AZ_CRC_CE("EnumValue"))
            {
                AZStd::pair<ValueType, AZStd::string>  enumValue;
                AZ::Edit::EnumConstant<ValueType> enumConstant;
                if (attrValue->Read<AZ::Edit::EnumConstant<ValueType>>(enumConstant))
                {
                    enumValue.first = ValueType(enumConstant.m_value);
                    enumValue.second = enumConstant.m_description;

                    genericGUI->addElement(enumValue);
                }
                else
                {
                    // Legacy path. Support temporarily for compatibility.
                    if (attrValue->Read< AZStd::pair<ValueType, AZStd::string> >(enumValue))
                    {
                        genericGUI->addElement(enumValue);
                    }
                    else
                    {
                        AZStd::pair<ValueType, const char*>  enumValueConst;
                        if (attrValue->Read< AZStd::pair<ValueType, const char*> >(enumValueConst))
                        {
                            AZStd::string consted = enumValueConst.second;

                            enumValue.first = enumValueConst.first;
                            enumValue.second = consted;

                            genericGUI->addElement(enumValue);
                        }
                        else
                        {
                            AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EnumValue' attribute from property '%s' into enum combo box. Expected pair<IntegerType, char*> or pair<IntegerType, AZStd::string>, where IntegerType is int or u32 ", debugName);
                        }
                    }
                }
            }
            else if (attrib == AZ::Edit::Attributes::EnumValues)
            {
                AZStd::vector<AZStd::pair<ValueType, AZStd::string> > enumValues;
                AZStd::vector<AZ::Edit::EnumConstant<ValueType>> enumConstantValues;

                if (attrValue->Read<AZStd::vector<AZ::Edit::EnumConstant<ValueType>>>(enumConstantValues))
                {
                    for (const AZ::Edit::EnumConstant<ValueType>& constantValue : enumConstantValues)
                    {
                        enumValues.emplace_back(static_cast<ValueType>(constantValue.m_value), constantValue.m_description);
                    }
                    genericGUI->setElements(enumValues);
                }
                else
                {
                    // Legacy path. Support temporarily for compatibility.
                    if (!attrValue->Read< AZStd::vector<AZStd::pair<ValueType, AZStd::string> > >(enumValues))
                    {
                        AZStd::vector<AZStd::pair<ValueType, const char*> > attempt2;
                        if (attrValue->Read<AZStd::vector<AZStd::pair<ValueType, const char*> > >(attempt2))
                        {
                            enumValues.reserve(attempt2.size());

                            for (const auto& secondPair : attempt2)
                            {
                                enumValues.emplace_back(secondPair.first, secondPair.second);
                            }
                        }
                        else
                        {
                            // emit a warning!
                            AZ_WarningOnce("AzToolsFramework", false, "Failed to read 'EnumValue' attribute from property '%s' into enum combo box", debugName);
                        }                        
                    }

                    genericGUI->setElements(enumValues);
                }
            }
            else
            {
                GenericComboBoxHandler<ValueType>::ConsumeAttribute(GUI, attrib, attrValue, debugName);
            }
        }

        bool HandlesType(const AZ::Uuid& id) const override
        {
            const AZ::TypeId& handledTypeId = this->GetHandledType();
            if (handledTypeId == id)
            {
                return true;
            }
            // Attempt to check if the supplied "id" is an enum type whose underlying type
            // matches the ValueType type
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            const AZ::TypeId& underlyingIdType = serializeContext ? serializeContext->GetUnderlyingTypeId(id) : id;
            return handledTypeId == underlyingIdType;
        }

    };

    class charEnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<char>
    {
    public:
        AZ_CLASS_ALLOCATOR(charEnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    class s8EnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<AZ::s8>
    {
    public:
        AZ_CLASS_ALLOCATOR(s8EnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    class u8EnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<AZ::u8>
    {
    public:
        AZ_CLASS_ALLOCATOR(u8EnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    class s16EnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<AZ::s16>
    {
    public:
        AZ_CLASS_ALLOCATOR(s16EnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    class u16EnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<AZ::u16>
    {
    public:
        AZ_CLASS_ALLOCATOR(u16EnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    // Default: int
    class defaultEnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<int>
    {
    public:
        AZ_CLASS_ALLOCATOR(defaultEnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    class u32EnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<AZ::u32>
    {
    public:
        AZ_CLASS_ALLOCATOR(u32EnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    class s64EnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<AZ::s64>
    {
    public:
        AZ_CLASS_ALLOCATOR(s64EnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    class u64EnumPropertyComboBoxHandler
        : public GenericEnumPropertyComboBoxHandler<AZ::u64>
    {
    public:
        AZ_CLASS_ALLOCATOR(u64EnumPropertyComboBoxHandler, AZ::SystemAllocator);
    };

    void RegisterEnumComboBoxHandler();
};

#endif
