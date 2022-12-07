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
#include <ScriptCanvas/Asset/AssetDescription.h>
#include <ScriptCanvas/Asset/RuntimeInputs.h>
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/ExecutionObjectCloning.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>

namespace ScriptEvents
{
    class ScriptEventsAsset;
}

namespace ScriptCanvas
{
    namespace Execution
    {
        struct StateStorage;
    }

    class RuntimeAsset;
    struct RuntimeVariable;

    constexpr const AZ::u32 RuntimeDataSubId = AZ_CRC_CE("RuntimeData"); // 0x163310ae

    class RuntimeAssetDescription
        : public AssetDescription
    {
    public:

        AZ_TYPE_INFO(RuntimeAssetDescription, "{7F49CB81-0655-4AF6-A1B5-95417A6FD568}");

        RuntimeAssetDescription();
    };

    struct RuntimeData
    {
        AZ_TYPE_INFO(RuntimeData, "{A935EBBC-D167-4C59-927C-5D98C6337B9C}");
        AZ_CLASS_ALLOCATOR(RuntimeData, AZ::SystemAllocator, 0);
        RuntimeData() = default;
        ~RuntimeData() = default;
        RuntimeData(const RuntimeData&) = default;
        RuntimeData& operator=(const RuntimeData&) = default;
        RuntimeData(RuntimeData&&);
        RuntimeData& operator=(RuntimeData&&);

        static void Reflect(AZ::ReflectContext* reflectContext);

        RuntimeInputs m_input;
        Grammar::DebugSymbolMap m_debugMap;

        // populate all and set to all to AssetLoadBehavior::Preload at build time
        AZ::Data::Asset<AZ::ScriptAsset> m_script;
        AZStd::vector<AZ::Data::Asset<RuntimeAsset>> m_requiredAssets;
        AZStd::vector<AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>> m_requiredScriptEvents;

        // populate all on initial load at run time
        AZStd::function<ExecutionState*(Execution::StateStorage&, ExecutionStateConfig&)> m_createExecution;
        AZStd::vector<Execution::CloneSource> m_cloneSources;
        AZStd::vector<AZ::BehaviorArgument> m_activationInputStorage;
        Execution::ActivationInputRange m_activationInputRange;

        // used to initialize statics only once, and not necessarily on the loading thread
        // the interpreted statics require the Lua context, and so they must be initialized on the main thread
        // this may have a work around with lua_newthread, which could be done on any loading thread
        bool m_areScriptLocalStaticsInitialized = false;

        bool RequiresStaticInitialization() const;

        bool RequiresDependencyConstructionParameters() const;

    private:

        bool static RequiresDependencyConstructionParametersRecurse(const RuntimeData& data);
    };

    struct RuntimeDataOverrides
    {
        AZ_TYPE_INFO(RuntimeDataOverrides, "{CE3C0AE6-4EBA-43B2-B2D5-7AC24A194E63}");
        AZ_CLASS_ALLOCATOR(RuntimeDataOverrides, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZ::Data::Asset<RuntimeAsset> m_runtimeAsset;
        AZStd::vector<RuntimeVariable> m_variables;
        AZStd::vector<bool> m_variableIndices;
        AZStd::vector<AZ::EntityId> m_entityIds;
        AZStd::vector<RuntimeDataOverrides> m_dependencies;

        void EnforcePreloadBehavior();
    };

    enum class IsPreloadedResult
    {
        Yes,
        PreloadBehaviorNotEnforced,
        DataNotLoaded,
    };

    IsPreloadedResult IsPreloaded(const RuntimeDataOverrides& overrides);
    
    constexpr const char* ToString(IsPreloadedResult result)
    {
        switch (result)
        {
        case ScriptCanvas::IsPreloadedResult::Yes:
            return "Data are preloaded and preload behavior enforced";

        case ScriptCanvas::IsPreloadedResult::PreloadBehaviorNotEnforced:
            return "Preload behavior is NOT enforced";

        case ScriptCanvas::IsPreloadedResult::DataNotLoaded:
            return "Data are NOT loaded and ready";

        default:
            return "";
        }
    }
    
    class RuntimeAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(RuntimeAsset, "{3E2AC8CD-713F-453E-967F-29517F331784}", AZ::Data::AssetData);

        static const char* GetFileExtension() { return "scriptcanvas_compiled"; }
        static const char* GetFileFilter() { return "*.scriptcanvas_compiled"; }

        RuntimeData m_runtimeData;

        RuntimeAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {}
    };

    using RuntimeAssetPtr = AZ::Data::Asset<RuntimeAsset>;

    IsPreloadedResult IsPreloaded(RuntimeAssetPtr asset);
}
