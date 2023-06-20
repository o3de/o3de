/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <Builder/ScriptCanvasBuilderComponent.h>

namespace ScriptCanvasUnitTest
{
    namespace ScriptCanvasBuilderComponentUnitTestStructures
    {
        class TestHandler
            : public AssetBuilderSDK::AssetBuilderBus::Handler
            , public AZ::ComponentApplicationBus::Handler
        {
        public:
            AZ::BehaviorContext* m_behaviorContext;
            AZ::SerializeContext* m_serializeContext;
            AZStd::string m_fingerprint;

            // ComponentApplicationBus
            AZ::ComponentApplication* GetApplication() override { return nullptr; }
            void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}
            void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override {}
            bool AddEntity(AZ::Entity*) override { return true; }
            bool RemoveEntity(AZ::Entity*) override { return true; }
            bool DeleteEntity(const AZ::EntityId&) override { return true; }
            AZ::Entity* FindEntity(const AZ::EntityId&) override { return nullptr; }
            AZ::SerializeContext* GetSerializeContext() override { return m_serializeContext; }
            AZ::BehaviorContext*  GetBehaviorContext() override { return m_behaviorContext; }
            AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
            const char* GetExecutableFolder() const override { return nullptr; }
            const char* GetBinFolder() const { return nullptr; }
            const char* GetAppRoot() override { return nullptr; }
            void EnumerateEntities(const EntityCallback& /*callback*/) override {}

            // AssetBuilderBus
            void RegisterBuilderInformation(const AssetBuilderSDK::AssetBuilderDesc& desc) override
            {
                m_fingerprint = desc.m_analysisFingerprint;
            }

            void Init(AZ::BehaviorContext* behaviorContext, AZ::SerializeContext* serializeContext)
            {
                m_behaviorContext = behaviorContext;
                m_serializeContext = serializeContext;
            }

            void Activate()
            {
                AZ::ComponentApplicationBus::Handler::BusConnect();
                AssetBuilderSDK::AssetBuilderBus::Handler::BusConnect();
            }

            void Deactivate()
            {
                AssetBuilderSDK::AssetBuilderBus::Handler::BusDisconnect();
                AZ::ComponentApplicationBus::Handler::BusDisconnect();
            }
        };
    };

    class ScriptCanvasScriptCanvasBuilderComponentUnitTestFixture
        : public ScriptCanvasUnitTestFixture
    {
    protected:
        AZ::BehaviorContext* m_behaviorContext;
        AZ::SerializeContext* m_serializeContext;
        ScriptCanvasBuilderComponentUnitTestStructures::TestHandler* m_testHandler;
        ScriptCanvasBuilder::PluginComponent* m_pluginComponent;


        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            m_serializeContext = aznew AZ::SerializeContext();
            m_behaviorContext = aznew AZ::BehaviorContext();

            m_testHandler = new ScriptCanvasBuilderComponentUnitTestStructures::TestHandler();
            m_testHandler->Init(m_behaviorContext, m_serializeContext);
            m_testHandler->Activate();

            AZ::Data::AssetManager::Descriptor desc;
            AZ::Data::AssetManager::Create(desc);
            m_pluginComponent = aznew ScriptCanvasBuilder::PluginComponent();
        };

        void TearDown() override
        {
            m_pluginComponent->Deactivate();
            delete m_pluginComponent;
            AZ::Data::AssetManager::Destroy();

            m_testHandler->Deactivate();
            delete m_testHandler;

            delete m_behaviorContext;
            delete m_serializeContext;

            ScriptCanvasUnitTestFixture::TearDown();
        };
    };

    TEST_F(ScriptCanvasScriptCanvasBuilderComponentUnitTestFixture, Activate_FingerprintContainsZeroHashValue_BehaviorContextIsEmpty)
    {
        m_pluginComponent->Activate();
        size_t splitterPos = m_testHandler->m_fingerprint.find_last_of('|');
        EXPECT_TRUE(splitterPos != m_testHandler->m_fingerprint.npos);
        uint64_t behaviorContextHash = AZStd::stoull(m_testHandler->m_fingerprint.substr(splitterPos + 1));
        EXPECT_EQ(behaviorContextHash, 0);
    }
}
