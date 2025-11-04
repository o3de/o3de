/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Common/RPITestFixture.h>
#include <Common/RHI/Factory.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/AzCore_Traits_Platform.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Script/ScriptSystemComponent.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzTest/Utils.h>

#include <Atom/RHI/Device.h>
#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <Atom/RPI.Reflect/ResourcePoolAsset.h>
#include <Atom/RPI.Reflect/Asset/AssetHandler.h>

#include <Atom/RPI.Public/Shader/ShaderResourceGroupPool.h>

namespace UnitTest
{
    using namespace AZ;
    using namespace RPI;

    // Expose AssetManagerComponent::Reflect function for testing
    class MyAssetManagerComponent : public AssetManagerComponent
    {
    public:
        static void Reflect(ReflectContext* reflection)
        {
            AssetManagerComponent::Reflect(reflection);
        }
    };

    RPITestFixture::RPITestFixture() = default;
    RPITestFixture::~RPITestFixture() = default;

    JsonRegistrationContext* RPITestFixture::GetJsonRegistrationContext()
    {
        return m_jsonRegistrationContext.get();
    }

    void RPITestFixture::Reflect(ReflectContext* context)
    {
        MyAssetManagerComponent::Reflect(context);
        RHI::ReflectSystemComponent::Reflect(context);
        RPI::RPISystem::Reflect(context);
        Name::Reflect(context);
    }

    void RPITestFixture::SetUp()
    {
        AssetManagerTestFixture::SetUp();

        AZ::RPI::Validation::SetEnabled(true);
        AZ::RHI::Validation::s_isEnabled = true;

        m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
        m_localFileIO.reset(aznew AZ::IO::LocalFileIO());
        AZ::IO::FileIOBase::SetInstance(m_localFileIO.get());

        AZ::IO::Path assetPath = AZStd::string_view{ AZ::Utils::GetProjectPath() };
        assetPath /= "Cache";
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", assetPath.c_str());

        m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
        AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());

        m_scriptSystemComponentDescriptor.reset(AZ::ScriptSystemComponent::CreateDescriptor());
        // Reflect the ScriptSystemComponent
        m_scriptSystemComponentDescriptor->Reflect(GetBehaviorContext());

        Reflect(GetSerializeContext());
        Reflect(GetBehaviorContext());
        Reflect(m_jsonRegistrationContext.get());

        NameDictionary::Create();

        m_rhiFactory.reset(aznew StubRHI::Factory());

        RPI::RPISystemDescriptor rpiSystemDescriptor;
        m_rpiSystem = AZStd::make_unique<RPI::RPISystem>();
        m_rpiSystem->Initialize(rpiSystemDescriptor);
        m_rpiSystem->InitializeSystemAssetsForTests();

        // Create the system entity
        m_systemEntity = AZStd::make_unique<AZ::Entity>(AZ::SystemEntityId);
        // Add the Lua Script System Component to add a Global Script Context
        m_systemEntity->CreateComponent<AZ::ScriptSystemComponent>();
        // Activate the System Entity
        m_systemEntity->Init();
        m_systemEntity->Activate();

        // Bind the reflected BehaviorContext functions to the ScriptContext
        AZ::ScriptContext* scriptContext{};
        AZ::ScriptSystemRequestBus::BroadcastResult(scriptContext, &AZ::ScriptSystemRequests::GetContext, AZ::ScriptContextIds::DefaultScriptContextId);
        ASSERT_NE(nullptr, scriptContext);
        scriptContext->BindTo(GetBehaviorContext());

        // Setup job context for job system
        JobManagerDesc desc;
        JobManagerThreadDesc threadDesc;
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
        threadDesc.m_cpuId = 0; // Don't set processors IDs on windows
#endif

        uint32_t numWorkerThreads = desc.GetWorkerThreadCount(AZStd::thread::hardware_concurrency());

        for (unsigned int i = 0; i < numWorkerThreads; ++i)
        {
            desc.m_workerThreads.push_back(threadDesc);
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
            threadDesc.m_cpuId++;
#endif
        }

        m_jobManager = AZStd::make_unique<JobManager>(desc);
        m_jobContext = AZStd::make_unique<JobContext>(*m_jobManager);
        JobContext::SetGlobalContext(m_jobContext.get());

        m_assetSystemStub.Activate();
    }

    void RPITestFixture::TearDown()
    {
        // Flushing the tick bus queue since AZ::RHI::Factory:Register queues a function
        AZ::SystemTickBus::ClearQueuedEvents();

        m_assetSystemStub.Deactivate();

        JobContext::SetGlobalContext(nullptr);
        m_jobContext = nullptr;
        m_jobManager = nullptr;

        // Deactivate and deletes the System Entity
        m_systemEntity.reset();

        m_rpiSystem->Shutdown();
        m_rpiSystem = nullptr;
        m_rhiFactory = nullptr;

        NameDictionary::Destroy();

        m_jsonRegistrationContext->EnableRemoveReflection();
        AZ::JsonSystemComponent::Reflect(m_jsonRegistrationContext.get());
        Reflect(m_jsonRegistrationContext.get());
        m_jsonRegistrationContext->DisableRemoveReflection();

        auto serializeContext = GetSerializeContext();
        serializeContext->EnableRemoveReflection();
        Reflect(serializeContext);
        serializeContext->DisableRemoveReflection();

        auto behaviorContext = GetBehaviorContext();
        behaviorContext->EnableRemoveReflection();
        Reflect(behaviorContext);
        m_scriptSystemComponentDescriptor->Reflect(behaviorContext);
        behaviorContext->DisableRemoveReflection();

        m_scriptSystemComponentDescriptor.reset();

        m_jsonRegistrationContext.reset();

        AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
        m_localFileIO.reset();

        AssetManagerTestFixture::TearDown();
    }

    void RPITestFixture::ProcessQueuedSrgCompilations(Data::Asset<ShaderAsset> shaderAsset, const AZ::Name& srgName)
    {
        Data::Instance<ShaderResourceGroupPool> srgPool = ShaderResourceGroupPool::FindOrCreate(shaderAsset, RPI::DefaultSupervariantIndex, srgName);
        srgPool->GetRHIPool()->CompileGroupsBegin();
        srgPool->GetRHIPool()->CompileGroupsForInterval(RHI::Interval(0, srgPool->GetRHIPool()->GetGroupsToCompileCount()));
        srgPool->GetRHIPool()->CompileGroupsEnd();
    }

}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
