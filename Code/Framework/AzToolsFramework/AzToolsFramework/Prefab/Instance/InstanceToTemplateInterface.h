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

            //! Generates a prefab dom for the entity in its current state and places the result in generatedDom
            virtual bool GenerateDomForEntity(PrefabDom& generatedEntityDom, const AZ::Entity& entity) = 0;

            //! Generates a prefab dom for the instance in its current state and places the result in generatedDom
            virtual bool GenerateDomForInstance(PrefabDom& generatedInstanceDom, const Instance& instance) = 0;

            //! Generates a patch using serialization system and places the result in generatedPatch
            virtual bool GeneratePatch(PrefabDom& generatedPatch, const PrefabDomValue& initialState, const PrefabDomValue& modifiedState) = 0;

            //! Generates a patch to be associated with a link with the given LinkId and places the result in generatedPatch
            virtual bool GeneratePatchForLink(PrefabDom& generatedPatch, const PrefabDom& initialState,
                const PrefabDom& modifiedState, const LinkId linkId) = 0;

            //! Updates the affected template for a given entityId using the providedPatch
            virtual bool PatchEntityInTemplate(PrefabDom& providedPatch, AZ::EntityId entityId) = 0;

            //! Generates a string matching the path to the entity alias corresponding to the entity id.
            //! @param entityId The entity id to use for generating alias path
            //! @return The string matching the path to the entity alias
            virtual AZStd::string GenerateEntityAliasPath(AZ::EntityId entityId) = 0;

            //! Generates a path to the entity matching the id from the focused prefab.
            //! @param entityId The entity id to fetch the path for.
            //! @return The path to the entity with the given id.
            virtual AZ::Dom::Path GenerateEntityPathFromFocusedPrefab(AZ::EntityId entityId) = 0;

            //! Generates an entity alias from given entity id and prefix and then add it as path into the provided patch.
            //! @param providedPatch The patch to add entity alias path to.
            //! @param entityId The entity id to use for generating alias path.
            //! @param prefix The prefix that will be added in front of the generated entity alias from entityId. 
            virtual void AppendEntityAliasToPatchPaths(PrefabDom& providedPatch, AZ::EntityId entityId, const AZStd::string& prefix = "") = 0;

            //! Add given entity alias path into the provided patch.
            //! @param providedPatch The patch to add entity alias path to.
            //! @param entityAliasPath The entity alias path to be added as path into the patch.
            virtual void AppendEntityAliasPathToPatchPaths(PrefabDom& providedPatch, const AZStd::string& entityAliasPath) = 0;

            //! Updates the template links (updating instances) for the given template and triggers propagation on its instances.
            //! @param providedPatch The patch to apply to the template.
            //! @param templateId The id of the template to update.
            //! @param instanceToExclude An optional reference to an instance of the template being updated that should not be refreshes as part of propagation.
            //!     Defaults to nullopt, which means that all instances will be refreshed.
            //! @return True if the template was patched correctly, false if the operation failed.
            virtual bool PatchTemplate(PrefabDomValue& providedPatch, TemplateId templateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) = 0;

            virtual void ApplyPatchesToInstance(const AZ::EntityId& entityId, PrefabDom& patches, const Instance& instanceToAddPatches) = 0;

            virtual InstanceOptionalReference GetTopMostInstanceInHierarchy(AZ::EntityId entityId) = 0;
        };
    }
}
