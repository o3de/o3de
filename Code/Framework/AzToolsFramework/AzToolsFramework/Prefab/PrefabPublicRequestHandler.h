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

#include <AzToolsFramework/Prefab/PrefabPublicRequestBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabPublicInterface;

        class PrefabPublicRequestHandler final
            : public PrefabPublicRequestBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(PrefabPublicRequestHandler, AZ::SystemAllocator, 0);
            AZ_RTTI(PrefabPublicRequestHandler, "{83FBDDF9-10BE-4373-B1DC-44B47EE4805C}");

            static void Reflect(AZ::ReflectContext* context);

            void Connect();
            void Disconnect();

            bool CreatePrefabInMemory(const AZStd::vector<AZ::EntityId>& entityIds, AZStd::string_view filePath) override;
            AZ::EntityId InstantiatePrefab(AZStd::string_view filePath, AZ::EntityId parent, const AZ::Vector3& position) override;

        private:
            PrefabPublicInterface* m_prefabPublicInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
