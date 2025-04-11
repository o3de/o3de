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

#include <ScriptCanvas/Core/GraphScopedTypes.h>

namespace ScriptCanvas
{
    class GraphVariable;    
    class Node;

    // Class that will return the Datum* pointed to by a paritcular variable.
    // Will handle signalling out changes.
    //
    // Should only be held in local functions and not stored for long-term use.
    class ModifiableDatumView      
    {    
    private:
        // Ensure object cannot be copied.
        ModifiableDatumView(const ModifiableDatumView&) = delete;
        ModifiableDatumView& operator=(const ModifiableDatumView&) = delete;

        friend class Node;
        friend class GraphVariable;
        
    public:
        AZ_CLASS_ALLOCATOR(ModifiableDatumView, AZ::SystemAllocator);

        ModifiableDatumView();
        ModifiableDatumView(const AZ::EntityId& uniqueId, const VariableId& variableId);

        ~ModifiableDatumView();

        bool IsValid() const;

        bool IsType(const ScriptCanvas::Data::Type& dataType) const;
        ScriptCanvas::Data::Type GetDataType() const;
        void SetDataType(const ScriptCanvas::Data::Type& dataType);

        const Datum* GetDatum() const;
        Datum CloneDatum();

        void SetLabel(const AZStd::string& label);

        void SetToDefaultValueOfType();

        void AssignToDatum(Datum&& datum);
        void AssignToDatum(const Datum& datum);

        void ReconfigureDatumTo(Datum&& datum);
        void HardCopyDatum(const Datum& datum);

        template<typename DataType>
        void SetAs(const DataType& arg)
        {
            (*m_datum->ModAs<DataType>()) = arg;
            SignalModification();
        }

        template<typename DataType>
        void SetAs(DataType&& arg)
        {
            (*m_datum->ModAs<AZStd::remove_cvref_t<DataType>>()) = AZStd::forward<DataType>(arg);
            SignalModification();
        }

        template<typename DataType>
        const DataType* GetAs() const
        {
            return m_datum ? m_datum->GetAs<DataType>() : nullptr;
        }

        void RelabelDatum(const AZStd::string& datumName);

        void SetVisibility(AZ::Crc32 visibility);
        AZ::Crc32 GetVisibility() const;

    protected:

        Datum* ModifyDatum();

        void ConfigureView(GraphVariable& graphVariable);
        void ConfigureView(Datum& datum);
        void ConfigureView(const ScriptCanvasId& uniqueId, const VariableId& variableId);

        void SignalModification();

    private:

        Datum*     m_datum;

        bool                   m_isDirty = false;
        GraphScopedVariableId  m_scopedVariableId;
    };
}
