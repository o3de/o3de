/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Script/ScriptAsset.h>
#include <AzCore/std/any.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

namespace AZ
{
    class EntityId;
}

namespace ScriptCanvas
{
    class Datum;
    class Nodeable;

    struct VariableId;

    namespace Grammar
    {
        class AbstractCodeModel;
    }

    namespace Translation
    {
        enum TargetFlags
        {
            Lua = 1 << 0,
            Cpp = 1 << 1,
            Hpp = 1 << 2,
        };

        // information required at runtime begin execution of the compiled graph from the host 
        struct RuntimeInputs
        {
            AZ_TYPE_INFO(RuntimeInputs, "{CFF0820B-EE0D-4E02-B847-2B295DD5B5CF}");
            AZ_CLASS_ALLOCATOR(RuntimeInputs, AZ::SystemAllocator, 0);

            static void Reflect(AZ::ReflectContext* reflectContext);

            Grammar::ExecutionStateSelection m_executionSelection = Grammar::ExecutionStateSelection::InterpretedPure;

            AZStd::vector<Nodeable*> m_nodeables;
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

        struct TargetResult
        {
            AZStd::string m_text;
            Grammar::SubgraphInterface m_subgraphInterface;
            RuntimeInputs m_runtimeInputs;
            Grammar::DebugSymbolMap m_debugMap;
            AZStd::sys_time_t m_duration;
        };

        using ErrorList = AZStd::vector<ValidationConstPtr>;
        using Errors = AZStd::unordered_map<TargetFlags, ErrorList>;
        using Translations = AZStd::unordered_map<TargetFlags, TargetResult>;

        AZStd::sys_time_t SumDurations(const Translations& translation);

        class Result
        {
        public:
            const AZStd::string m_invalidSourceInfo;
            const Grammar::AbstractCodeModelConstPtr m_model;
            const Translations m_translations;
            const Errors m_errors;
            const AZStd::sys_time_t m_parseDuration;
            const AZStd::sys_time_t m_translationDuration;

            Result(AZStd::string invalidSourceInfo);
            Result(Grammar::AbstractCodeModelConstPtr model);
            Result(Grammar::AbstractCodeModelConstPtr model, Translations&& translations, Errors&& errors);

            AZStd::string ErrorsToString() const;
            bool IsModelValid() const;
            bool IsSourceValid() const;
            AZ::Outcome<void, AZStd::string> IsSuccess(TargetFlags flag) const;
            bool TranslationSucceed(TargetFlags flag) const;
        };

        struct LuaAssetResult
        {
            AZ::Data::Asset<AZ::ScriptAsset> m_scriptAsset;
            RuntimeInputs m_runtimeInputs;
            Grammar::DebugSymbolMap m_debugMap;
            OrderedDependencies m_dependencies;
            AZStd::sys_time_t m_parseDuration;
            AZStd::sys_time_t m_translationDuration;
        };
    }
}
