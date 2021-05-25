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

#pragma optimize("", off)
#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>

#include <AzFramework/Components/TransformComponent.h>

namespace ScriptCanvas
{
    namespace Nodeables
    {
        namespace Spawning
        {
            SpawnNodeable::SpawnNodeable()
            {
                AZ::Data::AssetId cubeAssetId("{90552CB9-2F29-5A2E-976C-61BF7ADACB81}", 272062881);
                m_spawnableAsset = AZ::Data::AssetManager::Instance().GetAsset<AzFramework::Spawnable>(cubeAssetId, AZ::Data::AssetLoadBehavior::PreLoad);
                AZ::Data::AssetManager::Instance().BlockUntilLoadComplete(m_spawnableAsset);
            }

            SpawnNodeable::SpawnNodeable(const SpawnNodeable& rhs)
            {
                m_spawnableAsset = rhs.m_spawnableAsset;
            }

            void SpawnNodeable::OnInitializeExecutionState()
            {
                m_spawnTicket = AzFramework::EntitySpawnTicket(m_spawnableAsset);
            }

            void SpawnNodeable::OnDeactivate()
            {
                m_spawnTicket = AzFramework::EntitySpawnTicket();
            }

            //void SpawnNodeable::Translation(Data::Vector3Type translation)
            //{
            //    m_translation = translation;
            //}

            //void SpawnNodeable::Rotation(Data::Vector3Type rotation)
            //{
            //    m_rotation = rotation;
            //}

            //void SpawnNodeable::Scale(Data::Vector3Type scale)
            //{
            //    m_scale = scale;
            //}

            void SpawnNodeable::RequestSpawn(Data::Vector3Type translation, Data::Vector3Type rotation, Data::NumberType scale)
            {
                auto preSpawnCB = [this, translation, rotation, scale]([[maybe_unused]] AzFramework::EntitySpawnTicket& ticket,
                    AzFramework::SpawnableEntityContainerView view)
                {

                    AZ::Entity* rootEntity = *view.begin();

                    AzFramework::TransformComponent* entityTransform =
                        rootEntity->FindComponent<AzFramework::TransformComponent>();

                    if (entityTransform)
                    {
                        AZ::Vector3 rotationCopy = rotation;
                        AZ::Quaternion rotationQuat = AZ::Quaternion::CreateFromEulerAnglesDegrees(rotationCopy);

                        entityTransform->SetWorldTM(AZ::Transform(translation, rotationQuat, AZ::Vector3(scale, scale, scale)));
                    }
                };

                auto spawnCompleteCB = [this]([[maybe_unused]] AzFramework::EntitySpawnTicket& ticket,
                    AzFramework::SpawnableConstEntityContainerView view)
                {
                    AZStd::vector<Data::EntityIDType> spawnedEntities;
                    spawnedEntities.resize(view.size());

                    for (const AZ::Entity* entity : view)
                    {
                        spawnedEntities.emplace_back(entity->GetId());
                    }

                    CallOnSpawn(spawnedEntities);
                };

                AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(m_spawnTicket, preSpawnCB, spawnCompleteCB);
            }
        }
    }
}
