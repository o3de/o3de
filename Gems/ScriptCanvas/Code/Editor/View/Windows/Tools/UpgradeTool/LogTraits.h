/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

#define VE_LOG(...) LogBus::Broadcast(&LogTraits::Entry, __VA_ARGS__);

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class LogTraits
            : public AZ::EBusTraits
        {
        public:
            virtual void Activate() = 0;
            virtual void Clear() = 0;
            virtual void Deactivate() = 0;
            virtual void Entry(const char* format, ...) = 0;
            virtual const AZStd::vector<AZStd::string>* GetEntries() const = 0;
            virtual void SetVersionExporerExclusivity(bool enabled) = 0;
            virtual void SetVerbose(bool verbosity) = 0;
        };
        using LogBus = AZ::EBus<LogTraits>;
    }
}
