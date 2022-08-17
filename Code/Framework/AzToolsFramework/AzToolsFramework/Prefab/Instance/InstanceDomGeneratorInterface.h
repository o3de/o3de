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

            //! Generates an instance DOM that represents a given instance object.
            //! @param[out] instanceDom The output instance DOM that will be modified.
            //! @param instance The given instance object.
            //! @return bool on whether the operation succeeds.
            virtual bool GenerateInstanceDom(PrefabDom& instanceDom, const Instance& instance) const = 0;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
