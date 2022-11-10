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
        class InstanceEntityMapperInterface;
        class InstanceToTemplateInterface;
        class PrefabSystemComponentInterface;

        class InstanceDomGenerator
            : public InstanceDomGeneratorInterface
        {
        public:
            AZ_RTTI(InstanceDomGenerator, "{07E23525-1D91-41AA-B85F-136360BD1938}", InstanceDomGeneratorInterface);
            AZ_CLASS_ALLOCATOR(InstanceDomGenerator, AZ::SystemAllocator, 0);

            void RegisterInstanceDomGeneratorInterface();
            void UnregisterInstanceDomGeneratorInterface();

            //! Generates an instance DOM for a given instance object based on the currently focused instance.
            //! If the given instance is descendant of the focused instance, instance DOM stored in focused
            //! template DOM is used; otherwise, the instance DOM stored in the root template DOM is used.
            //! In addition, container entity is updated depending on the above relationship with the focused instance.
            //! Note: Link id would be valid in the generated DOM only if the given instance is a proper descendant
            //! of the focused or root instance.
            //! @param[out] instanceDom The output instance DOM that will be modified.
            //! @param instance The given instance object.
            void GenerateInstanceDom(PrefabDom& instanceDom, const Instance& instance) const override;

            //! Generates an entity DOM for a given entity object based on the currently focused instance.
            //! If the owning instance of the given entity is descendant of the focused instance, entity DOM stored in focused
            //! template DOM is used; otherwise, the entity DOM stored in the root template DOM is used.
            //! @param[out] entityDom The output entity DOM that will be modified.
            //! @param entity The given entity object.
            void GenerateEntityDom(PrefabDom& entityDom, const AZ::Entity& entity) const override;

        private:
            //! Given an instance and its DOM, updates the container entity in the DOM with the one seen
            //! from the root template DOM.
            //! @param[out] instanceDom The DOM of the instance that will be modified.
            //! @param instance The given instance object.
            void UpdateContainerEntityInDomFromRoot(PrefabDom& instanceDom, const Instance& instance) const;

            //! Given an instance, it walks up to the root instance in the hierarchy and populates the container entity dom as seen
            //! from the root template DOM.
            //! @param[out] containerEntityDom The DOM of the container entity as seen from the root.
            //! @param instance The given instance object.
            void GenerateContainerEntityDomFromRootInstance(PrefabDom& containerEntityDom, const Instance& instance) const;

            static AzFramework::EntityContextId s_editorEntityContextId;

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
