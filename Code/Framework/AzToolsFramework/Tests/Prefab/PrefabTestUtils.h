/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <vector>
#include <memory>
#include <Prefab/Instance/Instance.h>

namespace UnitTest
{
    namespace PrefabTestUtils
    {
        using namespace AzToolsFramework::Prefab;

        template<typename... InstanceArgs>
        inline AZStd::vector<AZStd::unique_ptr<Instance>> MakeInstanceList(InstanceArgs&&... instances)
        {
            static_assert((AZStd::is_same_v<InstanceArgs, AZStd::unique_ptr<Instance>>&& ...), "All arguments must be a AZStd::unique_ptr<Instance>&&");
            AZStd::vector<AZStd::unique_ptr<Instance>> instanceList;
            instanceList.reserve(sizeof...(InstanceArgs));
            (instanceList.emplace_back(AZStd::forward<InstanceArgs>(instances)), ...);
            return instanceList;
        }

        inline AZStd::vector<AZStd::unique_ptr<Instance>> MakeInstanceList()
        {
            return {};
        }
    }
}
