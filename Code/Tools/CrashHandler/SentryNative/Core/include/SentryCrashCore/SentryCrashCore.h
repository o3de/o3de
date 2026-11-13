/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/string.h>

//! Backend-agnostic sentry-native wiring shared by every crash-reporting host in the engine:
//! the Editor and the other Qt tools (via AZ::ToolsCrashHandler) and packaged game/server
//! launchers (via the CrashReporting gem's system component). Deliberately free of Qt and of any
//! host-tools-only dependency so the runtime side can link it.
namespace SentryCrash
{
    //! Mirrors the sentry-native option surface we expose. Populated from the SettingsRegistry by
    //! LoadConfig(); every field has a working default so a project only overrides what it cares
    //! about. Key names are documented on LoadConfig().
    struct Config
    {
        // --- Identity -------------------------------------------------------------------------
        AZStd::string m_dsn;
        AZStd::string m_release;
        AZStd::string m_environment{ "development" };
        AZStd::string m_dist;
        //! Free-form label for this host, e.g. "Editor", "MaterialEditor", "GameLauncher".
        AZStd::string m_moduleTag;

        // --- Paths ----------------------------------------------------------------------------
        AZStd::string m_databasePath;
        //! crashpad_handler executable. Empty lets sentry-native look next to the running exe.
        AZStd::string m_handlerPath;
        //! Companion UI process launched after a crash. Empty disables it (packaged builds).
        AZStd::string m_externalReporterPath;
        AZStd::vector<AZStd::string> m_attachments;

        // --- Capture --------------------------------------------------------------------------
        bool m_attachScreenshot{ true };
        bool m_symbolizeStacktraces{ false };
        bool m_enableLargeAttachments{ false };
        size_t m_maxBreadcrumbs{ 100 };

        // --- Delivery -------------------------------------------------------------------------
        double m_sampleRate{ 1.0 };
        uint64_t m_shutdownTimeoutMs{ 2000 };
        bool m_waitForUpload{ false };
        AZStd::string m_proxy;
        AZStd::string m_caCerts;

        // --- Privacy --------------------------------------------------------------------------
        bool m_requireUserConsent{ false };
        bool m_sendDefaultPii{ false };
        //! Rewrites absolute user-profile paths out of outgoing events (before_send). Independent
        //! of m_sendDefaultPii, which only governs what sentry-native itself collects.
        bool m_scrubUserPaths{ true };

        // --- Telemetry ------------------------------------------------------------------------
        bool m_autoSessionTracking{ true };
        bool m_enableLogs{ true };
        bool m_enableMetrics{ false };
        double m_tracesSampleRate{ 0.0 };
        bool m_enableAppHangTracking{ false };
        uint64_t m_appHangTimeoutMs{ 5000 };

        // --- Diagnostics ----------------------------------------------------------------------
        //! Routes sentry-native's own logging into AZ_TracePrintf/AZ_Warning.
        bool m_debug{ false };

        // --- Editor-only ----------------------------------------------------------------------
        //! Best-effort level save from inside the crash handler. See SetEmergencySaveCallback().
        bool m_attemptLevelSaveOnCrash{ true };
    };

    //! Builds a Config from the SettingsRegistry, under /O3DE/CrashReporting/. Recognised keys
    //! match the Config field names without the m_ prefix, capitalised: SentryDsn, Release,
    //! Environment, Dist, AttachScreenshot, SymbolizeStacktraces, EnableLargeAttachments,
    //! MaxBreadcrumbs, SampleRate, ShutdownTimeoutMs, WaitForUpload, Proxy, CaCerts,
    //! RequireUserConsent, SendDefaultPii, ScrubUserPaths, AutoSessionTracking, EnableLogs,
    //! EnableMetrics, TracesSampleRate, EnableAppHangTracking, AppHangTimeoutMs, Debug,
    //! AttemptLevelSaveOnCrash. Paths and moduleTag are derived from the arguments.
    //! appRoot is where the crash database lives (typically the project's user folder).
    Config LoadConfig(AZStd::string_view moduleTag, AZStd::string_view appRoot);

    //! Starts sentry-native. Returns false (and reports a warning) if the DSN is empty or
    //! sentry_init fails. Safe to call once per process; a second call is ignored.
    bool Initialize(const Config& config);

    //! Flushes pending events and shuts sentry-native down. Must be called on clean exit or
    //! queued envelopes (and the session's "exited" status) are never delivered.
    void Shutdown();

    bool IsInitialized();

    //! Returns the saved file's path, or an empty string if nothing was saved.
    using EmergencySaveCallback = AZStd::function<AZStd::string()>;

    //! Registers a best-effort save invoked from inside the crash handler when
    //! Config::m_attemptLevelSaveOnCrash is set.
    //!
    //! DANGER: sentry-native documents on_crash as running inside a signal handler (POSIX) or an
    //! UnhandledExceptionFilter (Windows), where only async-signal-safe work is officially legal.
    //! A level save allocates, takes locks and touches the asset system, so it is emphatically not
    //! safe by that standard - it is a deliberate best-effort trade (the same one Unreal's crash
    //! handler makes) to avoid losing a user's unsaved work. It is re-entrancy guarded and its
    //! failure can never block the crash report itself, but it can hang or fault; that is the
    //! accepted cost. Never move required reporting work into this callback.
    void SetEmergencySaveCallback(EmergencySaveCallback callback);

    //! Path of the file the emergency save produced during the *previous* run's crash, or empty.
    //! Read from the sidecar marker the crash handler writes, whose location is published to
    //! child processes via the SENTRY_O3DE_RECOVERY_FILE environment variable.
    AZStd::string GetRecoveryMarkerPath();

    //! True when the previous run of this application ended in a crash.
    bool CrashedLastRun();

    // --- Scope enrichment. No-ops before Initialize() succeeds. ---------------------------------

    void SetTag(const char* key, const char* value);
    void AddBreadcrumb(const char* type, const char* category, const char* message);

    //! Adds a named context object, e.g. "gpu" -> { "name": ..., "driver_version": ... }.
    void SetContext(const char* key, const AZStd::vector<AZStd::pair<AZStd::string, AZStd::string>>& values);

    //! Records the level/scene currently open, so crashes can be grouped by content.
    void SetLevelName(const char* levelName);

    //! Beat from the thread whose responsiveness matters (the main/UI thread). No-op unless
    //! Config::m_enableAppHangTracking is set. The first caller becomes the watched thread.
    void AppHangHeartbeat();
}
