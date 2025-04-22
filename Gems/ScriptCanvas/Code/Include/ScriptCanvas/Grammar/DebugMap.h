/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/Datum.h>
#include <ScriptCanvas/Core/Endpoint.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    namespace Grammar
    {
        void ReflectDebugSymbols(AZ::ReflectContext* context);

        enum class DebugDataSourceType : AZ::u8
        {
            Internal,
            SelfSlot,
            OtherSlot,
            Variable,
        };

        struct DebugDataSource
        {
            AZ_TYPE_INFO(DebugDataSource, "{0F20CB1B-7AC7-4338-99A8-718B0913D359}");
            AZ_CLASS_ALLOCATOR(DebugDataSource, AZ::SystemAllocator);

            DebugDataSourceType m_sourceType;
            SlotId m_slotId;
            Data::Type m_slotDatumType;
            AZ::LuaLoadFromStack m_fromStack = nullptr;

            static const size_t k_slotIndex = 0;
            static const size_t k_variableIndex = 1;
            AZStd::variant<SlotId, VariableId> m_source;

            static DebugDataSource FromInternal();

            static DebugDataSource FromInternal(const Data::Type& dataType);

            static DebugDataSource FromSelfSlot(const Slot& localSource);

            static DebugDataSource FromSelfSlot(const Slot& localSource, const Data::Type& ifInvalidType);

            static DebugDataSource FromOtherSlot(const SlotId& slotId, const Data::Type& originalType, const SlotId& source);

            static DebugDataSource FromReturn(const Slot& slot, ExecutionTreeConstPtr execution, VariableConstPtr variable);

            static DebugDataSource FromVariable(const SlotId& slotId, const Data::Type& originalType, const VariableId& source);

            // constructs internal
            DebugDataSource();

        protected:

            // constructs local
            explicit DebugDataSource(const Slot& localSource, const Data::Type& ifInvalidType);

            // constructs from output slot
            DebugDataSource(const SlotId& slotId, const Data::Type& originalType, const SlotId& source);

            // constructs from variable
            DebugDataSource(const SlotId& slotId, const Data::Type& originalType, const VariableId& source);
        };

        // this should handle in and out, don't make more changes until you *need* to.
        // follow it down the pipe first
        struct DebugExecution
        {
            AZ_TYPE_INFO(DebugExecution, "{AE18AB4E-C359-4D85-9F1E-64F3A7262AE2}");
            AZ_CLASS_ALLOCATOR(DebugExecution, AZ::SystemAllocator);

            NamedEndpoint m_namedEndpoint;
            AZStd::vector<DebugDataSource> m_data;
        };

        struct DebugSymbolMap
        {
        public:
            AZ_TYPE_INFO(DebugSymbolMap, "{47A225DC-1B56-4C84-8CED-A5BF51E59690}");
            AZ_CLASS_ALLOCATOR(DebugSymbolMap, AZ::SystemAllocator);

            AZStd::vector<DebugExecution> m_ins;
            AZStd::vector<DebugExecution> m_outs;
            AZStd::vector<DebugExecution> m_returns;
            AZStd::vector<DebugDataSource> m_variables;
        };

        // required during translation to properly write indexes into the DebugSymbolMap
        struct DebugSymbolMapReverse
        {
        public:
            AZ_TYPE_INFO(DebugSymbolMapReverse, "{47A225DC-1B56-4C84-8CED-A5BF51E59690}");
            AZ_CLASS_ALLOCATOR(DebugSymbolMapReverse, AZ::SystemAllocator);

            AZStd::unordered_map<ExecutionTreeConstPtr, size_t> m_in;
            AZStd::unordered_map<ExecutionTreeConstPtr, AZStd::vector<size_t>> m_out;
            AZStd::unordered_map<ExecutionTreeConstPtr, size_t> m_return;
            AZStd::unordered_map<OutputAssignmentConstPtr, size_t> m_variableSets;
            // maps the assignment index to variable change debug index, since not all assignments need one
            AZStd::unordered_map<OutputAssignmentConstPtr, AZStd::unordered_map<size_t, size_t>> m_assignments;
        };
    }

}
