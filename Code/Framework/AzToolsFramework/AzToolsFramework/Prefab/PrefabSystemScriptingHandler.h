/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <Prefab/PrefabSystemScriptingBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabSystemScriptingHandler
            : PrefabSystemScriptingBus::Handler
        {
        public:
            static void Reflect(AZ::ReflectContext* context);

            PrefabSystemScriptingHandler() = default;

            void Connect(PrefabSystemComponentInterface* prefabSystemComponentInterface);
            void Disconnect();

        private:
            AZ_DISABLE_COPY(PrefabSystemScriptingHandler);

            //////////////////////////////////////////////////////////////////////////
            // PrefabSystemScriptingBus implementation
            TemplateId CreatePrefabTemplate(const AZStd::vector<AZ::EntityId>& entityIds, const AZStd::string& filePath) override;
            //////////////////////////////////////////////////////////////////////////

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

