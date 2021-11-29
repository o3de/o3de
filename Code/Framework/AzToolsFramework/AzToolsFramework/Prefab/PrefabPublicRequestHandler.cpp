/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabPublicRequestHandler.h>

#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabPublicRequestHandler::Reflect(AZ::ReflectContext* context)
        {
            AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
            if (behaviorContext)
            {
                behaviorContext->EBus<PrefabPublicRequestBus>("PrefabPublicRequestBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                    ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                    ->Attribute(AZ::Script::Attributes::Module, "prefab")
                    ->Event("CreatePrefabInMemory", &PrefabPublicRequests::CreatePrefabInMemory)
                    ->Event("InstantiatePrefab", &PrefabPublicRequests::InstantiatePrefab)
                    ->Event("DeleteEntitiesAndAllDescendantsInInstance", &PrefabPublicRequests::DeleteEntitiesAndAllDescendantsInInstance)
                    ->Event("DetachPrefab", &PrefabPublicRequests::DetachPrefab)
                    ->Event("DuplicateEntitiesInInstance", &PrefabPublicRequests::DuplicateEntitiesInInstance)
                    ->Event("GetOwningInstancePrefabPath", &PrefabPublicRequests::GetOwningInstancePrefabPath)
                    ;
            }
        }

        void PrefabPublicRequestHandler::Connect()
        {
            m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
            AZ_Assert(m_prefabPublicInterface, "PrefabPublicRequestHandler - Could not retrieve instance of PrefabPublicInterface");

            PrefabPublicRequestBus::Handler::BusConnect();
        }

        void PrefabPublicRequestHandler::Disconnect()
        {
            PrefabPublicRequestBus::Handler::BusDisconnect();

            m_prefabPublicInterface = nullptr;
        }

        CreatePrefabResult PrefabPublicRequestHandler::CreatePrefabInMemory(const EntityIdList& entityIds, AZStd::string_view filePath)
        {
            return m_prefabPublicInterface->CreatePrefabInMemory(entityIds, filePath);
        }

        InstantiatePrefabResult PrefabPublicRequestHandler::InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position)
        {
            return m_prefabPublicInterface->InstantiatePrefab(filePath, parent, position);
        }

        PrefabOperationResult PrefabPublicRequestHandler::DeleteEntitiesAndAllDescendantsInInstance(const EntityIdList& entityIds)
        {
            return m_prefabPublicInterface->DeleteEntitiesAndAllDescendantsInInstance(entityIds);
        }

        PrefabOperationResult PrefabPublicRequestHandler::DetachPrefab(const AZ::EntityId& containerEntityId)
        {
            return m_prefabPublicInterface->DetachPrefab(containerEntityId);
        }

        DuplicatePrefabResult PrefabPublicRequestHandler::DuplicateEntitiesInInstance(const EntityIdList& entityIds)
        {
            return m_prefabPublicInterface->DuplicateEntitiesInInstance(entityIds);
        }

        AZStd::string PrefabPublicRequestHandler::GetOwningInstancePrefabPath(AZ::EntityId entityId) const
        {
            return m_prefabPublicInterface->GetOwningInstancePrefabPath(entityId).Native();
        }
    } // namespace Prefab
} // namespace AzToolsFramework
