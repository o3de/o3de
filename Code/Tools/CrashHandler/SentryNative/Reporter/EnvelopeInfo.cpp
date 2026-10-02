/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <EnvelopeInfo.h>

#include <sentry.h>

#include <cstdio>
#include <cstdlib>

namespace CrashHandler
{
    namespace
    {
        constexpr const char* RecoveryEnvVar = "SENTRY_O3DE_RECOVERY_FILE";

        std::string GetString(sentry_value_t object, const char* key)
        {
            sentry_value_t entry = sentry_value_get_by_key(object, key);
            if (sentry_value_get_type(entry) != SENTRY_VALUE_TYPE_STRING)
            {
                return {};
            }
            const char* text = sentry_value_as_string(entry);
            return text ? std::string(text) : std::string();
        }
    }

    EnvelopeInfo ParseEnvelope(const std::string& envelopePath)
    {
        EnvelopeInfo info;

        // sentry-native owns the envelope format, so use its parser rather than re-deriving the
        // newline-delimited-JSON framing here - it stays correct as the format evolves.
        sentry_envelope_t* envelope = sentry_envelope_read_from_file(envelopePath.c_str());
        if (!envelope)
        {
            return info;
        }

        sentry_value_t event = sentry_envelope_get_event(envelope);
        if (!sentry_value_is_null(event))
        {
            info.eventId = GetString(event, "event_id");
            info.release = GetString(event, "release");
            info.environment = GetString(event, "environment");

            // project_path travels as a tag: it is set at init, where the value is known, rather
            // than from on_crash, which cannot enrich the event under the crashpad backend.
            info.projectPath = GetString(sentry_value_get_by_key(event, "tags"), "project_path");

            sentry_value_t os =
                sentry_value_get_by_key(sentry_value_get_by_key(event, "contexts"), "os");
            info.osName = GetString(os, "name");
            info.osVersion = GetString(os, "version");

            sentry_value_t exceptionValues =
                sentry_value_get_by_key(sentry_value_get_by_key(event, "exception"), "values");
            if (sentry_value_get_type(exceptionValues) == SENTRY_VALUE_TYPE_LIST
                && sentry_value_get_length(exceptionValues) > 0)
            {
                info.exceptionType =
                    GetString(sentry_value_get_by_index(exceptionValues, 0), "type");
            }
        }

        sentry_envelope_free(envelope);
        return info;
    }

    std::string ConsumeRecoveryMarker()
    {
        const char* markerPath = std::getenv(RecoveryEnvVar);
        if (!markerPath || !*markerPath)
        {
            return {};
        }

        std::string recoveredPath;
        if (FILE* file = std::fopen(markerPath, "rb"))
        {
            char buffer[1025]{};
            const size_t read = std::fread(buffer, 1, sizeof(buffer) - 1, file);
            std::fclose(file);
            recoveredPath.assign(buffer, read);
        }

        // Consumed on read so a single recovery is not offered again on later launches.
        std::remove(markerPath);
        return recoveredPath;
    }
}
