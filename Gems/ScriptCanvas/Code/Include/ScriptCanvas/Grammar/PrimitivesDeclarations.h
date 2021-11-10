/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/enable_shared_from_this.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace ScriptCanvas
{
    class Graph;
    class Node;
    class Slot;
    class VariableData;

    enum class BuildConfiguration
    {
        Debug,          // C++ -> all others, debug information is available on request, no performance markers
        Performance,    // C++ -> PERFORMANCE_BUILD, no debug information, performance markers in place
        Release,        // C++ -> _RELEASE, no debug information, no performance markers
    };

    enum class ExecutionConfiguration
    {
        Debug,          // debug information is available, no performance marking
        Performance,    // no debug information, performance markers active
        Release,        // no debug information, no performance marking
        Traced          // debug information active, no performance marking
    };

    struct GraphData;

    using NamespacePath = AZStd::vector<AZStd::string>;

    namespace Data
    {
        class Type;
    }

    namespace Grammar
    {
        constexpr const char* k_luaSpecialCharacters = "\\//+-*^%#~={}[]();:,.\'\" ";

        constexpr const char* k_UnusedVariableName = "unused";

        constexpr const char* k_CloneSourceFunctionName = "CloneSourceObject";

        constexpr const char* k_DebugRuntimeErrorName = "DebugRuntimeError";
        constexpr const char* k_DeactivateName = "Deactivate";
        constexpr const char* k_DebugIsTracedName = "DebugIsTraced";
        constexpr const char* k_DebugSignalInName = "DEBUG_SIGNAL_IN";
        constexpr const char* k_DebugSignalInSubgraphName = "DEBUG_SIGNAL_IN_SUBGRAPH";
        constexpr const char* k_DebugSignalOutName = "DEBUG_SIGNAL_OUT";
        constexpr const char* k_DebugSignalOutSubgraphName = "DEBUG_SIGNAL_OUT_SUBGRAPH";
        constexpr const char* k_DebugSignalReturnName = "DEBUG_SIGNAL_RETURN";
        constexpr const char* k_DebugSignalReturnSubgraphName = "DEBUG_SIGNAL_RETURN_SUBGRAPH";
        constexpr const char* k_DebugVariableChangeName = "DEBUG_VARIABLE_CHANGE";
        constexpr const char* k_DebugVariableChangeSubgraphName = "DEBUG_VARIABLE_CHANGE_SUBGRAPH";

        constexpr const char* k_DependencySuffix = "_dp";

        constexpr const char* k_GetRandomSwitchControlNumberName = "GetRandomSwitchControlNumber";

        constexpr const char* k_EBusHandlerConnectName = "EBusHandlerConnect";
        constexpr const char* k_EBusHandlerConnectToName = "EBusHandlerConnectTo";
        constexpr const char* k_EBusHandlerCreateAndConnectName = "EBusHandlerCreateAndConnect";
        constexpr const char* k_EBusHandlerCreateAndConnectToName = "EBusHandlerCreateAndConnectTo";
        constexpr const char* k_EBusHandlerCreateName = "EBusHandlerCreate";
        constexpr const char* k_EBusHandlerDisconnectName = "EBusHandlerDisconnect";
        constexpr const char* k_EBusHandlerHandleEventName = "EBusHandlerHandleEvent";
        constexpr const char* k_EBusHandlerHandleEventResultName = "EBusHandlerHandleEventResult";
        constexpr const char* k_EBusHandlerIsConnectedName = "EBusHandlerIsConnected";
        constexpr const char* k_EBusHandlerIsConnectedToName = "EBusHandlerIsConnectedTo";

        // AZ Event name
        constexpr const char* k_AzEventHandlerConnectName = "Connect";
        constexpr const char* k_AzEventHandlerDisconnectName = "Disconnect";

        constexpr const char* k_LuaEpsilonString = "0.000001";

        constexpr const char* k_executionStateVariableName = "executionState";
        constexpr const char* k_ebusHandlerThisPointerName = "ebusHandlerThis";

        constexpr const char* k_InitializeStaticsName = "InitializeStatics";
        constexpr const char* k_InitializeNodeableOutKeys = "InitializeNodeableOutKeys";
        constexpr const char* k_InitializeExecutionOutByRequiredCountName = "InitializeExecutionOutByRequiredCount";
        constexpr const char* k_InterpretedConfigurationPerformance = "SCRIPT_CANVAS_GLOBAL_PERFORMANCE";
        constexpr const char* k_InterpretedConfigurationRelease = "SCRIPT_CANVAS_GLOBAL_RELEASE";

        constexpr const char* k_NodeableCallInterpretedOut = "ExecutionOut";
        constexpr const char* k_NodeableUserBaseClassName = "Nodeable";
        constexpr const char* k_NodeableSetExecutionOutName = "SetExecutionOut";
        constexpr const char* k_NodeableSetExecutionOutResultName = "SetExecutionOutResult";
        constexpr const char* k_NodeableSetExecutionOutUserSubgraphName = "SetExecutionOutUserSubgraph";

        constexpr const char* k_TypeSafeEBusResultName = "TypeSafeEBusResult";
        constexpr const char* k_TypeSafeEBusMultipleResultsName = "TypeSafeEBusMultipleResults";

        constexpr const char* k_OnGraphStartFunctionName = "OnGraphStart";
        constexpr const char* k_OverrideNodeableMetatableName = "OverrideNodeableMetatable";

        constexpr const char* k_stringFormatLexicalScopeName = "string";
        constexpr const char* k_stringFormatName = "format";

        constexpr const char* k_MetaTableSuffix = "_Instance_MT";

        constexpr const char* k_printLexicalScopeName = "Debug";
        constexpr const char* k_printName = "Log";

        constexpr const char* k_internalRuntimeSuffix = "_VM";
        constexpr const char* k_internalRuntimeSuffixLC = "_vm";
        constexpr const char* k_reservedWordProtection = "_scvm";

        constexpr const char* k_memberNamePrefix = "m_";

        constexpr const char* k_DependentAssetsArgName = "dependentAssets";
        constexpr const char* k_DependentAssetsIndexArgName = "dependentAssetsIndex";
        constexpr const char* k_UnpackDependencyConstructionArgsFunctionName = "UnpackDependencyConstructionArgs";
        constexpr const char* k_UnpackDependencyConstructionArgsLeafFunctionName = "UnpackDependencyConstructionArgsLeaf";

        enum class ExecutionCharacteristics : AZ::u32
        {
            Object,
            Pure,
        };

        // default to a pure, interpreted function
        enum class ExecutionStateSelection : AZ::u32
        {
            InterpretedPure,
            InterpretedPureOnGraphStart,
            InterpretedObject,
            InterpretedObjectOnGraphStart,
        };

        enum class VariableConstructionRequirement
        {
            InputEntityId,
            InputNodeable,
            InputVariable,
            None,
            Static,
        };

        // create Symbol enum
#define REGISTER_ENUM(x) x,
        enum class Symbol : AZ::u32
        {
#include "SymbolNames.h"
            Count,
        };
#undef REGISTER_ENUM

        // create the Symbol strings
#define REGISTER_ENUM(x) #x,
        static const char* g_SymbolNames[] =
        {
#include    "SymbolNames.h"
            "<ERROR>"
        };
#undef REGISTER_ENUM

        class AbstractCodeModel;

        struct DebugInfo;
        struct Dependency;
        struct EBusHandling;
        struct EventHandling;
        struct ExecutionId;
        struct ExecutionChild;
        struct ExecutionTree;
        struct FunctionPrototype;
        struct MetaData;
        struct NodeableParse;
        struct OutputAssignment;
        struct PropertyExtraction;
        struct ReturnValue;
        struct Scope;
        struct Variable;
        struct VariableRead;
        struct VariableWrite;
        struct VariableWriteHandling;

        using AbstractCodeModelConstPtr = AZStd::shared_ptr<const AbstractCodeModel>;
        using AbstractCodeModelPtr = AZStd::shared_ptr<AbstractCodeModel>;
        using DebugInfoConstPtr = AZStd::shared_ptr<const DebugInfo>;
        using DebugInfoPtr = AZStd::shared_ptr<DebugInfo>;
        using EBusHandlingConstPtr = AZStd::shared_ptr<const EBusHandling>;
        using EBusHandlingPtr = AZStd::shared_ptr<EBusHandling>;
        using EventHandlingConstPtr = AZStd::shared_ptr<const EventHandling>;
        using EventHandlingPtr = AZStd::shared_ptr<EventHandling>;
        using ExecutionTreeConstPtr = AZStd::shared_ptr<const ExecutionTree>;
        using ExecutionTreePtr = AZStd::shared_ptr<ExecutionTree>;
        using MetaDataConstPtr = AZStd::shared_ptr<const MetaData>;
        using MetaDataPtr = AZStd::shared_ptr<MetaData>;
        using NodeableParseConstPtr = AZStd::shared_ptr<const NodeableParse>;
        using NodeableParsePtr = AZStd::shared_ptr<NodeableParse>;
        using OutputAssignmentConstPtr = AZStd::shared_ptr<const OutputAssignment>;
        using OutputAssignmentPtr = AZStd::shared_ptr<OutputAssignment>;
        using PropertyExtractionConstPtr = AZStd::shared_ptr<const PropertyExtraction>;
        using PropertyExtractionPtr = AZStd::shared_ptr<PropertyExtraction>;
        using ReturnValueConstPtr = AZStd::shared_ptr<const ReturnValue>;
        using ReturnValuePtr = AZStd::shared_ptr<ReturnValue>;
        using ScopeConstPtr = AZStd::shared_ptr<const Scope>;
        using ScopePtr = AZStd::shared_ptr<Scope>;
        using VariableConstPtr = AZStd::shared_ptr<Variable>;
        using VariablePtr = AZStd::shared_ptr<Variable>;
        using VariableReadConstPtr = AZStd::shared_ptr<const VariableRead>;
        using VariableReadPtr = AZStd::shared_ptr<VariableRead>;
        using VariableWriteConstPtr = AZStd::shared_ptr<const VariableWrite>;
        using VariableWritePtr = AZStd::shared_ptr<VariableWrite>;
        using VariableWriteHandlingPtr = AZStd::shared_ptr<VariableWriteHandling>;
        using VariableWriteHandlingConstPtr = AZStd::shared_ptr<VariableWriteHandling>;

        using ControlVariablesBySourceNode = AZStd::unordered_map<const Node*, VariablePtr>;
        using ConversionByIndex = AZStd::unordered_map<size_t, Data::Type>;
        using EBusHandlingByNode = AZStd::unordered_map<const Node*, EBusHandlingPtr>;
        using EventHandlingByNode = AZStd::unordered_map<const Node*, EventHandlingPtr>;
        using NodeableParseByNode = AZStd::unordered_map<const Node*, NodeableParsePtr>;
        using ImplicitVariablesByNode = AZStd::unordered_map<ExecutionTreeConstPtr, VariablePtr>;
        using VariableHandlingBySlot = AZStd::unordered_map<const Slot*, VariableWriteHandlingPtr>;
        using VariableWriteHandlingSet = AZStd::unordered_set<VariableWriteHandlingPtr>;
        using VariableWriteHandlingConstSet = AZStd::unordered_set<VariableWriteHandlingConstPtr>;
        using VariableWriteHandlingByVariable = AZStd::unordered_map<VariableConstPtr, VariableWriteHandlingSet>;

        AZ_CVAR_EXTERNED(bool, g_disableParseOnGraphValidation);
        AZ_CVAR_EXTERNED(bool, g_printAbstractCodeModel);
        AZ_CVAR_EXTERNED(bool, g_printAbstractCodeModelAtPrefabTime);
        AZ_CVAR_EXTERNED(bool, g_saveRawTranslationOuputToFile);
        AZ_CVAR_EXTERNED(bool, g_saveRawTranslationOuputToFileAtPrefabTime);

        class SettingsCache
        {
        public:
            AZ_CLASS_ALLOCATOR(SettingsCache, AZ::SystemAllocator, 0);

            SettingsCache();
            ~SettingsCache();

        private:
            bool m_disableParseOnGraphValidation;
            bool m_printAbstractCodeModel;
            bool m_printAbstractCodeModelAtPrefabTime;
            bool m_saveRawTranslationOuputToFile;
            bool m_saveRawTranslationOuputToFileAtPrefabTime;
        };

        struct DependencyInfo
        {
            AZ::Data::AssetId assetId;
            bool requiresCtorParams = false;
            bool requiresCtorParamsForDependencies = false;
        };

        struct Request
        {
            AZ::Data::AssetId scriptAssetId;
            const Graph* graph = nullptr;
            AZStd::string_view name;
            AZStd::string_view path;
            AZStd::string_view namespacePath;
            AZ::u32 translationTargetFlags = 0;
            bool addDebugInformation = true;
            bool rawSaveDebugOutput = false;
            bool printModelToConsole = false;
        };

        struct Source
        {
            AZ_TYPE_INFO(Source, "{116D1E9E-11F5-4610-95AD-42BF2C32E530}");
            AZ_CLASS_ALLOCATOR(Source, AZ::SystemAllocator, 0);

            static AZ::Outcome<Source, AZStd::string> Construct(const Request& reqeust);

            static const VariableData k_emptyVardata;

            const Graph* m_graph = nullptr;
            const AZ::Data::AssetId m_assetId;
            const GraphData* m_graphData = nullptr;
            const VariableData* m_variableData = nullptr;
            const AZStd::string m_name;
            const AZStd::string m_path;
            const NamespacePath m_namespacePath;
            const bool m_addDebugInfo = true;
            const bool m_printModelToConsole = false;
            const AZStd::string m_assetIdString;

            Source() = default;
            Source
                ( const Graph& graph
                , const AZ::Data::AssetId& id
                , const GraphData& graphData
                , const VariableData& variableData
                , AZStd::string_view name
                , AZStd::string_view path
                , NamespacePath&& namespacePath
                , bool addDebugInformation = true
                , bool printModelToConsole = false);
        };

        AZStd::string ToTypeSafeEBusResultName(const Data::Type& type);
        AZStd::string ToSafeName(const AZStd::string& name);
        NamespacePath ToNamespacePath(AZStd::string_view path, AZStd::string_view name);
    }
}

namespace AZStd
{
    template<>
    struct hash<ScriptCanvas::Grammar::ExecutionTreeConstPtr>
    {
        using argument_type = ScriptCanvas::Grammar::ExecutionTreeConstPtr;
        using result_type = size_t;

        inline result_type operator() (const argument_type& id) const
        {
            size_t h = 0;
            hash_combine(h, id.get());
            return h;
        }
    };// struct hash<ScriptCanvas::Grammar::ExecutionTreeConstPtr>

    template<>
    struct hash<ScriptCanvas::Grammar::OutputAssignmentConstPtr>
    {
        using argument_type = ScriptCanvas::Grammar::OutputAssignmentConstPtr;
        using result_type = size_t;

        inline result_type operator() (const argument_type& id) const
        {
            size_t h = 0;
            hash_combine(h, id.get());
            return h;
        }
    };// struct hash<ScriptCanvas::Grammar::OutputAssignmentConstPtr>

    template<>
    struct hash<ScriptCanvas::Grammar::VariableConstPtr>
    {
        using argument_type = ScriptCanvas::Grammar::VariableConstPtr;
        using result_type = size_t;

        inline result_type operator() (const argument_type& id) const
        {
            size_t h = 0;
            hash_combine(h, id.get());
            return h;
        }
    };// struct hash<ScriptCanvas::Grammar::VariableConstPtr>

    template<>
    struct hash<ScriptCanvas::Grammar::VariableWriteHandlingPtr>
    {
        using argument_type = ScriptCanvas::Grammar::VariableWriteHandlingPtr;
        using result_type = size_t;

        inline result_type operator() (const argument_type& id) const
        {
            size_t h = 0;
            hash_combine(h, id.get());
            return h;
        }
    };// struct hash<ScriptCanvas::Grammar::VariableWriteHandlingPtr>
}
