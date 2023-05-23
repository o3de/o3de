/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/string/string_view.h>

namespace PrefabIdentifiers
{
    inline constexpr AZStd::string_view PrefabFocusChangedUpdaterIdentifier = "o3de.updater.onPrefabFocusChanged";
    inline constexpr AZStd::string_view PrefabInstancePropagationEndUpdaterIdentifier = "o3de.updater.onPrefabInstancePropagationEnd";
    inline constexpr AZStd::string_view PrefabUnsavedStateChangedUpdaterIdentifier = "o3de.updater.onPrefabUnsavedStateChanged";
} // namespace PrefabIdentifiers
