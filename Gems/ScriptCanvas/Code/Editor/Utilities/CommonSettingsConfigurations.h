/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// IMPORTANT: This is NOT the permanent location for these values.
#define SCRIPTCANVASEDITOR_AZ_QCOREAPPLICATION_SETTINGS_ORGANIZATION_NAME  "O3DE"
#define SCRIPTCANVASEDITOR_NAME_SHORT   "ScriptCanvasEditor"

#include <AzCore/std/string/string.h>

namespace ScriptCanvasEditor
{
    AZStd::string GetEditingGameDataFolder();
}
