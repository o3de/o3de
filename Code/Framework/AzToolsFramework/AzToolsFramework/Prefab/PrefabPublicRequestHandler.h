/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>
#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        
        class PrefabPublicRequestHandler final
            : public PrefabPublicRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabPublicRequestHandler, AZ::SystemAllocator, 0);
            AZ_RTTI(PrefabPublicRequestHandler, "{83FBDDF9-10BE-4373-B1DC-44B47EE4805C}");

            static void Reflect(AZ::ReflectContext* context)
            {
                AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context);
                if (behaviorContext)
                {
                    behaviorContext->EBus<PrefabPublicRequestBus>("PrefabPublicRequestBus")
                        ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                        ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                        ->Attribute(AZ::Script::Attributes::Module, "prefab")
                        ->Event("CreatePrefabInDisk", &PrefabPublicRequests::CreatePrefabInDisk)
                        ->Event("CreatePrefabInMemory", &PrefabPublicRequests::CreatePrefabInMemory)
                        ->Event("InstantiatePrefab", &PrefabPublicRequests::InstantiatePrefab)
                        ;
                }
            }

            void Connect()
            {
                m_prefabPublicInterface = AZ::Interface<Prefab::PrefabPublicInterface>::Get();
                AZ_Assert(m_prefabPublicInterface, "PrefabPublicRequestHandler - Could not retrieve instance of PrefabPublicInterface");

                PrefabPublicRequestBus::Handler::BusConnect();
            }

            void Disconnect()
            {
                PrefabPublicRequestBus::Handler::BusDisconnect();

                m_prefabPublicInterface = nullptr;
            }

            bool CreatePrefabInDisk(const AZStd::vector<AZ::EntityId>& entityIds, AZStd::string_view filePath) override
            {
                auto createPrefabOutcome = m_prefabPublicInterface->CreatePrefabInDisk(entityIds, filePath);
                if (!createPrefabOutcome.IsSuccess())
                {
                    AZ_Error("CreatePrefabInDisk", false,
                        "Failed to create Prefab on file path '%.*s'. Error message: %s.",
                        AZ_STRING_ARG(filePath),
                        createPrefabOutcome.GetError().c_str());

                    return false;
                }

                return true;
            }

            bool CreatePrefabInMemory(const AZStd::vector<AZ::EntityId>& entityIds, AZStd::string_view filePath) override
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

            AZ::EntityId InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position) override
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

        private:
            PrefabPublicInterface* m_prefabPublicInterface = nullptr;

        };
    } // namespace Prefab
} // namespace AzToolsFramework
