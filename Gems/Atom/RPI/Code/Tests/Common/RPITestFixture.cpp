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
#include <AzCore/IO/Path/Path.h>
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

        AZ::RPI::Validation::s_isEnabled = true;
        AZ::RHI::Validation::s_isEnabled = true;

        m_priorFileIO = AZ::IO::FileIOBase::GetInstance();
        m_localFileIO.reset(aznew AZ::IO::LocalFileIO());
        AZ::IO::FileIOBase::SetInstance(m_localFileIO.get());

        AZ::IO::Path assetPath = AZStd::string_view{ AZ::Utils::GetProjectPath() };
        assetPath /= "Cache";
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@products@", assetPath.c_str());

        // Remark, AZ::Utils::GetProjectPath() is not used when defining "user" folder,
        // instead We use AZ::Test::GetEngineRootPath();.
        // Reason:
        // When running unit tests, using AZ::Utils::GetProjectPath() will resolve to something like:
        // "/data/workspace/o3de/build/linux/External/Atom-9a4d112b/RPI/Code/Cache"
        // The ShaderMetricSystem.cpp writes to the @user@ folder and the following runtime error occurs:
        // "You may not alter data inside the asset cache.  Please check the call stack and consider writing into the source asset folder instead."
        // "Attempted write location: /data/workspace/o3de/build/linux/External/Atom-9a4d112b/RPI/Code/Cache/user/shadermetrics.json"
        // To avoid the error We use AZ::Test::GetEngineRootPath();
        AZ::IO::Path userPath = AZ::Test::GetEngineRootPath();
        userPath /= "user";
        AZ::IO::FileIOBase::GetInstance()->SetAlias("@user@", userPath.c_str());

        m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
        m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());

        Reflect(GetSerializeContext());
        Reflect(m_jsonRegistrationContext.get());

        NameDictionary::Create();

        m_rhiFactory.reset(aznew StubRHI::Factory());

        RPI::RPISystemDescriptor rpiSystemDescriptor;
        m_rpiSystem = AZStd::make_unique<RPI::RPISystem>();
        m_rpiSystem->Initialize(rpiSystemDescriptor);
        m_rpiSystem->InitializeSystemAssetsForTests();

        // Setup job context for job system
        JobManagerDesc desc;
        JobManagerThreadDesc threadDesc;
#if AZ_TRAIT_SET_JOB_PROCESSOR_ID
        threadDesc.m_cpuId = 0; // Don't set processors IDs on windows
#endif

        uint32_t numWorkerThreads = AZStd::thread::hardware_concurrency();

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

        m_rpiSystem->Shutdown();
        m_rpiSystem = nullptr;
        m_rhiFactory = nullptr;

        NameDictionary::Destroy();

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
        Reflect(m_jsonRegistrationContext.get());
        m_jsonRegistrationContext->DisableRemoveReflection();

        m_jsonRegistrationContext.reset();
        m_jsonSystemComponent.reset();

        AZ::IO::FileIOBase::SetInstance(m_priorFileIO);
        m_localFileIO.reset();

        AssetManagerTestFixture::TearDown();
    }

    AZ::RHI::Device* RPITestFixture::GetDevice()
    {
        return AZ::RHI::RHISystemInterface::Get()->GetDevice();
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
