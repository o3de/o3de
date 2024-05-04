/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Name/Name.h>
#include <AzCore/Name/NameDictionary.h>

namespace AzFramework
{
    static const AZ::Name ScriptCanvasToolsName = AZ::Name::FromStringLiteral("ScriptCanvasRemoteTools", nullptr);
    static constexpr AZ::Crc32 ScriptCanvasToolsKey("ScriptCanvasRemoteTools");
    static constexpr uint16_t ScriptCanvasToolsPort = 6787;
}
