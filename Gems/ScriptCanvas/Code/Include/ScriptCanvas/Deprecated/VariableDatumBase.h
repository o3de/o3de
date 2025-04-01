/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvas
{
    namespace Deprecated
    {
        class VariableDatumBase
        {
        public:
            AZ_TYPE_INFO(VariableDatumBase, "{93D2BD2B-1559-4968-B055-77736E06D3F2}");
            AZ_CLASS_ALLOCATOR(VariableDatumBase, AZ::SystemAllocator);
            static void Reflect(AZ::ReflectContext* context);

            VariableDatumBase() = default;
            explicit VariableDatumBase(const Datum& variableData);
            explicit VariableDatumBase(Datum&& variableData);
            VariableDatumBase(const Datum& value, VariableId id);
            
            bool operator==(const VariableDatumBase& rhs) const;
            bool operator!=(const VariableDatumBase& rhs) const;

            const VariableId& GetId() const { return m_id; }
            const Datum& GetData() const { return m_data; }
            Datum& GetData() { return m_data; }

            template<typename DatumType, typename ValueType>
            bool SetValueAs(ValueType&& value)
            {
                if (auto dataValueType = m_data.ModAs<DatumType>())
                {
                    (*dataValueType) = AZStd::forward<ValueType>(value);
                    OnValueChanged();
                    return true;
                }

                return false;
            }

            void SetAllowSignalOnChange(bool allowSignalChange)
            {
                m_signalValueChanges = allowSignalChange;
            }

            bool AllowsSignalOnChange() const
            {
                return m_signalValueChanges;
            }

        protected:
            void OnValueChanged();

            Datum m_data;
            VariableId m_id;

            // Certain editor functions do not need to be notified of value changes (e.g. exposed properties)
            bool m_signalValueChanges = true;

        };
    }
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::Deprecated::VariableDatumBase>
    {
        size_t operator()(const ScriptCanvas::Deprecated::VariableDatumBase& key) const
        {
            return hash<ScriptCanvas::VariableId>()(key.GetId());
        }
    };
}
