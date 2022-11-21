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
            //! Create an add-entity patch for new entity with alias path, and append it to patch array.
            //! @param patches An array object of DOM values which stores undo or redo patches.
            //! @param newEntityDom The entity DOM generated from the new entity.
            //! @param newEntityAliasPath The given alias path for the new entity.
            void AppendAddEntityPatch(
                PrefabDom& patches,
                const PrefabDomValue& newEntityDom,
                const AZStd::string& newEntityAliasPath);

            //! Create a remove patch with alias path, and append it to patch array.
            //! @param patches An array object of DOM values which stores undo or redo patches.
            //! @param pathToRemove The given path to where the DOM value will be removed.
            void AppendRemovePatch(
                PrefabDom& patches,
                const AZStd::string& pathToRemove);

            //! Create edit-entity patch(es), and append them to patch array.
            //! @param patches An array object of DOM values which stores undo or redo patches.
            //! @param entityDomBeforeUpdate The DOM presenting state of an entity before update.
            //! @param entityDomAfterUpdate The DOM presenting state of an entity after update.
            //! @param entityAliasPath The given alias path for the entity to be updated.
            void AppendUpdateEntityPatch(
                PrefabDom& patches,
                const PrefabDomValue& entityDomBeforeUpdate,
                const PrefabDomValue& entityDomAfterUpdate,
                const AZStd::string& entityAliasPath);

            //! Create edit-entity patch(es), and output them to patch array.
            //! Note: It will overwrite the given patch array with new patches.
            //! @param patches An array object of DOM values which stores undo or redo patches.
            //! @param entityDomBeforeUpdate The DOM presenting state of an entity before update.
            //! @param entityDomAfterUpdate The DOM presenting state of an entity after update.
            //! @param entityAliasPath The given alias path for the entity to be updated.
            void GenerateUpdateEntityPatch(
                PrefabDom& patches,
                const PrefabDomValue& entityDomBeforeUpdate,
                const PrefabDomValue& entityDomAfterUpdate,
                const AZStd::string& entityAliasPath);

            //! Update the entity in instance DOM with the provided entity DOM.
            //! @param instanceDom The given instance DOM.
            //! @param entityDom The entity DOM that will be put in the instance DOM.
            //! @param entityAliasPath The given alias path to the entity.
            void UpdateEntityInInstanceDom(
                PrefabDomReference instanceDom,
                const PrefabDomValue& entityDom,
                const AZStd::string& entityAliasPath);

            //! Remove DOM value in instance DOM.
            //! @param instanceDom The given instance DOM.
            //! @param pathToRemove The path to where the value will be removed.
            void RemoveValueInInstanceDom(
                PrefabDomReference instanceDom,
                const AZStd::string& pathToRemove);
        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework
