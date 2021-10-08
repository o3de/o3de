/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Prefab/PrefabLoaderInterface.h>
#include <Prefab/PrefabLoaderScriptingBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        /**
        * The Scripting Prefab Loader handles scripting-friendly API requests for the prefab loader
        */
        class ScriptingPrefabLoader
            : private PrefabLoaderScriptingBus::Handler
        {
        public:
            AZ_CLASS_ALLOCATOR(ScriptingPrefabLoader, AZ::SystemAllocator, 0);
            AZ_RTTI(ScriptingPrefabLoader, "{ABC3C989-4D4F-41E7-B25B-B0FEF97177E6}");

            void Connect(PrefabLoaderInterface* prefabLoaderInterface);
            void Disconnect();
            
        private:

            //////////////////////////////////////////////////////////////////////////
            // PrefabLoaderRequestBus implementation
            AZ::Outcome<AZStd::string, void> SaveTemplateToString(TemplateId templateId) override;
            //////////////////////////////////////////////////////////////////////////

            PrefabLoaderInterface* m_prefabLoaderInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

