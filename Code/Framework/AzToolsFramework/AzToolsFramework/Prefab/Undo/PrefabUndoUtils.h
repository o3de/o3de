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
            //! Create add-entity patch from entity's DOM and alias path, and append it to a given instance 
            //! patch which is an array of entity patches.
            //! @param patch An array object of DOM values which stores entity patches.
            //! @param newEntityDom The entity DOM generated from the new entity.
            //! @param newEntityAliasPath The path of the new entity for given instance patch.
            void AppendAddEntityPatch(
                PrefabDom& patch,
                const PrefabDomValue& newEntityDom,
                const AZStd::string& newEntityAliasPath);

            //! Create remove-entity patch from entity's alias path, and append it to a given instance 
            //! patch which is an array of entity patches.
            //! @param patch An array object of DOM values which stores entity patches.
            //! @param newEntityAliasPath The alias path of the entity to remove for given instance patch.
            void AppendRemoveEntityPatch(
                PrefabDom& patch,
                const AZStd::string& targetEntityAliasPath);

            //! Take an entity's before and after update state DOM, create an array of entity patches as
            //! an instance patch.
            //! @param patch A patch where an array object of DOM values which stores entity patches will
            //!     be generated.
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
        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework

