/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/PlatformIncl.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Utils/Utils.h>

#include <AzFramework/IO/LocalFileIO.h>

#include <Atom/RHI.Reflect/ReflectSystemComponent.h>

#include <Atom/RPI.Public/RPISystem.h>

#include <BuilderComponent.h>

#include <Tests.Builders/BuilderTestFixture.h>

extern "C" void CleanUpRpiPublicGenericClassInfo();
extern "C" void CleanUpRpiEditGenericClassInfo();

namespace UnitTest
{
    using namespace AZ;

    // Expose AssetManagerComponent::Reflect function for testing
    class MyAssetManagerComponent : public AssetManagerComponent
    {
    public:
        static void Reflect(ReflectContext* reflection)
        {
            AssetManagerComponent::Reflect(reflection);
        }
    };

    SerializeContext* BuilderTestFixture::GetSerializeContext() 
    {
        return m_context.get();
    }

    JsonRegistrationContext* BuilderTestFixture::GetJsonRegistrationContext()
    {
        return m_jsonRegistrationContext.get();
    }

    void BuilderTestFixture::Reflect(ReflectContext* context)
    {
        RHI::ReflectSystemComponent::Reflect(context);
        RPI::RPISystem::Reflect(context);
        RPI::BuilderComponent::Reflect(context);
        AZ::Name::Reflect(context);
        MyAssetManagerComponent::Reflect(context);
    }

    void BuilderTestFixture::SetUp()
    {
        LeakDetectionFixture::SetUp();

        //prepare reflection
        m_context = AZStd::make_unique<SerializeContext>();
        
        m_jsonRegistrationContext = AZStd::make_unique<AZ::JsonRegistrationContext>();
        m_jsonSystemComponent = AZStd::make_unique<AZ::JsonSystemComponent>();
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
        
        // Adding this handler to allow utility functions access the serialize context
        ComponentApplicationBus::Handler::BusConnect();
        AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);

        // Startup default local FileIO (hits OSAllocator) if not already setup.
        if (IO::FileIOBase::GetInstance() == nullptr)
        {
            IO::FileIOBase::SetInstance(aznew IO::LocalFileIO());
        }

        NameDictionary::Create();

        Reflect(m_context.get());
        Reflect(m_jsonRegistrationContext.get());

        m_streamer = AZStd::make_unique<AZ::IO::Streamer>(AZStd::thread_desc{}, AZ::StreamerComponent::CreateStreamerStack());
        Interface<AZ::IO::IStreamer>::Register(m_streamer.get());

        Data::AssetManager::Descriptor desc;
        Data::AssetManager::Create(desc);

        AZ::Utils::GetExecutableDirectory(m_currentDir, AZ_MAX_PATH_LEN);
    }

    void BuilderTestFixture::TearDown()
    {

        Data::AssetManager::Destroy();

        Interface<AZ::IO::IStreamer>::Unregister(m_streamer.get());
        m_streamer.reset();

        delete IO::FileIOBase::GetInstance();
        IO::FileIOBase::SetInstance(nullptr);

        AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
        ComponentApplicationBus::Handler::BusDisconnect();        

        m_jsonRegistrationContext->EnableRemoveReflection();
        m_jsonSystemComponent->Reflect(m_jsonRegistrationContext.get());
        Reflect(m_jsonRegistrationContext.get());
        m_jsonRegistrationContext->DisableRemoveReflection();
        
        m_jsonRegistrationContext.reset();
        m_jsonSystemComponent.reset();

        NameDictionary::Destroy();

        m_context.reset();

        CleanUpRpiPublicGenericClassInfo();
        CleanUpRpiEditGenericClassInfo();

        LeakDetectionFixture::TearDown();
    }

} // namespace UnitTest
