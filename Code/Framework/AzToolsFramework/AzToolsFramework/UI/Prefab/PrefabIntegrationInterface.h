/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>

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
        };

    } // namespace Prefab
} // namespace AzToolsFramework

