/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <string>

namespace CrashHandler
{
    struct EnvelopeInfo
    {
        std::string eventId;
        std::string projectPath;
        std::string release;
        std::string environment;
        std::string osName;
        std::string osVersion;
        std::string exceptionType;
        //! Level file written by the emergency save during the crash, if one succeeded.
        //! Sourced from the sidecar marker rather than the envelope, because with the crashpad
        //! backend on_crash may filter but not enrich the event.
        std::string recoveredLevelPath;
    };

    //! Best-effort: never throws, and returns a partially- or fully-empty EnvelopeInfo when the
    //! envelope is missing or unreadable.
    EnvelopeInfo ParseEnvelope(const std::string& envelopePath);

    //! Reads (and deletes) the emergency-save marker named by the SENTRY_O3DE_RECOVERY_FILE
    //! environment variable, which the crashing process publishes to its children.
    std::string ConsumeRecoveryMarker();
}
