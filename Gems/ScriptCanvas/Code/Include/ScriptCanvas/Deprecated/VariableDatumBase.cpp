/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <ScriptCanvas/Deprecated/VariableDatum.h>
#include <ScriptCanvas/Variable/VariableBus.h>

namespace ScriptCanvas
{
    namespace Deprecated
    {
        void VariableDatumBase::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<VariableDatumBase>()
                    ->Version(0)
                    ->Field("m_data", &VariableDatumBase::m_data)
                    ->Field("m_variableId", &VariableDatumBase::m_id)
                    ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &VariableId::MakeVariableId)
                    ;

                if (auto editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<VariableDatumBase>("Variable", "Represents a Variable field within a Script Canvas Graph")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->DataElement(AZ::Edit::UIHandlers::Default, &VariableDatumBase::m_data, "Datum", "Datum within Script Canvas Graph")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                        ->Attribute(AZ::Edit::Attributes::ChangeNotify, &VariableDatumBase::OnValueChanged)
                        ;
                }
            }
        }
        VariableDatumBase::VariableDatumBase(const Datum& datum)
            : m_data(datum)
            , m_id(VariableId::MakeVariableId())
        {}

        VariableDatumBase::VariableDatumBase(const Datum& value, VariableId id)
            : m_data(value)
            , m_id(id)
        {}

        VariableDatumBase::VariableDatumBase(Datum&& datum)
            : m_data(AZStd::move(datum))
            , m_id(VariableId::MakeVariableId())
        {}

        bool VariableDatumBase::operator==(const VariableDatumBase& rhs) const
        {
            if (this == &rhs)
            {
                return true;
            }

            return (m_data == rhs.m_data).IsSuccess();
        }

        bool VariableDatumBase::operator!=(const VariableDatumBase& rhs) const
        {
            return !operator==(rhs);
        }

        void VariableDatumBase::OnValueChanged()
        {
        }
    }
}
