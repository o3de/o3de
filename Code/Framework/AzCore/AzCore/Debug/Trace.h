/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>
#include <AzCore/base.h>

namespace AZ
{
    namespace Debug
    {
        namespace Platform
        {
            void OutputToDebugger(const char* window, const char* message);
        }

        /// Global instance to the tracer.
        extern class Trace      g_tracer;

        enum LogLevel : int
        {
            Disabled = 0,
            Errors = 1,
            Warnings = 2,
            Info = 3
        };

        class Trace
        {
        public:
            static Trace& Instance()    { return g_tracer; }

            // Declare Trace init for assert tracking initializations, and get/setters for verbosity level
            static void Init();
            static void Destroy();
            static int GetAssertVerbosityLevel();
            static void SetAssertVerbosityLevel(int level);

            /**
            * Returns the default string used for a system window.
            * It can be useful for Trace message handlers to easily validate if the window they received is the fallback window used by this class,
            * or to force a Trace message bus handler to do special processing by using a known, consistent char*
            */
            static const char* GetDefaultSystemWindow();
            static bool IsDebuggerPresent();
            static bool AttachDebugger();
            static bool WaitForDebugger(float timeoutSeconds = -1.f);

            /// True or false if we want to handle system exceptions.
            static void HandleExceptions(bool isEnabled);

            /// Breaks program execution immediately.
            static void Break();

            /// Crash the application
            static void Crash();

            /// Terminates the process with the specified exit code
            static void Terminate(int exitCode);

            /// Indicates if trace logging functions are enabled based on compile mode and cvar logging level
            static bool IsTraceLoggingEnabledForLevel(LogLevel level);

            static void Assert(const char* fileName, int line, const char* funcName, const char* format, ...);
            static void Error(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...);
            static void Warning(const char* fileName, int line, const char* funcName, const char* window, const char* format, ...);
            static void Printf(const char* window, const char* format, ...);

            static void Output(const char* window, const char* message);

            static void PrintCallstack(const char* window, unsigned int suppressCount = 0, void* nativeContext = 0);

            /// PEXCEPTION_POINTERS on Windows, always NULL on other platforms
            static void* GetNativeExceptionInfo();
        };
    }
}

#ifdef AZ_ENABLE_TRACING

/**
* AZ tracing macros provide debug information reporting for assert, errors, warnings, and informational messages.
* The syntax allows printf style formatting for the message, e.g DBG_Error("System | MyWindow", true/false, "message %d", ...).
* Asserts are always sent to the "System" window, since they cannot be ignored.
*
* The four different types of macro to use, depending on the situation:
*    - Asserts should be used for critical errors, where the program cannot continue. They print the message together
*      with the file and line number. They then break program execution.
*    - Errors should be used where something is clearly wrong, but the program can continue safely. They print the message
*      together with the file and line number. Depending on platform they will notify the user that
*      an error has occurred, e.g. with a message box or an on-screen message.
*    - Warnings should be used when something could be wrong. They print the message together with the file and line number, and
*      take no other action.
*    - Printfs are purely informational. They print the message unadorned.
*    - Traces which have "Once" at the end will display the message only once for the life of the application instance.
*
* \note AZCore implements the AZStd::Default_Assert so if you want to customize it, use the Trace Listeners...
*/
/**
 * This is used to prevent incorrect usage if the user forgets to write the boolean expression to assert.
 * Example:
 *     AZ_Assert("Fail always"); // <- assertion will never fail
 * Correct usage:
 *     AZ_Assert(false, "Fail always");
 */

    namespace AZ
    {
        namespace TraceInternal
        {
            enum class ExpressionValidResult { Valid, Invalid_ConstCharPtr, Invalid_ConstCharArray };
            template<typename T>
            struct ExpressionIsValid
            {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Valid;
            };
            template<>
            struct ExpressionIsValid<const char*&>
            {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Valid;
            };
            template<unsigned int N>
            struct ExpressionIsValid<const char(&)[N]>
            {
                static constexpr ExpressionValidResult value = ExpressionValidResult::Invalid_ConstCharArray;
            };
        } // namespace TraceInternal
    } // namespace AZ
    #define AZ_TraceFmtCompileTimeCheck(expression, isVaArgs, baseMsg, msg, msgVargs)                                                                                                 \
    {                                                                                                                                                                                 \
        using namespace AZ::TraceInternal;                                                                                                                                            \
        [[maybe_unused]] const auto& rTraceFmtCompileTimeCheckExpressionHelper = (expression); /* This is needed for edge cases for expressions containing lambdas, that were unsupported before C++20 */   \
        constexpr ExpressionValidResult isValidTraceFmtResult = ExpressionIsValid<decltype(rTraceFmtCompileTimeCheckExpressionHelper)>::value;                                        \
        /* Assert different message depending whether it's const char array or if we have extra arguments */                                                                          \
        static_assert(!(isVaArgs) ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharArray : true, baseMsg " " msg);                                                    \
        static_assert(isVaArgs  ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharArray : true, baseMsg " " msgVargs);                                               \
        static_assert(!(isVaArgs) ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharPtr   : true, baseMsg " " msg " or "#expression" != nullptr");                     \
        static_assert(isVaArgs  ? isValidTraceFmtResult != ExpressionValidResult::Invalid_ConstCharPtr   : true, baseMsg " " msgVargs " or "#expression" != nullptr");                \
    }
    #define AZ_Assert(expression, ...)                                                                                  \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                           \
    if(!(expression))                                                                                                   \
    {                                                                                                                   \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                            \
        "String used in place of boolean expression for AZ_Assert.",                                                    \
        "Did you mean AZ_Assert(false, \"%s\", "#expression"); ?",                                                      \
        "Did you mean AZ_Assert(false, "#expression", "#__VA_ARGS__"); ?");                                             \
        AZ::Debug::Trace::Instance().Assert(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, __VA_ARGS__);                    \
    }                                                                                                                   \
    AZ_POP_DISABLE_WARNING

    #define AZ_Error(window, expression, ...)                                                                              \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                              \
    if (!(expression))                                                                                                     \
    {                                                                                                                      \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                               \
            "String used in place of boolean expression for AZ_Error.",                                                    \
            "Did you mean AZ_Error("#window", false, \"%s\", "#expression"); ?",                                           \
            "Did you mean AZ_Error("#window", false, "#expression", "#__VA_ARGS__"); ?");                                  \
        AZ::Debug::Trace::Instance().Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, __VA_ARGS__);                \
    }                                                                                                                      \
    AZ_POP_DISABLE_WARNING

    #define AZ_CONCAT_VAR_NAME(x, y) x ## y

    //! The AZ_ErrorOnce macro output the result of the format string only once for use of the macro itself.
    //! It does not take into account the result of the format string to determine whether to output the string or not
    //! What this means is that if the formatting results in different output string result only the first result
    //! will ever be output
    #define AZ_ErrorOnce(window, expression, ...)                                                                           \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                               \
    if (!(expression))                                                                                                      \
    {                                                                                                                       \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                                \
            "String used in place of boolean expression for AZ_ErrorOnce.",                                                 \
            "Did you mean AZ_ErrorOnce("#window", false, \"%s\", "#expression"); ?",                                        \
            "Did you mean AZ_ErrorOnce("#window", false, "#expression", "#__VA_ARGS__"); ?");                               \
        static bool AZ_CONCAT_VAR_NAME(azErrorDisplayed, __LINE__) = false;                                                                    \
        if (!AZ_CONCAT_VAR_NAME(azErrorDisplayed, __LINE__))                                                                                  \
        {                                                                                                                   \
            AZ_CONCAT_VAR_NAME(azErrorDisplayed, __LINE__) = true;                                                                            \
            AZ::Debug::Trace::Instance().Error(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, __VA_ARGS__);             \
        }                                                                                                                   \
    }                                                                                                                       \
    AZ_POP_DISABLE_WARNING

    #define AZ_Warning(window, expression, ...)                                                                             \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                               \
    if (!(expression))                                                                                                      \
    {                                                                                                                       \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                                \
            "String used in place of boolean expression for AZ_Warning.",                                                   \
            "Did you mean AZ_Warning("#window", false, \"%s\", "#expression"); ?",                                          \
            "Did you mean AZ_Warning("#window", false, "#expression", "#__VA_ARGS__"); ?");                                 \
        AZ::Debug::Trace::Instance().Warning(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, __VA_ARGS__);               \
    }                                                                                                                       \
    AZ_POP_DISABLE_WARNING

    //! The AZ_WarningOnce macro output the result of the format string for once each use of the macro
    //! It does not take into account the result of the format string to determine whether to output the string or not
    //! What this means is that if the formatting results in different output string result only the first result
    //! will ever be output
    #define AZ_WarningOnce(window, expression, ...)                                                                             \
    AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option")                                                                   \
    if (!(expression))                                                                                                          \
    {                                                                                                                           \
        AZ_TraceFmtCompileTimeCheck(expression, AZ_VA_HAS_ARGS(__VA_ARGS__),                                                    \
            "String used in place of boolean expression for AZ_WarningOnce.",                                                   \
            "Did you mean AZ_WarningOnce("#window", false, \"%s\", "#expression"); ?",                                          \
            "Did you mean AZ_WarningOnce("#window", false, "#expression", "#__VA_ARGS__"); ?");                                 \
        static bool AZ_CONCAT_VAR_NAME(azWarningDisplayed, __LINE__) = false;                                                                     \
        if (!AZ_CONCAT_VAR_NAME(azWarningDisplayed, __LINE__))                                                                                    \
        {                                                                                                                       \
            AZ::Debug::Trace::Instance().Warning(__FILE__, __LINE__, AZ_FUNCTION_SIGNATURE, window, __VA_ARGS__);               \
            AZ_CONCAT_VAR_NAME(azWarningDisplayed, __LINE__) = true;                                                                              \
        }                                                                                                                       \
    }                                                                                                                           \
    AZ_POP_DISABLE_WARNING

    #define AZ_TracePrintf(window, ...)                                                                            \
    if(AZ::Debug::Trace::IsTraceLoggingEnabledForLevel(AZ::Debug::LogLevel::Info))                                 \
    {                                                                                                              \
        AZ::Debug::Trace::Instance().Printf(window, __VA_ARGS__);                                                  \
    }


    //! The AZ_TrancePrintfOnce macro output the result of the format string only once for each use of the macro
    //! It does not take into account the result of the format string to determine whether to output the string or not
    //! What this means is that if the formatting results in different output string result only the first result
    //! will ever be output
    #define AZ_TracePrintfOnce(window, ...) \
    { \
        static bool AZ_CONCAT_VAR_NAME(azTracePrintfDisplayed, __LINE__) = false; \
        if (!AZ_CONCAT_VAR_NAME(azTracePrintfDisplayed, __LINE__)) \
        { \
            AZ_TracePrintf(window, __VA_ARGS__); \
            AZ_CONCAT_VAR_NAME(azTracePrintfDisplayed, __LINE__) = true; \
        } \
    }

/*!
 * Verify version of the trace checks evaluates the expression even in release.
 *
 * i.e.
 *   // with assert
 *   buffer = azmalloc(size,alignment);
 *   AZ_Assert( buffer != nullptr,"Assert Message");
 *   ...
 *
 *   // with verify
 *   AZ_Verify((buffer = azmalloc(size,alignment)) != nullptr,"Assert Message")
 *   ...
 */
    #define AZ_Verify(expression, ...) AZ_Assert(0 != (expression), __VA_ARGS__)
    #define AZ_VerifyError(window, expression, ...) AZ_Error(window, 0 != (expression), __VA_ARGS__)
    #define AZ_VerifyWarning(window, expression, ...) AZ_Warning(window, 0 != (expression), __VA_ARGS__)

#else // !AZ_ENABLE_TRACING

    #define AZ_Assert(...)
    #define AZ_Error(...)
    #define AZ_ErrorOnce(...)
    #define AZ_Warning(...)
    #define AZ_WarningOnce(...)
    #define AZ_TracePrintf(...)
    #define AZ_TracePrintfOnce(...)

    #define AZ_Verify(expression, ...)                  AZ_UNUSED(expression)
    #define AZ_VerifyError(window, expression, ...)     AZ_UNUSED(expression)
    #define AZ_VerifyWarning(window, expression, ...)   AZ_UNUSED(expression)

#endif  // AZ_ENABLE_TRACING

#define AZ_Printf(window, ...)       AZ::Debug::Trace::Instance().Printf(window, __VA_ARGS__);

#if !defined(RELEASE)
// Unconditional critical error log, enabled up to Performance config
#define AZ_Fatal(window, format, ...)       AZ::Debug::Trace::Instance().Printf(window, "[FATAL] " format "\n", ##__VA_ARGS__);
#define AZ_Crash()                          AZ::Debug::Trace::Instance().Crash();
#else
#define AZ_Fatal(window, format, ...)
#define AZ_Crash()
#endif

#if defined(AZ_DEBUG_BUILD)
#   define AZ_DbgIf(expression) \
        AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") \
        if (expression) \
        AZ_POP_DISABLE_WARNING
#   define AZ_DbgElseIf(expression) \
        AZ_PUSH_DISABLE_WARNING(4127, "-Wunknown-warning-option") \
        else if (expression) \
        AZ_POP_DISABLE_WARNING
#else
#   define AZ_DbgIf(expression)         if (false)
#   define AZ_DbgElseIf(expression)     else if (false)
#endif
