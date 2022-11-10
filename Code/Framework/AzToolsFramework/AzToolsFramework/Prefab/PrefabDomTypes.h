/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        using PrefabDomAllocator = PrefabDom::AllocatorType;

        using PrefabDomReference = AZStd::optional<AZStd::reference_wrapper<PrefabDom>>;
        using PrefabDomConstReference = AZStd::optional<AZStd::reference_wrapper<const PrefabDom>>;
        using PrefabDomValueReference = AZStd::optional<AZStd::reference_wrapper<PrefabDomValue>>;
        using PrefabDomValueConstReference = AZStd::optional<AZStd::reference_wrapper<const PrefabDomValue>>;

    } // namespace Prefab
} // namespace AzToolsFramework

