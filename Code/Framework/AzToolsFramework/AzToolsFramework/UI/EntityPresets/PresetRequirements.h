/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        //! A component a preset needs, and the gem that supplies it (named in messages to the user).
        struct RequiredComponent
        {
            AZStd::string m_componentName;
            AZStd::string m_gemName;
        };

        //! Those of @p required whose component is not registered in this editor.
        AZTF_API AZStd::vector<RequiredComponent> MissingComponents(
            const AZStd::vector<RequiredComponent>& required);

        //! Tell the user the preset cannot be created and what is missing. Call before opening an undo batch.
        AZTF_API void ReportMissingRequirements(
            const AZStd::string& presetName, const AZStd::vector<RequiredComponent>& missing);

        //! Ask whether to build a reduced version of the preset when optional parts are missing.
        //! @param reduction What the user gets instead, e.g. "without vegetation".
        //! @return False if the user cancelled.
        AZTF_API bool ConfirmReducedSetup(
            const AZStd::string& presetName,
            const AZStd::string& reduction,
            const AZStd::vector<RequiredComponent>& missing);
    } // namespace EntityPresets
} // namespace AzToolsFramework
