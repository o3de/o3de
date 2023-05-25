/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzToolsFramework/Component/EditorComponentAPIComponent.h>
#include <Component/EditorComponentAPIBus.h>
#include <Entity/EditorEntityActionComponent.h>

#include <EMotionFX/Source/MotionSet.h>
#include <Integration/Editor/Components/EditorActorComponent.h>
#include <Integration/Editor/Components/EditorAnimGraphComponent.h>
#include <UI/PropertyEditor/PropertyManagerComponent.h>

#include <Tests/SystemComponentFixture.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/JackActor.h>

namespace EMotionFX
{
    using CanDeleteJackEntityFixture = ComponentFixture<
        AZ::AssetManagerComponent,
        AZ::JobManagerComponent,
        AZ::StreamerComponent,
        AZ::UserSettingsComponent,
        AzToolsFramework::Components::PropertyManagerComponent,
        AzToolsFramework::Components::EditorEntityActionComponent,
        AzToolsFramework::Components::EditorComponentAPIComponent,
        EMotionFX::Integration::SystemComponent
    >;

    TEST_F(CanDeleteJackEntityFixture, CanDeleteJackEntity)
    {
        // C1559174:  Automate P1 Test - Simple_JackLocomotion - Jack can be removed from the scene
        RecordProperty("test_case_id", "C1559174");
        m_app.RegisterComponentDescriptor(Integration::ActorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::AnimGraphComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::EditorActorComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(Integration::EditorAnimGraphComponent::CreateDescriptor());
        m_app.RegisterComponentDescriptor(AzFramework::TransformComponent::CreateDescriptor());

        AZ::Entity* entity = aznew AZ::Entity(AZ::EntityId(83502341));
        entity->CreateComponent<AzFramework::TransformComponent>();
        Integration::EditorActorComponent* editorActorComponent = entity->CreateComponent<Integration::EditorActorComponent>();
        Integration::EditorAnimGraphComponent* editorAnimGraphComponent = entity->CreateComponent<Integration::EditorAnimGraphComponent>();

        // Load Jack actor asset
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));

        entity->Init();
        entity->Activate();
        editorActorComponent->SetActorAsset(actorAsset);

        // Load anim graph asset
        AZ::Data::AssetId animGraphAssetId("{37629818-5166-4B96-83F5-5818B6A1F449}");
        editorAnimGraphComponent->SetAnimGraphAssetId(animGraphAssetId);
        AZ::Data::Asset<Integration::AnimGraphAsset> animGraphAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::AnimGraphAsset>(animGraphAssetId, AZ::Data::AssetLoadBehavior::Default);
        AZStd::unique_ptr<TwoMotionNodeAnimGraph> animGraphPtr = AnimGraphFactory::Create<TwoMotionNodeAnimGraph>();
        AnimGraph* animGraph = animGraphPtr.get();
        animGraphPtr.release();
        animGraphAsset.GetAs<Integration::AnimGraphAsset>()->SetData(animGraph);
        editorAnimGraphComponent->OnAssetReady(animGraphAsset);

        // Load motion set asset.
        AZ::Data::AssetId motionSetAssetId("{224BFF5F-D0AD-4216-9CEF-42F419CC6265}");
        editorAnimGraphComponent->SetMotionSetAssetId(motionSetAssetId);
        AZ::Data::Asset<Integration::MotionSetAsset> motionSetAsset = AZ::Data::AssetManager::Instance().CreateAsset<Integration::MotionSetAsset>(motionSetAssetId);
        motionSetAsset.GetAs<Integration::MotionSetAsset>()->SetData(new MotionSet());
        editorAnimGraphComponent->OnAssetReady(motionSetAsset);

        // Make sure the entity exist before deletion.
        const AZ::EntityId entityID = entity->GetId();
        AZ::Entity* foundEntity{};
        AZ::ComponentApplicationBus::BroadcastResult(foundEntity, &AZ::ComponentApplicationRequests::FindEntity, entityID);
        EXPECT_TRUE(foundEntity) << "Entity should be founded after initialized and activated.";

        // Delete the entity.
        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationRequests::DeleteEntity, entityID);

        // Make sure the entity is gone after deletion.
        foundEntity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(foundEntity, &AZ::ComponentApplicationRequests::FindEntity, entityID);
        EXPECT_FALSE(foundEntity) << "Entity should NOT be founded after calling delete.";
    }
} // namespace EMotionFX
