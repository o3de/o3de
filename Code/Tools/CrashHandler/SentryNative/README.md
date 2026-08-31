# sentry-native crash-reporting backend

This adds a second, compile-time-selectable backend for O3DE's external crash handler, built on the
official [sentry-native](https://github.com/getsentry/sentry-native) SDK, alongside the existing
Crashpad/Backtrace path. Selection is via a new PAL trait:

```cmake
ly_set(PAL_TRAIT_CRASH_HANDLER_BACKEND "Crashpad")    # existing behaviour (default)
ly_set(PAL_TRAIT_CRASH_HANDLER_BACKEND "SentryNative")
```

Only one backend is ever linked. Both expose the same
`CrashHandler::ToolsCrashHandler::InitCrashHandler(...)` entry point, so all existing callers (the
Editor, ProjectManager, LuaIDE, AssetProcessor, the Atom tools, ScriptCanvas) are unchanged.

## Layout

| Path | Target | Purpose |
|---|---|---|
| `SentryNative/Core` | `AZ::SentryCrashCore` | Option wiring, config, scope enrichment. Qt-free and not host-tools gated, so packaged launchers can link it. |
| `SentryNative/` | `AZ::ToolsCrashHandler` | Tools-side entry point; same API as the Crashpad backend's target of the same name. |
| `SentryNative/Reporter` | `SentryCrashReporter` | Optional post-crash Qt dialog, launched via sentry-native's `external_crash_reporter_path`. |

The vendored SDK is fetched by `cmake/3rdParty/sentry-native.cmake` (`FetchContent`, pinned tag),
which also stages `crashpad_handler` next to the binaries — without that, crash capture silently
does nothing.

## Configuration

All settings live under `/O3DE/CrashReporting/` in the SettingsRegistry, so a project configures
them from `<project>/Registry/crash_handler.setreg`. Every key is optional except the DSN; with no
DSN configured, crash reporting stays off and logs a warning rather than half-initializing.

```jsonc
{
    "O3DE": {
        "CrashReporting": {
            // Required. Get this from your Sentry project settings.
            "SentryDsn": "https://<publicKey>@<host>/<projectId>",

            "Release": "my-project@1.0.0",
            "Environment": "development",
            "Dist": "",

            "AttachScreenshot": true,
            "SymbolizeStacktraces": false,
            "EnableLargeAttachments": false,
            "MaxBreadcrumbs": 100,

            "SampleRate": 1.0,
            "ShutdownTimeoutMs": 2000,
            "WaitForUpload": false,
            "Proxy": "",
            "CaCerts": "",

            "RequireUserConsent": false,
            "SendDefaultPii": false,
            "ScrubUserPaths": true,

            "AutoSessionTracking": true,
            "EnableLogs": true,
            "EnableMetrics": false,
            "TracesSampleRate": 0.0,
            "EnableAppHangTracking": true,
            "AppHangTimeoutMs": 5000,

            "Debug": false,
            "AttemptLevelSaveOnCrash": true
        }
    }
}
```

Do not commit a real DSN to a shared repository — treat it as an endpoint credential.

## Packaged builds

Enabling the `CrashReporting` gem is sufficient: its system component initializes the SDK on
activation and calls `sentry_close()` on deactivation (which is what flushes queued envelopes and
marks the session as a clean exit). No launcher-side call is required.

The packaged path deliberately does not launch the Qt reporter dialog (packaged builds do not ship
it) and disables the editor-only level-save hook.

## Editor level recovery

When `AttemptLevelSaveOnCrash` is set, the Editor's `on_crash` hook performs a best-effort save via
the existing `CCryEditDoc::SaveAutoBackup(true)` and records the resulting file in a sidecar marker.
The reporter dialog then offers to reopen it on restart.

**This is deliberately unsafe by the strict standard.** sentry-native documents `on_crash` as
running inside a signal handler / `UnhandledExceptionFilter`, where only async-signal-safe work is
legal; a level save allocates and takes locks. It is a considered trade — losing unsaved work is
worse than a small chance of the save itself faulting, and the crash report is already captured by
that point. It is re-entrancy guarded and can never block the report. Set the key to `false` to
disable it.

## Known gaps

- **Symbolication**: no debug-file upload step, so frames appear as `module+offset` in Sentry until
  symbols are uploaded separately (e.g. via `sentry-cli`).
- **Performance tracing**: `TracesSampleRate` is plumbed but no spans are instrumented, so enabling
  it produces no span data on its own.
- **Session replay**: not wired.
- **Structured logging**: `EnableLogs` is plumbed, but engine `AZ_TracePrintf` output is not bridged
  to `sentry_log_*`; logs currently reach Sentry only as the attached log file.
- **PII scrubbing is targeted, not exhaustive**: sentry-native exposes no way to enumerate the keys
  of an object value, so `ScrubUserPaths` rewrites the places absolute paths actually surface
  (stack frames, known extras) rather than walking the whole event. `SendDefaultPii` is off by
  default and Sentry's server-side scrubbing covers the remainder.
