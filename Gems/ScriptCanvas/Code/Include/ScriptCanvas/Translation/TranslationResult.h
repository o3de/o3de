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
#include <ScriptCanvas/Asset/RuntimeInputs.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Data/Data.h>
#include <ScriptCanvas/Debugger/ValidationEvents/ValidationEvent.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Grammar/PrimitivesDeclarations.h>

namespace ScriptCanvas
{
    namespace Translation
    {
        enum TargetFlags
        {
            Lua = 1 << 0,
            Cpp = 1 << 1,
            Hpp = 1 << 2,
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
