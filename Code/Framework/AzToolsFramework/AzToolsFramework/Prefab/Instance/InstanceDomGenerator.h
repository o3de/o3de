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
#include <AzToolsFramework/Prefab/Instance/InstanceDomGeneratorInterface.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Instance;
        class PrefabFocusInterface;
        class PrefabSystemComponentInterface;

        class InstanceDomGenerator
            : public InstanceDomGeneratorInterface
        {
        public:
            AZ_RTTI(InstanceDomGenerator, "{07E23525-1D91-41AA-B85F-136360BD1938}", InstanceDomGeneratorInterface);
            AZ_CLASS_ALLOCATOR(InstanceDomGenerator, AZ::SystemAllocator, 0);

            bool GenerateInstanceDom(const Instance* instance, PrefabDom& instanceDom) override;

            void RegisterInstanceDomGeneratorInterface();
            void UnregisterInstanceDomGeneratorInterface();

        private:
            /**
             * Update the container entity's transform from focused instance and its dom according to the root.
             * @param instance The pointer to the focused instance.
             * @param instanceDom The dom of the instance that will be edited by this function.
             */
            void ReplaceFocusedContainerTransformAccordingToRoot(const Instance* instance, PrefabDom& instanceDom) const;

            static AzFramework::EntityContextId s_editorEntityContextId;

            PrefabFocusInterface* m_prefabFocusInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    }
}
