/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <Integration/Assets/ActorAsset.h>
#include <Integration/Rendering/RenderBackend.h>
#include <Integration/Rendering/RenderActor.h>
#include <Integration/Rendering/RenderFlag.h>
#include <Integration/Rendering/RenderActorInstance.h>
#include <Integration/Rendering/RenderBackendManager.h>
#include <Integration/System/SystemCommon.h>
#include <Tests/SystemComponentFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/TestActorAssets.h>

namespace EMotionFX
{
    namespace Integration
    {
        class TestRenderActor
            : public RenderActor
        {
        public:
            AZ_RTTI(TestRenderActor, "{560849A4-7767-4654-8C61-EDA9A0059BE1}", RenderActor)
            AZ_CLASS_ALLOCATOR(TestRenderActor, EMotionFXAllocator, 0)

            TestRenderActor(ActorAsset* actorAsset)
                : RenderActor()
                , m_actorAsset(actorAsset)
            {
            }

        public:
            ActorAsset* m_actorAsset = nullptr;
        };

        class TestRenderActorInstance
            : public RenderActorInstance
        {
        public:
            AZ_RTTI(TestRenderActorInstance, "{8F5CD404-9661-4A71-9583-EB8E66F3C0E8}", RenderActorInstance)
            AZ_CLASS_ALLOCATOR(TestRenderActorInstance, EMotionFXAllocator, 0)

            TestRenderActorInstance(AZ::EntityId entityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<ActorAsset>& asset,
                const ActorAsset::MaterialList& materialPerLOD,
                SkinningMethod skinningMethod,
                const AZ::Transform& worldTransform)
                : RenderActorInstance(asset, actorInstance.get(), entityId)
                , m_entityId(entityId)
                , m_actorAsset(asset)
                , m_actorInstance(actorInstance)
                , m_materialPerLOD(materialPerLOD)
                , m_skinningMethod(skinningMethod)
                , m_worldTransform(worldTransform)
            {
            }

            MOCK_METHOD1(OnTick, void(float));
            MOCK_METHOD1(DebugDraw, void(const EMotionFX::ActorRenderFlagBitset&));
            MOCK_CONST_METHOD0(IsVisible, bool());
            MOCK_METHOD1(SetIsVisible, void(bool));
            MOCK_METHOD1(SetMaterials, void(const ActorAsset::MaterialList&));
            MOCK_METHOD0(UpdateBounds, void());
            MOCK_METHOD0(GetWorldBounds, AZ::Aabb());
            MOCK_METHOD0(GetLocalBounds, AZ::Aabb());

        public:
            AZ::EntityId m_entityId;
            AZ::Data::Asset<ActorAsset> m_actorAsset;
            EMotionFXPtr<EMotionFX::ActorInstance> m_actorInstance;
            ActorAsset::MaterialList m_materialPerLOD;
            SkinningMethod m_skinningMethod = SkinningMethod::Linear;
            AZ::Transform m_worldTransform = AZ::Transform::CreateIdentity();
        };

        class TestRenderBackend
            : public RenderBackend
        {
        public:
            AZ_RTTI(TestRenderBackend, "{22CC2C55-8019-4302-8DFD-E08E0CA48114}", RenderBackend)
            AZ_CLASS_ALLOCATOR(TestRenderBackend, EMotionFXAllocator, 0)

            RenderActor* CreateActor(ActorAsset* actorAsset) override
            {
                return aznew TestRenderActor(actorAsset);
            }

            RenderActorInstance* CreateActorInstance(AZ::EntityId entityId,
                const EMotionFXPtr<EMotionFX::ActorInstance>& actorInstance,
                const AZ::Data::Asset<ActorAsset>& asset,
                const ActorAsset::MaterialList& materialPerLOD,
                SkinningMethod skinningMethod,
                const AZ::Transform& worldTransform) override
            {
                return aznew TestRenderActorInstance(entityId, actorInstance, asset, materialPerLOD, skinningMethod, worldTransform);
            }
        };

        class RenderBackendManagerFixture
            : public SystemComponentFixture
        {
        public:
            void SetUp() override
            {
                SystemComponentFixture::SetUp();
                EXPECT_NE(AZ::Interface<RenderBackendManager>::Get(), nullptr);
            }

        public:
            TestRenderBackend* m_renderBackend = nullptr;
        };

        TEST_F(RenderBackendManagerFixture, AdjustRenderBackend)
        {
            RenderBackendManager* renderBackendManager = AZ::Interface<RenderBackendManager>::Get();
            EXPECT_NE(renderBackendManager, nullptr);

            m_renderBackend = aznew TestRenderBackend();
            AZ::Interface<RenderBackendManager>::Get()->SetRenderBackend(m_renderBackend);
            RenderBackend* renderBackend = renderBackendManager->GetRenderBackend();
            EXPECT_NE(renderBackend, nullptr);
            EXPECT_EQ(renderBackend->RTTI_GetType(), azrtti_typeid<TestRenderBackend>());
        }

        class RenderBackendActorTestComponent
            : public AZ::Component
        {
        public:
            AZ_COMPONENT(RenderBackendActorTestComponent, "{699DE64B-ADD1-4B27-AC54-3D041AF82938}");

            RenderBackendActorTestComponent()
                : AZ::Component()
            {
            }

            static void Reflect(AZ::ReflectContext* context)
            {
                auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (serializeContext)
                {
                    serializeContext->Class<RenderBackendActorTestComponent, AZ::Component>()
                        ->Version(1);
                }
            }

            void Activate() override
            {
                m_actorInstance = m_actorAsset->CreateInstance(GetEntity());
                EXPECT_NE(m_actorInstance, nullptr);

                RenderBackend* renderBackend = AZ::Interface<RenderBackendManager>::Get()->GetRenderBackend();
                m_renderActorInstance.reset(renderBackend->CreateActorInstance(GetEntityId(),
                    m_actorInstance,
                    m_actorAsset,
                    /*materialPerLOD=*/{},
                    SkinningMethod::Linear,
                    /*transform=*/{}));
            }

            void Deactivate() override
            {
                m_actorAsset.Release();
                m_actorInstance.reset();
                m_renderActorInstance.reset();
            }

        public:
            AZ::Data::Asset<ActorAsset> m_actorAsset;
            ActorAsset::ActorInstancePtr m_actorInstance;
            AZStd::unique_ptr<RenderActorInstance> m_renderActorInstance;
        };

        TEST_F(RenderBackendManagerFixture, RenderActorComponentTest)
        {
            m_app.RegisterComponentDescriptor(RenderBackendActorTestComponent::CreateDescriptor());

            RenderBackendManager* renderBackendManager = AZ::Interface<RenderBackendManager>::Get();
            ASSERT_NE(renderBackendManager, nullptr);

            m_renderBackend = aznew TestRenderBackend();
            AZ::Interface<RenderBackendManager>::Get()->SetRenderBackend(m_renderBackend);
            RenderBackend* renderBackend = renderBackendManager->GetRenderBackend();
            ASSERT_NE(renderBackend, nullptr);
            EXPECT_EQ(renderBackend->RTTI_GetType(), azrtti_typeid<TestRenderBackend>());

            AZ::Data::AssetId actorAssetId("{D568F319-49E9-47BA-9E1C-24F949EF28DD}");
            AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
            AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));

            // Create render actor.
            AZStd::unique_ptr<RenderActor> renderActor(renderBackend->CreateActor(actorAsset.Get()));
            EXPECT_EQ(renderActor->RTTI_GetType(), azrtti_typeid<TestRenderActor>());
            TestRenderActor* testRenderActor = azdynamic_cast<TestRenderActor*>(renderActor.get());
            ASSERT_NE(testRenderActor, nullptr);

            // Create entity with a test actor component (that creates a render actor instance).
            AZ::EntityId entityId(42);
            auto gameEntity = AZStd::make_unique<AZ::Entity>();
            gameEntity->SetId(entityId);

            auto testActorComponent = gameEntity->CreateComponent<RenderBackendActorTestComponent>();
            testActorComponent->m_actorAsset = actorAsset;
            gameEntity->Init();
            gameEntity->Activate();

            EXPECT_EQ(testActorComponent->m_actorAsset, actorAsset);
            EXPECT_NE(testActorComponent->m_actorInstance, nullptr);
            TestRenderActorInstance* renderActorInstance = azdynamic_cast<TestRenderActorInstance*>(testActorComponent->m_renderActorInstance.get());
            ASSERT_NE(renderActorInstance, nullptr);
            EXPECT_EQ(renderActorInstance->RTTI_GetType(), azrtti_typeid<TestRenderActorInstance>());
            EXPECT_EQ(renderActorInstance->m_entityId, entityId);
            EXPECT_EQ(renderActorInstance->m_actorInstance, testActorComponent->m_actorInstance);
            EXPECT_EQ(renderActorInstance->GetActor(), actorAsset->GetActor());
        }
    } // namespace Integration
} // namespace EMotionFX
