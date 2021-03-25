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

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        using PrefabDom = rapidjson::Document;
        using PrefabDomValue = rapidjson::Value;
        using PrefabDomPath = rapidjson::Pointer;
        using PrefabDomList = AZStd::vector<PrefabDom>;

        using PrefabDomValueReference = AZStd::optional<AZStd::reference_wrapper<PrefabDomValue>>;
        using PrefabDomValueConstReference = AZStd::optional<AZStd::reference_wrapper<const PrefabDomValue>>;

    } // namespace Prefab
} // namespace AzToolsFramework

