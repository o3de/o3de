/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/Path/Path.h>

namespace AZ::SceneAPI::Events
{
    //! This structure is used to store the pattern and script path to execute when the pattern matches an asset source.
    struct ScriptConfig
    {
        AZStd::string m_pattern;
        AZ::IO::FixedMaxPath m_scriptPath;
    };

    //! These events are used to manage the default script rules.
    class ScriptConfigEvents
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual ~ScriptConfigEvents() = default;

        //! Copies the script config entries into scriptConfigList.
        virtual void GetScriptConfigList(AZStd::vector<SceneAPI::Events::ScriptConfig>& scriptConfigList) const = 0;

        //! Determines if the script config matches a create jobs request
        virtual AZStd::optional<SceneAPI::Events::ScriptConfig> MatchesScriptConfig(const AZStd::string& sourceFile) const = 0;
    };

    using ScriptConfigEventBus = AZ::EBus<ScriptConfigEvents>;
}
