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
#pragma once

#include <GridMate/Serialize/ContainerMarshal.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/ModifiableDatumView.h>
#include <ScriptCanvas/Variable/GraphVariable.h>

namespace ScriptCanvas
{
    class GraphVariableNetBindingTable;

    class DatumMarshaler
    {
    public:
        void SetNetBindingTable(GraphVariableNetBindingTable* netBindingTable);
        void Marshal(GridMate::WriteBuffer& wb, const Datum* const & cont) const;
        bool UnmarshalToPointer(const Datum*& target, GridMate::ReadBuffer& rb);

    private:
        template <typename T>
        void MarshalType(GridMate::WriteBuffer& wb, const Datum* const & property) const
        {
            GridMate::Marshaler<T> marshaler;
            const T* value = property->GetAs<T>();
            marshaler.Marshal(wb, *value);
        }

        template <typename T>
        bool UnmarshalType(const Datum*& target, GridMate::ReadBuffer& rb, GraphVariable* graphVariable)
        {
            bool valueChanged = false;
            ModifiableDatumView datumView;

            if (graphVariable)
            {
                graphVariable->ConfigureDatumView(datumView);

                if (datumView.IsValid())
                {
                    GridMate::Marshaler<T> marshaler;
                    T value;

                    marshaler.Unmarshal(value, rb);
                    datumView.SetAs(value);
                    target = graphVariable->GetDatum();
                    valueChanged = true;
                }
            }

            return valueChanged;
        }

    private:
        //! The network binding table is needed to determine which Datum to update
        //! when unmarshaling data. 
        //  :SCTODO: synced Datums should be tracked via ID
        //! and that ID should be used to lookup Datums (right now we can assume
        //! which Datum should be updated, since only one Datum is supported).
        GraphVariableNetBindingTable* m_graphVariableNetBindingTable = nullptr;
    };

    //! Simple throttler that simple operates via dirty flag.
    class DatumThrottler
    {
    public:
        DatumThrottler() = default;

        void SignalDirty();
        bool WithinThreshold(const Datum* newValue) const;
        void UpdateBaseline(const Datum* baseline);

    private:
        bool m_isDirty = false;
    };
}
