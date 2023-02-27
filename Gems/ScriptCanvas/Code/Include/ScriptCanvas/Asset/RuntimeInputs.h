/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/any.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/utils.h>
#include <ScriptCanvas/Data/DataType.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

namespace ScriptCanvas
{
    class Datum;
    class Nodeable;
    struct VariableId;

    namespace Grammar
    {
        struct ParsedRuntimeInputs;
    }

    // Information required at runtime begin execution of the compiled graph from the host
    struct RuntimeInputs
    {
        AZ_TYPE_INFO(RuntimeInputs, "{CFF0820B-EE0D-4E02-B847-2B295DD5B5CF}");
        AZ_CLASS_ALLOCATOR(RuntimeInputs, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* reflectContext);

        Grammar::ExecutionStateSelection m_executionSelection = Grammar::ExecutionStateSelection::InterpretedPure;

        AZStd::vector<Nodeable*> m_nodeables;
        // \todo Datum should be able to be cut by now, replaced by AZStd::any (and maybe TypedNullPointer if necessary)
        AZStd::vector<AZStd::pair<VariableId, Datum>> m_variables;

        // either the entityId was a (member) variable in the source graph, or it got promoted to one during parsing
        AZStd::vector<AZStd::pair<VariableId, Data::EntityIDType>> m_entityIds;

        // Statics required for internal, local values that need non-code constructible initialization,
        // when the system can't pass in the input from C++.
        AZStd::vector<AZStd::pair<VariableId, AZStd::any>> m_staticVariables;

        RuntimeInputs() = default;
        RuntimeInputs(const RuntimeInputs&) = default;
        RuntimeInputs(RuntimeInputs&&);
        ~RuntimeInputs() = default;

        void CopyFrom(const Grammar::ParsedRuntimeInputs& inputs);

        size_t GetConstructorParameterCount() const;

        RuntimeInputs& operator=(const RuntimeInputs&) = default;
        RuntimeInputs& operator=(RuntimeInputs&&);
    };
} // namespace ScriptCanvas
