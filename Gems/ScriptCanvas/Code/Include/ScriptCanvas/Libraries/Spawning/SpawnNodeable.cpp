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

#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>

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

                m_spawnTicket = AzFramework::EntitySpawnTicket(m_spawnableAsset);
            }

            SpawnNodeable::SpawnNodeable(const SpawnNodeable& rhs)
            {
                m_spawnableAsset = rhs.m_spawnableAsset;
                m_spawnTicket = AzFramework::EntitySpawnTicket(rhs.m_spawnableAsset);
            }

            void SpawnNodeable::Spawn()
            {
                AzFramework::SpawnableEntitiesInterface::Get()->SpawnAllEntities(m_spawnTicket);
            }
        }
    }
}
