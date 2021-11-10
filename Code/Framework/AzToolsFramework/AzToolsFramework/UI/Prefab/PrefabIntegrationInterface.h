/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabIntegrationInterface
        {
        public:
            AZ_RTTI(PrefabIntegrationInterface, "{B88AE045-7711-49BC-8336-45003D1C9116}");

            /**
             * Create a new entity at the position provided.
             * @param position The position the new entity will be created at.
             * @param parentId The id of the parent of the newly created entity.
             * @return The id of the newly created entity.
             */
            virtual AZ::EntityId CreateNewEntityAtPosition(const AZ::Vector3& position, AZ::EntityId parentId) = 0;

            //! Constructs and executes the close dialog on a prefab template corresponding to templateId.
            //! @param templateId The id of the template the user chose to close.
            virtual int ExecuteClosePrefabDialog(TemplateId templateId) = 0;

            //! Constructs and executes the save dialog on a prefab template corresponding to templateId.
            //! @param templateId The id of the template the user chose to save.
            //! @param useSaveAllPrefabsPreference A flag indicating whether SaveAllPrefabsPreference should be used for saving templates. 
            virtual void ExecuteSavePrefabDialog(TemplateId templateId, bool useSaveAllPrefabsPreference = false) = 0;
        };

    } // namespace Prefab
} // namespace AzToolsFramework

