/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        namespace PrefabUndoUtils
        {
            //! Create add-entity patch from entity's DOM and alias path, and append it to a given instance patch. 
            //! @param patch An array object of DOM values which stores entity patches.
            //! @param newEntityDom The entity DOM generated from the new entity.
            //! @param newEntityAliasPath The path of the new entity for given instance patch.
            void AppendAddEntityPatch(
                PrefabDom& patch,
                const PrefabDomValue& newEntityDom,
                const AZStd::string& newEntityAliasPath);

            //! Create remove patch from an alias path, and append it to a given instance patch.
            //! @param patch An array object of DOM values which stores entity patches.
            //! @param targetAliasPath The alias path to DOM value that will be removed.
            void AppendRemovePatch(
                PrefabDom& patches,
                const AZStd::string& targetAliasPath);

            //! Create edit-entity patch, and append it to a given instance patch array.
            //! @param patch An array object of DOM values which stores entity patches will be generated.
            //! @param entityDomBeforeUpdate The DOM presenting state of an entity before update.
            //! @param entityDomAfterUpdate The DOM presenting state of an entity after update.
            //! @param entityAliasPath The alias path of the entity to update for given instance patch.
            void AppendUpdateEntityPatch(
                PrefabDom& patch,
                const PrefabDomValue& entityDomBeforeUpdate,
                const PrefabDomValue& entityDomAfterUpdate,
                const AZStd::string& entityAliasPath);

            //! Take an entity's before and after update state DOM, create an instance patch of entity patches.
            //! @param patch An array object of DOM values which stores entity patches will be generated.
            //! @param entityDomBeforeUpdate The DOM presenting state of an entity before update.
            //! @param entityDomAfterUpdate The DOM presenting state of an entity after update.
            //! @param entityAliasPathForPatches The alias path of the entity to update for given instance patch.
            void GenerateUpdateEntityPatch(
                PrefabDom& patch,
                const PrefabDomValue& entityDomBeforeUpdate,
                const PrefabDomValue& entityDomAfterUpdate,
                const AZStd::string& entityAliasPathForPatches);

            //! Update the provided cached instance DOM corresponding to the entity with its patch DOM and alias path.
            //! @param cachedOwningInstanceDom Cached instance DOM of an entity's owning instance.
            //! @param entityDom The entity's patch DOM.
            //! @param entityAliasPath The alias path as position where the entity's patch DOM will be stored.
            void UpdateCachedOwningInstanceDom(
                PrefabDomReference cachedOwningInstanceDom,
                const PrefabDomValue& entityDom,
                const AZStd::string& entityAliasPath);

            //! Remove the DOM (entity or instance) pointed by the provided alias path in the cached instance DOM.
            //! @param cachedOwningInstanceDom Cached instance DOM that is provided.
            //! @param aliasPath The alias path as position where the DOM will be removed.
            void UpdateCachedOwningInstanceDomWithRemoval(
                PrefabDomReference cachedOwningInstanceDom,
                const AZStd::string& aliasPath);
        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework
