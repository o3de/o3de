/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>
#include <Integration/Components/ActorComponent.h>
#include <EMotionFX/Source/Allocators.h>
#include <Tests/Integration/EntityComponentFixture.h>
#include <Tests/TestAssetCode/TestActorAssets.h>
#include <Tests/TestAssetCode/JackActor.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    TEST_F(EntityComponentFixture, ActorComponent_Attachment)
    {
        // Entity 1
        AZ::EntityId entityId(740216387);
        
        auto gameEntity = AZStd::make_unique<AZ::Entity>();
        gameEntity->SetId(entityId);
        
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));

        auto transformComponent = gameEntity->CreateComponent<AzFramework::TransformComponent>();
        auto actorComponent = gameEntity->CreateComponent<Integration::ActorComponent>();
        
        gameEntity->Init();
        gameEntity->Activate();

        actorComponent->OnAssetReady(actorAsset);
        
        // Entity 2 - attaches to previous entity
        auto gameEntityAttachment = AZStd::make_unique<AZ::Entity>();

        Integration::ActorComponent::Configuration actorConfig;
        actorConfig.m_attachmentTarget = entityId;
        actorConfig.m_attachmentType = Integration::AttachmentType::SkinAttachment;

        auto transformComponentAttachmentEntity = gameEntityAttachment->CreateComponent<AzFramework::TransformComponent>();
        auto actorComponentAttachmentEntity = gameEntityAttachment->CreateComponent<Integration::ActorComponent>(&actorConfig);

        gameEntityAttachment->Init();
        gameEntityAttachment->Activate();

        actorComponentAttachmentEntity->OnAssetReady(actorAsset);
        
        // Deactivate main actor first
        gameEntity->Deactivate();
        gameEntityAttachment->Deactivate();
        
        EXPECT_TRUE(true);
    }
    
    TEST_F(EntityComponentFixture, ActorComponent_AttachmentDeactivatesFirst)
    {
        AZ::EntityId entityId(740216387);
        
        auto gameEntity = AZStd::make_unique<AZ::Entity>();
        gameEntity->SetId(entityId);
        
        AZ::Data::AssetId actorAssetId("{5060227D-B6F4-422E-BF82-41AAC5F228A5}");
        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<JackNoMeshesActor>();
        AZ::Data::Asset<Integration::ActorAsset> actorAsset = TestActorAssets::GetAssetFromActor(actorAssetId, AZStd::move(actor));

        auto transformComponent = gameEntity->CreateComponent<AzFramework::TransformComponent>();
        auto actorComponent = gameEntity->CreateComponent<Integration::ActorComponent>();
        
        gameEntity->Init();
        gameEntity->Activate();

        actorComponent->OnAssetReady(actorAsset);
        
        // Entity 2 - attaches to previous entity        
        auto gameEntityAttachment = AZStd::make_unique<AZ::Entity>();

        Integration::ActorComponent::Configuration actorConfig;
        actorConfig.m_attachmentTarget = entityId;
        actorConfig.m_attachmentType = Integration::AttachmentType::SkinAttachment;

        auto transformComponentAttachmentEntity = gameEntityAttachment->CreateComponent<AzFramework::TransformComponent>();
        auto actorComponentAttachmentEntity = gameEntityAttachment->CreateComponent<Integration::ActorComponent>(&actorConfig);

        gameEntityAttachment->Init();
        gameEntityAttachment->Activate();

        actorComponentAttachmentEntity->OnAssetReady(actorAsset);
        
        // Deactivate attachment first
        gameEntityAttachment->Deactivate();
        gameEntity->Deactivate();
        
        EXPECT_TRUE(true);
    }
}
