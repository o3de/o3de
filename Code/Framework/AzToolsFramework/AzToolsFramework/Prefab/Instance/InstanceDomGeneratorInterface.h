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

            // Given a pointer to an instance and a reference to a dom, update the dom so that it represents the instance,
            // and return if the dom creation succeeded or not.
            virtual bool GenerateInstanceDom(const Instance* instance, PrefabDom& instanceDom) = 0;
        };
    }
}
