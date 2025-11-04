/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/RTTI.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class InstanceDomGeneratorInterface
        {
        public:
            AZ_RTTI(InstanceDomGeneratorInterface, "{269DE807-64B2-4157-93B0-BEDA4133C9A0}");
            virtual ~InstanceDomGeneratorInterface() = default;

            //! Gets a copy of instance DOM that represents a given instance object from template.
            //! Caller should check if the generated DOM is a valid JSON object.
            //! @param[out] instanceDom The output instance DOM that will be modified.
            //! @param instance The given instance object.
            virtual void GetInstanceDomFromTemplate(PrefabDom& instanceDom, const Instance& instance) const = 0;

            //! Gets a copy of entity DOM that represents a given entity object from template.
            //! Caller should check if the generated DOM is a valid JSON object.
            //! @param[out] entityDom The output entity DOM that will be modified.
            //! @param entity The given entity object.
            virtual void GetEntityDomFromTemplate(PrefabDom& entityDom, const AZ::Entity& entity) const = 0;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
