/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <iostream>
#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <Libraries/Libraries.h>
#include <ScriptCanvas/Asset/RuntimeAsset.h>
#include <ScriptCanvas/Core/Contract.h>
#include <ScriptCanvas/Core/Graph.h>
#include <ScriptCanvas/Core/Node.h>
#include <ScriptCanvas/Core/Nodeable.h>
#include <ScriptCanvas/Core/Slot.h>
#include <ScriptCanvas/Execution/ExecutionPerformanceTimer.h>
#include <ScriptCanvas/Execution/Interpreted/ExecutionInterpretedAPI.h>
#include <ScriptCanvas/Execution/RuntimeComponent.h>
#include <ScriptCanvas/Serialization/DatumSerializer.h>
#include <ScriptCanvas/Serialization/BehaviorContextObjectSerializer.h>
#include <ScriptCanvas/Serialization/RuntimeVariableSerializer.h>
#include <ScriptCanvas/SystemComponent.h>
#include <ScriptCanvas/Variable/GraphVariableManagerComponent.h>
#include <ScriptCanvas/Core/Contracts/MathOperatorContract.h>

#include <ScriptCanvas/Libraries/Spawning/CreateSpawnTicketNodeable.h>
#include <ScriptCanvas/Libraries/Spawning/DespawnNodeable.h>
#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>


#if defined(SC_EXECUTION_TRACE_ENABLED)
#include <ScriptCanvas/Asset/ExecutionLogAsset.h>
#endif
#include <AutoGen/ScriptCanvasAutoGenRegistry.h>

namespace ScriptCanvasSystemComponentCpp
{
#if !defined(_RELEASE)
    const int k_infiniteLoopDetectionMaxIterations = 1000000;
    const int k_maxHandlerStackDepth = 25;
#else
    const int k_infiniteLoopDetectionMaxIterations = 10000000;
    const int k_maxHandlerStackDepth = 100;
#endif

    bool IsDeprecated(const AZ::AttributeArray& attributes)
    {
        bool isDeprecated{};

        if (auto isDeprecatedAttributePtr = AZ::FindAttribute(AZ::Script::Attributes::Deprecated, attributes))
        {
            AZ::AttributeReader(nullptr, isDeprecatedAttributePtr).Read<bool>(isDeprecated);
        }

        return isDeprecated;
    }
}

namespace ScriptCanvas
{
    AZ_ENUM_CLASS(PerformanceReportFileStream,
        None,
        Stdout,
        Stderr
    );
}

namespace ScriptCanvas
{
    // Console Variable to determine where the scriptcanvas output performance report is sent
    AZ_CVAR(PerformanceReportFileStream, sc_outputperformancereport, PerformanceReportFileStream::None, {}, AZ::ConsoleFunctorFlags::Null,
        "Determines where the Script Canvas performance report should be output.");

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ScriptCanvasModel::Instance().Reflect(context);

        VersionData::Reflect(context);
        Nodeable::Reflect(context);
        SourceHandle::Reflect(context);
        ReflectLibraries(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(1)
                // ScriptCanvas avoids a use dependency on the AssetBuilderSDK. Therefore the Crc is used directly to register this component with the Gem builder
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AZ_CRC_CE("AssetBuilder") }))
                ->Field("m_infiniteLoopDetectionMaxIterations", &SystemComponent::m_infiniteLoopDetectionMaxIterations)
                ->Field("maxHandlerStackDepth", &SystemComponent::m_maxHandlerStackDepth)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("Script Canvas", "Script Canvas System Component")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "Scripting")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_infiniteLoopDetectionMaxIterations, "Infinite Loop Protection Max Iterations", "Script Canvas will avoid infinite loops by detecting potentially re-entrant conditions that execute up to this number of iterations.")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_maxHandlerStackDepth, "Max Handler Stack Depth", "Script Canvas will avoid infinite loops at run-time by detecting sending Ebus Events while handling said Events. This limits the stack depth of the broadcast.")
                    ->Attribute(AZ::Edit::Attributes::Min, 1000) // Safeguard user given value is valid
                    ;
            }
        }

        if (AZ::JsonRegistrationContext* jsonContext = azrtti_cast<AZ::JsonRegistrationContext*>(context))
        {
            jsonContext->Serializer<AZ::DatumSerializer>()->HandlesType<Datum>();
            jsonContext->Serializer<AZ::BehaviorContextObjectSerializer>()->HandlesType<BehaviorContextObject>();
            jsonContext->Serializer<AZ::RuntimeVariableSerializer>()->HandlesType<RuntimeVariable>();
        }

#if defined(SC_EXECUTION_TRACE_ENABLED)
        ExecutionLogData::Reflect(context);
        ExecutionLogAsset::Reflect(context);
#endif//defined(SC_EXECUTION_TRACE_ENABLED)
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("ScriptCanvasService"));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        (void)incompatible;
    }

    void SystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        // \todo configure the application to require these services
        // required.push_back(AZ_CRC_CE("AssetDatabaseService"));
        // required.push_back(AZ_CRC_CE("ScriptService"));
    }

    void SystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void SystemComponent::Init()
    {
        RegisterCreatableTypes();

        m_infiniteLoopDetectionMaxIterations = ScriptCanvasSystemComponentCpp::k_infiniteLoopDetectionMaxIterations;
        m_maxHandlerStackDepth = ScriptCanvasSystemComponentCpp::k_maxHandlerStackDepth;
    }

    void SystemComponent::Activate()
    {
        SystemRequestBus::Handler::BusConnect();
        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        if (behaviorContext)
        {
            AZ::BehaviorContextBus::Handler::BusConnect(behaviorContext);
        }

        if (IsAnyScriptInterpreted()) // or if is the editor...
        {
            Execution::ActivateInterpreted();
        }

        SafeRegisterPerformanceTracker();
    }

    void SystemComponent::Deactivate()
    {
        AZ::BehaviorContextBus::Handler::BusDisconnect();
        SystemRequestBus::Handler::BusDisconnect();

        ModPerformanceTracker()->CalculateReports();
        Execution::PerformanceTrackingReport report = ModPerformanceTracker()->GetGlobalReport();

        const double ready = aznumeric_caster(report.timing.initializationTime);
        const double instant = aznumeric_caster(report.timing.executionTime);
        const double latent = aznumeric_caster(report.timing.latentTime);
        const double total = aznumeric_caster(report.timing.totalTime);

        FILE* performanceReportStream = nullptr;
        if (auto console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
        {
            if (PerformanceReportFileStream performanceOutputOption;
                console->GetCvarValue("sc_outputperformancereport", performanceOutputOption) == AZ::GetValueResult::Success)
            {
                switch (performanceOutputOption)
                {
                case PerformanceReportFileStream::None:
                    performanceReportStream = nullptr;
                    break;
                case PerformanceReportFileStream::Stdout:
                    performanceReportStream = stdout;
                    break;
                case PerformanceReportFileStream::Stderr:
                    performanceReportStream = stderr;
                    break;
                }
            }
        }
        if (performanceReportStream != nullptr)
        {
            fprintf(performanceReportStream, "Global ScriptCanvas Performance Report:\n");
            fprintf(performanceReportStream, "[ INITIALIZE] %s\n", AZStd::fixed_string<32>::format("%7.3f ms", ready / 1000.0).c_str());
            fprintf(performanceReportStream, "[  EXECUTION] %s\n", AZStd::fixed_string<32>::format("%7.3f ms", instant / 1000.0).c_str());
            fprintf(performanceReportStream, "[     LATENT] %s\n", AZStd::fixed_string<32>::format("%7.3f ms", latent / 1000.0).c_str());
            fprintf(performanceReportStream, "[      TOTAL] %s\n", AZStd::fixed_string<32>::format("%7.3f ms", total / 1000.0).c_str());
        }
        SafeUnregisterPerformanceTracker();
    }

    bool SystemComponent::IsScriptUnitTestingInProgress()
    {
        return m_scriptBasedUnitTestingInProgress;
    }

    void SystemComponent::MarkScriptUnitTestBegin()
    {
        m_scriptBasedUnitTestingInProgress = true;
    }

    void SystemComponent::MarkScriptUnitTestEnd()
    {
        m_scriptBasedUnitTestingInProgress = false;
    }

    void SystemComponent::CreateEngineComponentsOnEntity(AZ::Entity* entity)
    {
        if (entity)
        {
            auto graph = entity->CreateComponent<Graph>();
            entity->CreateComponent<GraphVariableManagerComponent>(graph->GetScriptCanvasId());
        }
    }

    Graph* SystemComponent::CreateGraphOnEntity(AZ::Entity* graphEntity)
    {
        return graphEntity ? graphEntity->CreateComponent<Graph>() : nullptr;
    }

    ScriptCanvas::Graph* SystemComponent::MakeGraph()
    {
        AZ::Entity* graphEntity = aznew AZ::Entity("Script Canvas");
        ScriptCanvas::Graph* graph = graphEntity->CreateComponent<ScriptCanvas::Graph>();
        return graph;
    }

    ScriptCanvasId SystemComponent::FindScriptCanvasId(AZ::Entity* graphEntity)
    {
        auto* graph = graphEntity ? AZ::EntityUtils::FindFirstDerivedComponent<ScriptCanvas::Graph>(graphEntity) : nullptr;
        return graph ? graph->GetScriptCanvasId() : ScriptCanvasId();
    }

    ScriptCanvas::Node* SystemComponent::GetNode(const AZ::EntityId& nodeId, const AZ::Uuid& typeId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, nodeId);
        if (entity)
        {
            ScriptCanvas::Node* node = azrtti_cast<ScriptCanvas::Node*>(entity->FindComponent(typeId));
            return node;
        }

        return nullptr;
    }

    ScriptCanvas::Node* SystemComponent::CreateNodeOnEntity(const AZ::EntityId& entityId, ScriptCanvasId scriptCanvasId, const AZ::Uuid& nodeType)
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to retrieve application serialize context");

        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(nodeType);
        AZ_Assert(classData, "Type %s is not registered in the serialization context", nodeType.ToString<AZStd::string>().data());

        if (classData)
        {
            AZ::Entity* nodeEntity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(nodeEntity, &AZ::ComponentApplicationRequests::FindEntity, entityId);
            ScriptCanvas::Node* node = reinterpret_cast<ScriptCanvas::Node*>(classData->m_factory->Create(classData->m_name));
            AZ_Assert(node, "ClassData (%s) does not correspond to a supported ScriptCanvas Node", classData->m_name);
            if (node && nodeEntity)
            {
                nodeEntity->SetName(classData->m_name);
                nodeEntity->AddComponent(node);
            }

            GraphRequestBus::Event(scriptCanvasId, &GraphRequests::AddNode, nodeEntity->GetId());
            return node;
        }

        return nullptr;
    }

    void SystemComponent::AddOwnedObjectReference(const void* object, BehaviorContextObject* behaviorContextObject)
    {
        if (object == nullptr)
        {
            return;
        }

        LockType lock(m_ownedObjectsByAddressMutex);
        [[maybe_unused]] auto emplaceResult = m_ownedObjectsByAddress.emplace(object, behaviorContextObject);

        AZ_Assert(emplaceResult.second, "Adding second owned reference to memory");
    }

    BehaviorContextObject* SystemComponent::FindOwnedObjectReference(const void* object)
    {
        if (object == nullptr)
        {
            return nullptr;
        }

        LockType lock(m_ownedObjectsByAddressMutex);
        auto iter = m_ownedObjectsByAddress.find(object);

        return iter == m_ownedObjectsByAddress.end() ? nullptr : iter->second;
    }

    void SystemComponent::RemoveOwnedObjectReference(const void* object)
    {
        if (object == nullptr)
        {
            return;
        }

        LockType lock(m_ownedObjectsByAddressMutex);
        m_ownedObjectsByAddress.erase(object);
    }

    AZStd::pair<DataRegistry::Createability, TypeProperties> SystemComponent::GetCreatibility(AZ::SerializeContext* serializeContext, AZ::BehaviorClass* behaviorClass)
    {
        TypeProperties typeProperties;

        bool canCreate{};
        // BehaviorContext classes with the ExcludeFrom attribute with a value of the ExcludeFlags::List is not creatable
        const AZ::u64 exclusionFlags = AZ::Script::Attributes::ExcludeFlags::List;
        auto excludeClassAttributeData = azrtti_cast<const AZ::Edit::AttributeData<AZ::Script::Attributes::ExcludeFlags>*>(AZ::FindAttribute(AZ::Script::Attributes::ExcludeFrom, behaviorClass->m_attributes));

        const AZ::u64 flags = excludeClassAttributeData ? excludeClassAttributeData->Get(nullptr) : 0;
        bool listOnly = ((flags & AZ::Script::Attributes::ExcludeFlags::ListOnly) == AZ::Script::Attributes::ExcludeFlags::ListOnly); // ListOnly exclusions may create variables
        canCreate = listOnly || (!excludeClassAttributeData || (!(flags & exclusionFlags)));
        canCreate = canCreate && (serializeContext->FindClassData(behaviorClass->m_typeId));
        canCreate = canCreate && !ScriptCanvasSystemComponentCpp::IsDeprecated(behaviorClass->m_attributes);

        if (canCreate)
        {
            for (auto base : behaviorClass->m_baseClasses)
            {
                if (AZ::Component::TYPEINFO_Uuid() == base)
                {
                    canCreate = false;
                    break; // only out of the for : base classes loop. DO NOT break out of the parent loop.
                }
            }
        }

        // Assets are not safe enough for variable creation, yet. They can be created with one Az type (Data::Asset<T>), but set to nothing.
        // When read back in, they will (if lucky) just be Data::Asset<Data>, which breaks type safety at best, and requires a lot of sanity checking.
        // This is NOT blacked at the createable types or BehaviorContext level, since they could be used to at least pass information through,
        // and may be used other scripting contexts.
        AZ::IRttiHelper* rttiHelper = behaviorClass->m_azRtti;
        if (rttiHelper && rttiHelper->GetGenericTypeId() == azrtti_typeid<AZ::Data::Asset>())
        {
            canCreate = false;
        }

        if (AZ::FindAttribute(AZ::ScriptCanvasAttributes::AllowInternalCreation, behaviorClass->m_attributes))
        {
            canCreate = true;
            typeProperties.m_isTransient = true;
        }

        // create able variables must have full memory support
        canCreate = canCreate &&
            (behaviorClass->m_allocate
                && behaviorClass->m_cloner
                && behaviorClass->m_mover
                && behaviorClass->m_destructor
                && behaviorClass->m_deallocate) &&
            AZStd::none_of(behaviorClass->m_baseClasses.begin(), behaviorClass->m_baseClasses.end(), [](const AZ::TypeId& base) { return azrtti_typeid<AZ::Component>() == base; });

        if (!canCreate)
        {
            return { DataRegistry::Createability::None , TypeProperties{} };
        }
        else if (!AZ::FindAttribute(AZ::ScriptCanvasAttributes::VariableCreationForbidden, behaviorClass->m_attributes))
        {
            return { DataRegistry::Createability::SlotAndVariable, typeProperties };
        }
        else
        {
            return { DataRegistry::Createability::SlotOnly, typeProperties };
        }
    }

    void SystemComponent::RegisterCreatableTypes()
    {
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Serialize Context should not be missing at this point");
        AZ::BehaviorContext* behaviorContext{};
        AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
        AZ_Assert(behaviorContext, "Behavior Context should not be missing at this point");

        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        for (const auto& classIter : behaviorContext->m_classes)
        {
            auto createability = GetCreatibility(serializeContext, classIter.second);
            if (createability.first != DataRegistry::Createability::None)
            {
                dataRegistry->RegisterType(classIter.second->m_typeId, createability.second, createability.first);
            }
        }
    }

    void SystemComponent::OnAddClass(const char*, AZ::BehaviorClass* behaviorClass)
    {
        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        if (!dataRegistry)
        {
            AZ_Warning("ScriptCanvas", false, "Data registry not available. Can't register new class.");

            return;
        }

        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Serialize Context missing. Can't register new class.");

        auto createability = GetCreatibility(serializeContext, behaviorClass);
        if (createability.first != DataRegistry::Createability::None)
        {
            dataRegistry->RegisterType(behaviorClass->m_typeId, createability.second, createability.first);
        }
    }

    void SystemComponent::OnRemoveClass(const char*, AZ::BehaviorClass* behaviorClass)
    {
        // The data registry might not be available when unloading the ScriptCanvas module
        auto dataRegistry = ScriptCanvas::GetDataRegistry();
        if (dataRegistry)
        {
            dataRegistry->UnregisterType(behaviorClass->m_typeId);
        }
    }

    void SystemComponent::SetInterpretedBuildConfiguration(BuildConfiguration config)
    {
        Execution::SetInterpretedExecutionMode(config);
    }

    AZ::EnvironmentVariable<Execution::PerformanceTracker*> SystemComponent::s_perfTracker;
    AZStd::shared_mutex SystemComponent::s_perfTrackerMutex;

    Execution::PerformanceTracker* SystemComponent::ModPerformanceTracker()
    {
        // First attempt to use the module-static reference; take a read lock to check it.
        // This is the fast path which won't block.
        {
            AZStd::shared_lock<AZStd::shared_mutex> lock(s_perfTrackerMutex);
            if (s_perfTracker)
            {
                return s_perfTracker.Get();
            }
        }

        // If the instance doesn't exist (which means we could be in a different module),
        // take the full lock and request it.
        AZStd::unique_lock<AZStd::shared_mutex> lock(s_perfTrackerMutex);
        s_perfTracker = AZ::Environment::FindVariable<Execution::PerformanceTracker*>(s_trackerName);
        return s_perfTracker ? s_perfTracker.Get() : nullptr;
    }

    void SystemComponent::SafeRegisterPerformanceTracker()
    {
        if (ModPerformanceTracker())
        {
            return;
        }

        AZStd::unique_lock<AZStd::shared_mutex> lock(s_perfTrackerMutex);

        auto tracker = aznew Execution::PerformanceTracker();
        s_perfTracker = AZ::Environment::CreateVariable<Execution::PerformanceTracker*>(s_trackerName);
        s_perfTracker.Get() = tracker;
    }

    void SystemComponent::SafeUnregisterPerformanceTracker()
    {
        auto performanceTracker = ModPerformanceTracker();
        if (!performanceTracker)
        {
            return;
        }

        AZStd::unique_lock<AZStd::shared_mutex> lock(s_perfTrackerMutex);
        *s_perfTracker = nullptr;
        s_perfTracker.Reset();
        delete performanceTracker;
    }
}
