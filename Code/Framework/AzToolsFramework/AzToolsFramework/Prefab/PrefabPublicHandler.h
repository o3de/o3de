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

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Math/Vector3.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>


using EntityList = AZStd::vector<AZ::Entity*>;

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class InstanceEntityMapperInterface;

        class PrefabPublicHandler final
            : public PrefabPublicInterface
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabPublicHandler, AZ::SystemAllocator, 0);
            AZ_RTTI(PrefabPublicHandler, "{35802943-6B60-430F-9DED-075E3A576A25}", PrefabPublicInterface);

            PrefabPublicHandler();
            ~PrefabPublicHandler();

            PrefabOperationResult CreatePrefab(const AZStd::vector<AZ::EntityId>& entityIds, AZStd::string_view filePath) override;
            PrefabOperationResult InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, AZ::Vector3 position) override;

            bool IsInstanceContainerEntity(AZ::EntityId entityId) override;
            AZ::EntityId GetInstanceContainerEntityId(AZ::EntityId entityId) override;

        private:

            bool RetrieveAndSortPrefabEntitiesAndInstances(const EntityList& inputEntities, const Instance& commonRootEntityOwningInstance,
                EntityList& outEntities, AZStd::vector<AZStd::unique_ptr<Instance>>& outInstances) const;

            //! Gets the owning instance of a valid commonRootEntity and the root prefab instance for an invalid commonRootEntity.
            InstanceOptionalReference GetCommonRootEntityOwningInstance(AZ::EntityId entityId);
            static Instance* GetParentInstance(Instance* instance);
            static Instance* GetAncestorOfInstanceThatIsChildOfRoot(const Instance* ancestor, Instance* descendant);
            static void GenerateContainerEntityTransform(const EntityList& topLevelEntities, AZ::Vector3& translation, AZ::Quaternion& rotation);

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface;
        };
    }
}
