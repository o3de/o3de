/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/limits.h>
#include <AzCore/Interface/Interface.h>

namespace AzToolsFramework::Editor
{
    /*!
     * EditorSettingsInterface
     * 
     */

    class EditorSettingsInterface
    {
    public:
        AZ_RTTI(EditorSettingsInterface, "{E7479325-6DA2-4E62-938C-EE4F4727F94B}");

        /*
        // TODO - what about an Outcome?
        virtual void RegisterIntProperty(
            AZStd::string_view category,
            AZStd::string_view subcategory,
            int defaultValue,
            int minValue = AZStd::numeric_limits<int>::min(),
            int maxValue = AZStd::numeric_limits<int>::max()) = 0;
        */

        virtual void OpenEditorSettingsDialog() = 0;

        // TEMP for testing purposes
        virtual AZStd::string GetTestSettingsList() = 0;
    };
} // namespace AzToolsFramework::Editor
