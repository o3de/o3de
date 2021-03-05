/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution(the "License").All use of this software is governed by the License,
* or , if provided, by the license below or the license accompanying this file.Do not
* remove or modify any license notices.This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
