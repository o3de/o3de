/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <SentryCrashCore/SentryCrashCore.h>

#include <sentry.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzCore/std/string/conversions.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>

namespace SentryCrash
{
    namespace
    {
        constexpr const char* SettingsPrefix = "/O3DE/CrashReporting/";
        constexpr const char* RecoveryEnvVar = "SENTRY_O3DE_RECOVERY_FILE";

        std::atomic_bool s_initialized{ false };
        EmergencySaveCallback s_emergencySave;
        bool s_attemptLevelSave = false;
        bool s_scrubUserPaths = true;
        bool s_appHangTrackingEnabled = false;
        AZStd::string s_recoveryMarkerPath;
        AZStd::string s_userProfilePath;
        int s_crashedLastRun = -1;

        AZStd::string SettingsKey(const char* leaf)
        {
            return AZStd::string(SettingsPrefix) + leaf;
        }

        void ReadSetting(AZ::SettingsRegistryInterface* registry, const char* leaf, AZStd::string& out)
        {
            AZStd::string value;
            if (registry->Get(value, SettingsKey(leaf).c_str()) && !value.empty())
            {
                out = value;
            }
        }

        void ReadSetting(AZ::SettingsRegistryInterface* registry, const char* leaf, bool& out)
        {
            bool value{};
            if (registry->Get(value, SettingsKey(leaf).c_str()))
            {
                out = value;
            }
        }

        void ReadSetting(AZ::SettingsRegistryInterface* registry, const char* leaf, double& out)
        {
            double value{};
            if (registry->Get(value, SettingsKey(leaf).c_str()))
            {
                out = value;
            }
        }

        void ReadSetting(AZ::SettingsRegistryInterface* registry, const char* leaf, uint64_t& out)
        {
            AZ::s64 value{};
            if (registry->Get(value, SettingsKey(leaf).c_str()) && value >= 0)
            {
                out = static_cast<uint64_t>(value);
            }
        }

        void ReadSetting(AZ::SettingsRegistryInterface* registry, const char* leaf, size_t& out)
        {
            AZ::s64 value{};
            if (registry->Get(value, SettingsKey(leaf).c_str()) && value >= 0)
            {
                out = static_cast<size_t>(value);
            }
        }

        AZStd::string GetUserProfilePath()
        {
#if defined(AZ_PLATFORM_WINDOWS)
            constexpr const char* profile = "USERPROFILE";
#else
            const char* profile = "HOME";
#endif
            if (auto outcome = AZ::Utils::GetEnv(AZStd::span(buffer, AZ_ARRAY_SIZE(buffer)), profileVar); outcome.IsSuccess())
            {
                return AZStd::string(outcome.GetValue());
            }
            return {};
        }

        //! Rewrites the absolute user-profile prefix out of a single string value in place.
        void ScrubValueAt(sentry_value_t owner, const char* key)
        {
            if (s_userProfilePath.empty())
            {
                return;
            }
            sentry_value_t entry = sentry_value_get_by_key(owner, key);
            if (sentry_value_get_type(entry) != SENTRY_VALUE_TYPE_STRING)
            {
                return;
            }
            const char* text = sentry_value_as_string(entry);
            if (!text)
            {
                return;
            }
            AZStd::string scrubbed(text);
            if (scrubbed.find(s_userProfilePath) == AZStd::string::npos)
            {
                return;
            }
            AZ::StringFunc::Replace(scrubbed, s_userProfilePath.c_str(), "<user>");
            sentry_value_set_by_key(owner, key, sentry_value_new_string(scrubbed.c_str()));
        }

        void ScrubStacktrace(sentry_value_t stacktrace)
        {
            sentry_value_t frames = sentry_value_get_by_key(stacktrace, "frames");
            if (sentry_value_get_type(frames) != SENTRY_VALUE_TYPE_LIST)
            {
                return;
            }
            const size_t frameCount = sentry_value_get_length(frames);
            for (size_t i = 0; i < frameCount; ++i)
            {
                sentry_value_t frame = sentry_value_get_by_index(frames, i);
                ScrubValueAt(frame, "abs_path");
                ScrubValueAt(frame, "filename");
                ScrubValueAt(frame, "package");
            }
        }

        //! Targeted, not exhaustive: sentry-native exposes no way to enumerate the keys of an
        //! object value, so a generic recursive walk is impossible through the public API. We scrub
        //! the places absolute paths actually surface - stack frames and the extras we set
        //! ourselves - and rely on send_default_pii=false plus Sentry's server-side scrubbing for
        //! everything else.
        sentry_value_t ScrubEvent(sentry_value_t event, void*, void*)
        {
            if (!s_scrubUserPaths)
            {
                return event;
            }

            for (const char* container : { "exception", "threads" })
            {
                sentry_value_t values = sentry_value_get_by_key(sentry_value_get_by_key(event, container), "values");
                if (sentry_value_get_type(values) != SENTRY_VALUE_TYPE_LIST)
                {
                    continue;
                }
                const size_t count = sentry_value_get_length(values);
                for (size_t i = 0; i < count; ++i)
                {
                    ScrubStacktrace(sentry_value_get_by_key(sentry_value_get_by_index(values, i), "stacktrace"));
                }
            }

            sentry_value_t extra = sentry_value_get_by_key(event, "extra");
            if (sentry_value_get_type(extra) == SENTRY_VALUE_TYPE_OBJECT)
            {
                for (const char* key : { "project_path", "recovered_level_path", "log_path" })
                {
                    ScrubValueAt(extra, key);
                }
            }

            return event;
        }

        void ForwardSentryLog(sentry_level_t level, const char* message, va_list args, void*)
        {
            char buffer[1024];
            azvsnprintf(buffer, sizeof(buffer), message, args);
            if (level >= SENTRY_LEVEL_ERROR)
            {
                AZ_Warning("CrashReporting", false, "sentry: %s", buffer);
            }
            else
            {
                AZ_TracePrintf("CrashReporting", "sentry: %s\n", buffer);
            }
        }

        void WriteRecoveryMarker(const char* savedLevelPath)
        {
            if (s_recoveryMarkerPath.empty())
            {
                return;
            }
            
            FILE* file = nullptr;
            azfopen(&file, s_recoveryMarkerPath.c_str(), "wb");
            if (file)
            {
                std::fputs(savedLevelPath, file);
                std::fclose(file);
            }
        }

        sentry_value_t OnCrash(const sentry_ucontext_t*, sentry_value_t event, void*)
        {
            // Re-entrancy guard: a fault raised *by* the emergency save would otherwise recurse
            // straight back into here and never let the crash report through.
            static std::atomic_bool crashHandled{ false };
            bool expected = false;
            if (!crashHandled.compare_exchange_strong(expected, true))
            {
                return event;
            }

            if (s_attemptLevelSave && s_emergencySave)
            {
                const AZStd::string savedPath = s_emergencySave();
                if (!savedPath.empty())
                {
                    WriteRecoveryMarker(savedPath.c_str());
                }
            }

            // Returning the event unmodified: with the crashpad backend on_crash can filter but not
            // enrich, so the recovery path travels via the sidecar marker instead of the event.
            return event;
        }

        void OnCrashedLastRun(void*)
        {
            s_crashedLastRun = 1;
        }
    } // namespace

    Config LoadConfig(AZStd::string_view moduleTag, AZStd::string_view appRoot)
    {
        Config config;
        config.m_moduleTag = moduleTag;

        AZStd::string root{ appRoot };
        if (!root.empty() && root.back() != '/' && root.back() != '\\')
        {
            root += '/';
        }
        config.m_databasePath = root + ".sentry-native";

        if (auto* registry = AZ::SettingsRegistry::Get(); registry != nullptr)
        {
            ReadSetting(registry, "SentryDsn", config.m_dsn);
            ReadSetting(registry, "Release", config.m_release);
            ReadSetting(registry, "Environment", config.m_environment);
            ReadSetting(registry, "Dist", config.m_dist);
            ReadSetting(registry, "Proxy", config.m_proxy);
            ReadSetting(registry, "CaCerts", config.m_caCerts);

            ReadSetting(registry, "AttachScreenshot", config.m_attachScreenshot);
            ReadSetting(registry, "SymbolizeStacktraces", config.m_symbolizeStacktraces);
            ReadSetting(registry, "EnableLargeAttachments", config.m_enableLargeAttachments);
            ReadSetting(registry, "MaxBreadcrumbs", config.m_maxBreadcrumbs);

            ReadSetting(registry, "SampleRate", config.m_sampleRate);
            ReadSetting(registry, "ShutdownTimeoutMs", config.m_shutdownTimeoutMs);
            ReadSetting(registry, "WaitForUpload", config.m_waitForUpload);

            ReadSetting(registry, "RequireUserConsent", config.m_requireUserConsent);
            ReadSetting(registry, "SendDefaultPii", config.m_sendDefaultPii);
            ReadSetting(registry, "ScrubUserPaths", config.m_scrubUserPaths);

            ReadSetting(registry, "AutoSessionTracking", config.m_autoSessionTracking);
            ReadSetting(registry, "EnableLogs", config.m_enableLogs);
            ReadSetting(registry, "EnableMetrics", config.m_enableMetrics);
            ReadSetting(registry, "TracesSampleRate", config.m_tracesSampleRate);
            ReadSetting(registry, "EnableAppHangTracking", config.m_enableAppHangTracking);
            ReadSetting(registry, "AppHangTimeoutMs", config.m_appHangTimeoutMs);

            ReadSetting(registry, "Debug", config.m_debug);
            ReadSetting(registry, "AttemptLevelSaveOnCrash", config.m_attemptLevelSaveOnCrash);
        }

        return config;
    }

    bool Initialize(const Config& config)
    {
        if (s_initialized)
        {
            AZ_Warning("CrashReporting", false, "Sentry crash reporting is already initialized");
            return true;
        }

        if (config.m_dsn.empty())
        {
            AZ_Warning("CrashReporting", false, "No Sentry DSN configured (%sSentryDsn) - crash reporting is disabled", SettingsPrefix);
            return false;
        }

        s_attemptLevelSave = config.m_attemptLevelSaveOnCrash;
        s_scrubUserPaths = config.m_scrubUserPaths;
        s_appHangTrackingEnabled = config.m_enableAppHangTracking;
        s_userProfilePath = GetUserProfilePath();
        s_recoveryMarkerPath = config.m_databasePath + "/last_crash_recovery.txt";

        sentry_options_t* options = sentry_options_new();

        sentry_options_set_dsn(options, config.m_dsn.c_str());
        sentry_options_set_database_path(options, config.m_databasePath.c_str());
        sentry_options_set_environment(options, config.m_environment.c_str());
        if (!config.m_release.empty())
        {
            sentry_options_set_release(options, config.m_release.c_str());
        }
        if (!config.m_dist.empty())
        {
            sentry_options_set_dist(options, config.m_dist.c_str());
        }
        if (!config.m_handlerPath.empty())
        {
            sentry_options_set_handler_path(options, config.m_handlerPath.c_str());
        }
        if (!config.m_externalReporterPath.empty())
        {
            sentry_options_set_external_crash_reporter_path(options, config.m_externalReporterPath.c_str());
        }
        if (!config.m_proxy.empty())
        {
            sentry_options_set_proxy(options, config.m_proxy.c_str());
        }
        if (!config.m_caCerts.empty())
        {
            sentry_options_set_ca_certs(options, config.m_caCerts.c_str());
        }

        for (const AZStd::string& attachment : config.m_attachments)
        {
            sentry_options_add_attachment(options, attachment.c_str());
        }

        sentry_options_set_attach_screenshot(options, config.m_attachScreenshot ? 1 : 0);
        sentry_options_set_symbolize_stacktraces(options, config.m_symbolizeStacktraces ? 1 : 0);
        sentry_options_set_enable_large_attachments(options, config.m_enableLargeAttachments ? 1 : 0);
        sentry_options_set_max_breadcrumbs(options, config.m_maxBreadcrumbs);

        sentry_options_set_sample_rate(options, config.m_sampleRate);
        sentry_options_set_shutdown_timeout(options, config.m_shutdownTimeoutMs);
        sentry_options_set_crashpad_wait_for_upload(options, config.m_waitForUpload ? 1 : 0);

        sentry_options_set_require_user_consent(options, config.m_requireUserConsent ? 1 : 0);
        sentry_options_set_send_default_pii(options, config.m_sendDefaultPii ? 1 : 0);
        sentry_options_set_before_send(options, ScrubEvent, nullptr);

        sentry_options_set_auto_session_tracking(options, config.m_autoSessionTracking ? 1 : 0);
        sentry_options_set_enable_logs(options, config.m_enableLogs ? 1 : 0);
        sentry_options_set_enable_metrics(options, config.m_enableMetrics ? 1 : 0);
        sentry_options_set_traces_sample_rate(options, config.m_tracesSampleRate);
        sentry_options_set_enable_app_hang_tracking(options, config.m_enableAppHangTracking ? 1 : 0);
        sentry_options_set_app_hang_timeout(options, config.m_appHangTimeoutMs);

        sentry_options_set_on_crash(options, OnCrash, nullptr);
        sentry_options_set_on_crashed_last_run(options, OnCrashedLastRun, nullptr);

        if (config.m_debug)
        {
            sentry_options_set_debug(options, 1);
            sentry_options_set_logger(options, ForwardSentryLog, nullptr);
        }

        // Published so the companion reporter process - launched by crashpad, inheriting this
        // environment - can find the marker without re-deriving the database path.
        AZStd::string recoveryEnv = s_recoveryMarkerPath;
#if defined(AZ_PLATFORM_WINDOWS)
        _putenv_s(RecoveryEnvVar, recoveryEnv.c_str());
        _putenv_s("SENTRY_DSN", config.m_dsn.c_str());
#else
        setenv(RecoveryEnvVar, recoveryEnv.c_str(), 1);
        setenv("SENTRY_DSN", config.m_dsn.c_str(), 1);
#endif

        if (sentry_init(options) != 0)
        {
            AZ_Warning("CrashReporting", false, "sentry_init failed - crash reporting is disabled");
            return false;
        }

        s_initialized = true;
        s_crashedLastRun = sentry_get_crashed_last_run();

        sentry_set_tag("module", config.m_moduleTag.c_str());

        AZ_TracePrintf(
            "CrashReporting",
            "Sentry crash reporting initialized for %s (environment=%s, database=%s)\n",
            config.m_moduleTag.c_str(),
            config.m_environment.c_str(),
            config.m_databasePath.c_str());
        return true;
    }

    void Shutdown()
    {
        if (!s_initialized.exchange(false))
        {
            return;
        }
        // Ends the session as a clean exit and flushes queued envelopes; without this, events
        // captured late in the run are silently dropped.
        sentry_close();
    }

    bool IsInitialized()
    {
        return s_initialized;
    }

    void SetEmergencySaveCallback(EmergencySaveCallback callback)
    {
        s_emergencySave = AZStd::move(callback);
    }

    AZStd::string GetRecoveryMarkerPath()
    {
        const char* fromEnv = std::getenv(RecoveryEnvVar);
        return fromEnv ? AZStd::string(fromEnv) : s_recoveryMarkerPath;
    }

    bool CrashedLastRun()
    {
        return s_crashedLastRun == 1;
    }

    void SetTag(const char* key, const char* value)
    {
        if (s_initialized && key && value)
        {
            sentry_set_tag(key, value);
        }
    }

    void AddBreadcrumb(const char* type, const char* category, const char* message)
    {
        if (!s_initialized || !message)
        {
            return;
        }
        sentry_value_t crumb = sentry_value_new_breadcrumb(type, message);
        if (category)
        {
            sentry_value_set_by_key(crumb, "category", sentry_value_new_string(category));
        }
        sentry_add_breadcrumb(crumb);
    }

    void SetContext(const char* key, const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& values)
    {
        if (!s_initialized || !key)
        {
            return;
        }
        sentry_value_t context = sentry_value_new_object();
        for (const auto& [name, value] : values)
        {
            sentry_value_set_by_key(context, name.c_str(), sentry_value_new_string(value.c_str()));
        }
        sentry_set_context(key, context);
    }

    void SetLevelName(const char* levelName)
    {
        SetTag("level", levelName ? levelName : "<none>");
    }

    void AppHangHeartbeat()
    {
        if (s_initialized && s_appHangTrackingEnabled)
        {
            sentry_app_hang_heartbeat();
        }
    }
} // namespace SentryCrash
