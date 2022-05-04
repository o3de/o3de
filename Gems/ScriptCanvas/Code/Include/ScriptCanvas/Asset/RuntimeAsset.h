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
#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Core/GraphData.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Translation/TranslationResult.h>
#include <ScriptCanvas/Variable/VariableData.h>
#include <ScriptCanvas/Execution/ExecutionContext.h>
#include <ScriptCanvas/Execution/ExecutionObjectCloning.h>
#include <ScriptCanvas/Execution/ExecutionStateDeclarations.h>

namespace ScriptCanvas
{
    namespace Execution
    {
        struct StateStorage;
    }

    class RuntimeAsset;
    struct RuntimeVariable;

    constexpr const AZ::u32 RuntimeDataSubId = AZ_CRC_CE("RuntimeData"); // 0x163310ae
    constexpr const AZ::u32 SubgraphInterfaceSubId = AZ_CRC_CE("SubgraphInterface"); // 0xdfe6dc72

    class RuntimeAssetDescription : public AssetDescription
    {
    public:

        AZ_TYPE_INFO(RuntimeAssetDescription, "{7F49CB81-0655-4AF6-A1B5-95417A6FD568}");

        RuntimeAssetDescription()
            : AssetDescription(
                azrtti_typeid<RuntimeAsset>(),
                "Script Canvas Runtime",
                "Script Canvas Runtime Graph",
                "@projectroot@/scriptcanvas",
                ".scriptcanvas_compiled",
                "Script Canvas Runtime",
                "Untitled-%i",
                "Script Canvas Files (*.scriptcanvas_compiled)",
                "Script Canvas Runtime",
                "Script Canvas Runtime",
                "Icons/ScriptCanvas/Viewport/ScriptCanvas.png",
                AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
                false
            )
        {}
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

        Translation::RuntimeInputs m_input;
        Grammar::DebugSymbolMap m_debugMap;

        // populate all and set to all to AssetLoadBehavior::Preload at build time
        AZ::Data::Asset<AZ::ScriptAsset> m_script;
        AZStd::vector<AZ::Data::Asset<RuntimeAsset>> m_requiredAssets;
        AZStd::vector<AZ::Data::Asset<ScriptEvents::ScriptEventsAsset>> m_requiredScriptEvents;

        // populate all on initial load at run time
        AZStd::function<ExecutionState*(Execution::StateStorage&, ExecutionStateConfig&)> m_createExecution;
        AZStd::vector<Execution::CloneSource> m_cloneSources;
        AZStd::vector<AZ::BehaviorValueParameter> m_activationInputStorage;
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

    class SubgraphInterfaceAsset;

    class SubgraphInterfaceAssetDescription : public AssetDescription
    {
    public:

        AZ_TYPE_INFO(SubgraphInterfaceAssetDescription, "{7F7BE1A5-9447-41C2-9190-18580075094C}");

        SubgraphInterfaceAssetDescription()
            : AssetDescription(
                azrtti_typeid<SubgraphInterfaceAsset>(),
                "Script Canvas Function Interface",
                "Script Canvas Function Interface",
                "@projectroot@/scriptcanvas",
                ".scriptcanvas_fn_compiled",
                "Script Canvas Function Interface",
                "Untitled-Function-%i",
                "Script Canvas Compiled Function Interfaces (*.scriptcanvas_fn_compiled)",
                "Script Canvas Function Interface",
                "Script Canvas Function Interface",
                "Icons/ScriptCanvas/Viewport/ScriptCanvas_Function.png",
                AZ::Color(1.0f, 0.0f, 0.0f, 1.0f),
                false
            )
        {}
    };

    struct SubgraphInterfaceData
    {
        AZ_TYPE_INFO(SubgraphInterfaceData, "{1734C569-7D40-4491-9EEE-A225E333C9BA}");
        AZ_CLASS_ALLOCATOR(SubgraphInterfaceData, AZ::SystemAllocator, 0);
        SubgraphInterfaceData() = default;
        ~SubgraphInterfaceData() = default;
        SubgraphInterfaceData(const SubgraphInterfaceData&) = default;
        SubgraphInterfaceData& operator=(const SubgraphInterfaceData&) = default;
        SubgraphInterfaceData(SubgraphInterfaceData&&);
        SubgraphInterfaceData& operator=(SubgraphInterfaceData&&);

        static void Reflect(AZ::ReflectContext* reflectContext);

        AZStd::string m_name;
        Grammar::SubgraphInterface m_interface;
    };

    class SubgraphInterfaceAsset
        : public AZ::Data::AssetData
    {
    public:
        AZ_RTTI(SubgraphInterfaceAsset, "{E22967AC-7673-4778-9125-AF49D82CAF9F}", AZ::Data::AssetData);
        AZ_CLASS_ALLOCATOR(SubgraphInterfaceAsset, AZ::SystemAllocator, 0);

        static const char* GetFileExtension() { return "scriptcanvas_fn_compiled"; }
        static const char* GetFileFilter() { return "*.scriptcanvas_fn_compiled"; }

        SubgraphInterfaceData m_interfaceData;

        SubgraphInterfaceAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(), AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : AZ::Data::AssetData(assetId, status)
        {}
    };
}
