/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string.h>
#include <AzToolsFramework/Prefab/Overrides/PrefabOverrideTypes.h>
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

            //! Create an 'add' or 'replace' patch for updating value in DOM.
            //! To remove value, use AppendRemovePatch.
            //! @param patches An array object of DOM values which stores undo or redo patches.
            //! @param domValue The DOM value to add or replace.
            //! @param pathToUpdate The given path to the value.
            //! @param patchType The patch type for the patch (supported value: Add and Edit).
            void AppendUpdateValuePatch(
                PrefabDom& patches,
                const PrefabDomValue& domValue,
                const AZStd::string& pathToUpdate,
                const PatchType patchType);

            //! Create a 'remove' patch with alias path, and append it to patch array.
            //! @param patches An array object of DOM values which stores undo or redo patches.
            //! @param pathToRemove The given path to where the DOM value will be removed.
            void AppendRemovePatch(
                PrefabDom& patches,
                const AZStd::string& pathToRemove);

            //! Create patches by comparing DOM states before and after update, and append them to patch array.
            //! @param patches An array object of DOM values which stores undo or redo patches.
            //! @param domValueBeforeUpdate The DOM presenting state of the value before update.
            //! @param domValueAfterUpdate The DOM presenting state of the value after update.
            //! @param pathToValue The given path to the value.
            void GenerateAndAppendPatch(
                PrefabDom& patches,
                const PrefabDomValue& domValueBeforeUpdate,
                const PrefabDomValue& domValueAfterUpdate,
                const AZStd::string& pathToValue);

            //! Update the entity in prefab DOM with the provided entity DOM.
            //! @param prefabDom The given prefab DOM.
            //! @param entityDom The entity DOM that will be put in the prefab DOM.
            //! @param entityAliasPath The given alias path to the entity.
            void UpdateEntityInPrefabDom(
                PrefabDomReference prefabDom,
                const PrefabDomValue& entityDom,
                const AZStd::string& entityAliasPath);

            //! Update the value in prefab DOM with the provided DOM value.
            //! @param prefabDom The given prefab DOM.
            //! @param domValue The DOM value that will be put in the prefab DOM.
            //! @param pathToValue The given path to the value.
            void UpdateValueInPrefabDom(
                PrefabDomReference prefabDom,
                const PrefabDomValue& domValue,
                const AZStd::string& pathToValue);

            //! Remove DOM value in prefab DOM.
            //! @param prefabDom The given prefab DOM.
            //! @param pathToRemove The path to where the value will be removed.
            void RemoveValueInPrefabDom(
                PrefabDomReference prefabDom,
                const AZStd::string& pathToRemove);

        } // namespace PrefabUndoUtils
    } // namespace Prefab
} // namespace AzToolsFramework
