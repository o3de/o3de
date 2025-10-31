/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Framework/ScriptCanvasUnitTestFixture.h>
#include <Libraries/Entity/EntityFunctions.h>

namespace ScriptCanvasUnitTest
{
    using namespace ScriptCanvas;

    class ScriptCanvasUnitTestEntityFunctions
        : public ScriptCanvasUnitTestFixture
        , public AZ::TransformBus::Handler
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        void SetUp() override
        {
            ScriptCanvasUnitTestFixture::SetUp();

            m_localTransform = AZ::Transform::CreateIdentity();
            m_wordTransform = AZ::Transform::CreateIdentity();
            m_entity.Init();
            m_entity.Activate();

            AZ::TransformBus::Handler::BusConnect(m_id);
            AZ::ComponentApplicationBus::Handler::BusConnect();
        }

        void TearDown() override
        {
            m_entity.Deactivate();
            AZ::ComponentApplicationBus::Handler::BusDisconnect();
            AZ::TransformBus::Handler::BusDisconnect(m_id);

            ScriptCanvasUnitTestFixture::TearDown();
        }

        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationBus
        AZ::ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void UnregisterComponentDescriptor(const AZ::ComponentDescriptor*) override { }
        void RegisterEntityAddedEventHandler(AZ::EntityAddedEvent::Handler&) override { }
        void RegisterEntityRemovedEventHandler(AZ::EntityRemovedEvent::Handler&) override { }
        void RegisterEntityActivatedEventHandler(AZ::EntityActivatedEvent::Handler&) override { }
        void RegisterEntityDeactivatedEventHandler(AZ::EntityDeactivatedEvent::Handler&) override { }
        void SignalEntityActivated(AZ::Entity*) override { }
        void SignalEntityDeactivated(AZ::Entity*) override { }
        bool AddEntity(AZ::Entity*) override { return false; }
        bool RemoveEntity(AZ::Entity*) override { return false; }
        bool DeleteEntity(const AZ::EntityId&) override { return false; }
        AZ::Entity* FindEntity(const AZ::EntityId&) override { return &m_entity; }
        AZ::SerializeContext* GetSerializeContext() override { return nullptr; }
        AZ::BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        AZ::JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const AZ::ComponentApplicationRequests::EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // TransformBus
        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler&) override {}
        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler&) override {}
        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler&) override {}
        void NotifyChildChangedEvent(AZ::ChildChangeType, AZ::EntityId) override {}
        const AZ::Transform& GetLocalTM() override { return m_localTransform; }
        bool IsStaticTransform() override { return false; }
        const AZ::Transform& GetWorldTM() override { return m_wordTransform; }
        void SetWorldTM(const AZ::Transform& tm) override { m_wordTransform = tm; }
        //////////////////////////////////////////////////////////////////////////

        AZ::EntityId m_id{123};
        AZ::Transform m_localTransform;
        AZ::Transform m_wordTransform;
        AZ::Entity m_entity;
    };

    TEST_F(ScriptCanvasUnitTestEntityFunctions, GetEntityRight_Call_GetExpectedResult)
    {
        float scale = 123.f;
        auto actualResult = EntityFunctions::GetEntityRight(m_id, scale);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector3(scale, 0, 0)));
#else
        EXPECT_EQ(actualResult, AZ::Vector3(scale, 0, 0));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, GetEntityForward_Call_GetExpectedResult)
    {
        float scale = 123.f;
        auto actualResult = EntityFunctions::GetEntityForward(m_id, scale);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector3(0, scale, 0)));
#else
        EXPECT_EQ(actualResult, AZ::Vector3(0, scale, 0));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, GetEntityUp_Call_GetExpectedResult)
    {
        float scale = 123.f;
        auto actualResult = EntityFunctions::GetEntityUp(m_id, scale);
#if AZ_TRAIT_USE_PLATFORM_SIMD_NEON
        EXPECT_THAT(actualResult, IsClose(AZ::Vector3(0, 0, scale)));
#else
        EXPECT_EQ(actualResult, AZ::Vector3(0, 0, scale));
#endif // AZ_TRAIT_USE_PLATFORM_SIMD_NEON
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, Rotate_Call_GetExpectedResult)
    {
        auto rotation = AZ::Vector3(180, 0, 0);
        EntityFunctions::Rotate(m_id, AZ::Vector3(180, 0, 0));
        EXPECT_EQ(m_wordTransform.GetRotation(), AZ::ConvertEulerDegreesToQuaternion(rotation));
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, IsActive_Call_GetExpectedResult)
    {
        auto actualResult = EntityFunctions::IsActive(m_id);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, IsValid_Call_GetExpectedResult)
    {
        auto actualResult = EntityFunctions::IsValid(m_id);
        EXPECT_TRUE(actualResult);
    }

    TEST_F(ScriptCanvasUnitTestEntityFunctions, ToString_Call_GetExpectedResult)
    {
        auto actualResult = EntityFunctions::ToString(m_id);
        EXPECT_EQ(actualResult, m_id.ToString());
    }
}
