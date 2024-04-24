/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

//! @file contains settings that are tweakable for the Prefab system.
//! These settings are stored in the settings registry so as to be available to all parts of the system without
//! the use of the legacy settings system.
//! These values are in this file in AzToolsFramework so that it can be included without bringing
//! in any heavy-weight dependencies.

#include <AzCore/std/string/string_view.h>
namespace AzToolsFramework::Prefab::Settings
{
    constexpr AZStd::string_view DetachPrefabRemovesContainerName = "/Amazon/Preferences/Editor/General/DetachPrefabRemovesContainer";
    constexpr const bool DetachPrefabRemovesContainerDefault = true;
}
