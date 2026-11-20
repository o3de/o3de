/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ::Render
{
    // System Component TypeIds
    inline constexpr const char* StarsSystemComponentTypeId = "{16A6D162-72FC-4E57-8B54-FED5D6177066}";
    inline constexpr const char* StarsEditorSystemComponentTypeId = "{096F46EC-C312-454F-B856-66140F494790}";

    // Module derived classes TypeIds
    inline constexpr const char* StarsModuleInterfaceTypeId = "{9180C4B7-AC79-4773-B3E1-F3413DDFB575}";
    inline constexpr const char* StarsModuleTypeId = "{7028B679-AF74-485D-B1BF-94CEA6DDDF78}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* StarsEditorModuleTypeId = StarsModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* StarsRequestsTypeId = "{0273D624-94B1-44A0-9D16-159BCCF1E91E}";
} // namespace AZ::Render
