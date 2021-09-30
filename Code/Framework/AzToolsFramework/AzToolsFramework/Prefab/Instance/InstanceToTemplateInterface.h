/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#include <AzCore/Component/EntityId.h>
#include <AzCore/RTTI/RTTI.h>

#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/Template/Template.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceToTemplateInterface
        {
        public:
            AZ_RTTI(InstanceToTemplateInterface, "{9EF54D0F-0951-40B6-91AB-2EB55A322692}");
            virtual ~InstanceToTemplateInterface() = default;

            //! Generates a prefabdom for the entity in its current state and places the result in generatedDom
            virtual bool GenerateDomForEntity(PrefabDom& generatedEntityDom, const AZ::Entity& entity) = 0;

            //! Generates a prefabdom for the instance in its current state and places the result in generatedDom
            virtual bool GenerateDomForInstance(PrefabDom& generatedInstanceDom, const Instance& instance) = 0;

            //! Generates a patch using serialization system and places the result in generatedPatch
            virtual bool GeneratePatch(PrefabDom& generatedPatch, const PrefabDom& initialState, const PrefabDom& modifiedState) = 0;

            //! Generates a patch to be associated with a link with the given LinkId and places the result in generatedPatch
            virtual bool GeneratePatchForLink(PrefabDom& generatedPatch, const PrefabDom& initialState,
                const PrefabDom& modifiedState, const LinkId linkId) = 0;

            //! Updates the affected template for a given entityId using the providedPatch
            virtual bool PatchEntityInTemplate(PrefabDom& providedPatch, AZ::EntityId entityId) = 0;

            virtual void AppendEntityAliasToPatchPaths(PrefabDom& providedPatch, const AZ::EntityId& entityId) = 0;

            //! Updates the template links (updating instances) for the given template and triggers propagation on its instances.
            //! @param providedPatch The patch to apply to the template.
            //! @param templateId The id of the template to update.
            //! @param immediate An optional flag whether to apply the patch immediately (needed for Undo/Redos) or wait until next system tick.
            //! @param instanceToExclude An optional reference to an instance of the template being updated that should not be refreshes as part of propagation.
            //!     Defaults to nullopt, which means that all instances will be refreshed.
            //! @return True if the template was patched correctly, false if the operation failed.
            virtual bool PatchTemplate(PrefabDomValue& providedPatch, TemplateId templateId, bool immediate = false, InstanceOptionalReference instanceToExclude = AZStd::nullopt) = 0;

            virtual void ApplyPatchesToInstance(const AZ::EntityId& entityId, PrefabDom& patches, const Instance& instanceToAddPatches) = 0;

            virtual InstanceOptionalReference GetTopMostInstanceInHierarchy(AZ::EntityId entityId) = 0;
        };
    }
}
