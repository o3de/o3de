/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/limits.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        using LinkId = AZ::u64;
        using TemplateId = AZ::u64;

        inline static constexpr LinkId InvalidLinkId = AZStd::numeric_limits<LinkId>::max();
        inline static constexpr TemplateId InvalidTemplateId = AZStd::numeric_limits<TemplateId>::max();
    } // namespace Prefab
} // namespace AzToolsFramework

