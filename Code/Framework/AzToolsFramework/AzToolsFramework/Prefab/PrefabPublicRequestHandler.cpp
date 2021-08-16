/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabPublicRequestHandler.h>

#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>

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

        bool PrefabPublicRequestHandler::CreatePrefabInMemory(const AZStd::vector<AZ::EntityId>& entityIds, AZStd::string_view filePath)
        {
            auto createPrefabOutcome = m_prefabPublicInterface->CreatePrefabInMemory(entityIds, filePath);
            if (!createPrefabOutcome.IsSuccess())
            {
                AZ_Error("CreatePrefabInMemory", false,
                    "Failed to create Prefab on file path '%.*s'. Error message: %s.",
                    AZ_STRING_ARG(filePath),
                    createPrefabOutcome.GetError().c_str());

                return false;
            }

            return true;
        }

        AZ::EntityId PrefabPublicRequestHandler::InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position)
        {
            auto instantiatePrefabOutcome = m_prefabPublicInterface->InstantiatePrefab(filePath, parent, position);
            if (!instantiatePrefabOutcome.IsSuccess())
            {
                AZ_Error("InstantiatePrefab", false,
                    "Failed to instantiate Prefab on file path '%.*s'. Error message: %s.",
                    AZ_STRING_ARG(filePath),
                    instantiatePrefabOutcome.GetError().c_str());

                return AZ::EntityId();
            }

            return instantiatePrefabOutcome.GetValue();
        }
    } // namespace Prefab
} // namespace AzToolsFramework
