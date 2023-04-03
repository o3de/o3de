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
        struct InstanceClimbUpResult;

        class InstanceDomGenerator
            : public InstanceDomGeneratorInterface
        {
        public:
            AZ_RTTI(InstanceDomGenerator, "{07E23525-1D91-41AA-B85F-136360BD1938}", InstanceDomGeneratorInterface);
            AZ_CLASS_ALLOCATOR(InstanceDomGenerator, AZ::SystemAllocator);

            void RegisterInstanceDomGeneratorInterface();
            void UnregisterInstanceDomGeneratorInterface();

            //! Gets a copy of instance DOM for a given instance object from template based on the currently focused instance.
            //! If the given instance is descendant of the focused instance, instance DOM stored in focused
            //! template DOM is used; otherwise, the instance DOM stored in the root template DOM is used.
            //! Note: Link id would be valid in the generated DOM only if the given instance is a proper descendant
            //! of the focused or root instance.
            //! @param[out] instanceDom The output instance DOM that will be modified. It must be empty so that it can be filled.
            //! @param instance The given instance object.
            void GetInstanceDomFromTemplate(PrefabDom& instanceDom, const Instance& instance) const override;

            //! Gets a copy of entity DOM for a given entity object from template based on the currently focused instance.
            //! If the owning instance of the given entity is descendant of the focused instance, entity DOM stored in focused
            //! template DOM is used; otherwise, the entity DOM stored in the root template DOM is used.
            //! @param[out] entityDom The output entity DOM that will be modified. It must be empty so that it can be filled.
            //! @param entity The given entity object.
            void GetEntityDomFromTemplate(PrefabDom& entityDom, const AZ::Entity& entity) const override;

        private:
            //! Given an instance and its DOM, updates the container entity in the DOM with the one seen
            //! from the highest ancestor template DOM that contains the data.
            //! @param[out] instanceDom The DOM of the instance that will be modified.
            //! @param instance The given instance object.
            void UpdateContainerEntityInDomFromHighestAncestor(PrefabDom& instanceDom, const Instance& instance) const;

            //! Given an instance and a provided DOM, populates the DOM with the container entity DOM as seen
            //! from the template DOM of the highest ancestor that contains the data, or from the instance if it's the root.
            //! @param[out] containerEntityDom The DOM of the container entity as seen from the highest ancestor or root.
            //! @param instance The given instance object.
            void GenerateContainerEntityDomFromHighestAncestorOrSelf(PrefabDom& containerEntityDom, const Instance& instance) const;

            //! Given a climb-up path and a provided DOM, populates the DOM with the container entity DOM as seen
            //! from the template DOM of the highest ancestor that contains the data, or from the instance if it's the root.
            //! @param[out] containerEntityDom The DOM of the container entity as seen from the highest ancestor or root.
            //! @param climbUpResult The given climb-up object that contains the root and a path from an instance to the root.
            void GenerateContainerEntityDomFromClimbUpResult(PrefabDom& containerEntityDom, const InstanceClimbUpResult& climbUpResult) const;

            static AzFramework::EntityContextId s_editorEntityContextId;

            InstanceEntityMapperInterface* m_instanceEntityMapperInterface = nullptr;
            InstanceToTemplateInterface* m_instanceToTemplateInterface = nullptr;
            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
