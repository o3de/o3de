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
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class PrefabFocusInterface;
        class PrefabSystemComponentInterface;

        class InstanceDomGenerator
        {
        public:
            AZ_RTTI(InstanceUpdateExecutor, "{07E23525-1D91-41AA-B85F-136360BD1938}");
            AZ_CLASS_ALLOCATOR(InstanceDomGenerator, AZ::SystemAllocator, 0);

            void Initialize();

            bool GenerateInstanceDomAccordingToCurrentFocus(const Instance* instance, PrefabDom& instanceDom) const;

        private:
            // focusedInstance is the pointer to the focused instance.
            // instanceDom is the dom of the instance that is being propagated, and will be edited by this function.
            void ReplaceFocusedContainerTransformAccordingToRoot(const Instance* focusedInstance, PrefabDom& focusedInstanceDom) const;

            static AzFramework::EntityContextId s_editorEntityContextId;

            PrefabFocusInterface* m_prefabFocusInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
