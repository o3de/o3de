/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the required headers
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include "StandardHeaders.h"
#include "MultiThreadManager.h"


namespace MCore
{
    // forward declaration
    class LogFile;
    class LogManager;

    /**
     * The log callback interface.
     * Classes inherited from the callback interface can be added to the log manager's stack
     * through which it iterates each time you log something. The Log() method of the callback
     * is called when logging. It can for example be used to write into a combo box in a MFC environment or into
     * an in-game console.
     */
    class MCORE_API LogCallback
    {
    public:
        /**
         * The importance of a logged message.
         */
        enum ELogLevel
        {
            LOGLEVEL_NONE           = 0,        /**< Use this log level to disable all logging for the callback. */
            LOGLEVEL_FATAL          = 1 << 0,   /**< Use this for errors that are very likely to crash the application. */
            LOGLEVEL_ERROR          = 1 << 1,   /**< Use this for errors that might result into something unexpected. */
            LOGLEVEL_WARNING        = 1 << 2,   /**< Use this for errors that won't hurt the stability, but might result in something visually incorrect for example. */
            LOGLEVEL_INFO           = 1 << 3,   /**< Use this for grouped, general information, e.g. when loading nodes only show how many nodes have been loaded without any additional information. */
            LOGLEVEL_DETAILEDINFO   = 1 << 4,   /**< Use this for detailed information, e.g. when loading nodes you can log the name, parent and all information stored inside the structure. */
            LOGLEVEL_DEBUG          = 1 << 5,   /**< Use this for debugging information, e.g. log vertex values which is very slow and not desired in a release built. */
            LOGLEVEL_ALL            = (LOGLEVEL_FATAL | LOGLEVEL_ERROR | LOGLEVEL_WARNING | LOGLEVEL_INFO | LOGLEVEL_DETAILEDINFO | LOGLEVEL_DEBUG),/**< Use this for enabling all kinds of log levels. */
            LOGLEVEL_DEFAULT        = (LOGLEVEL_FATAL | LOGLEVEL_ERROR | LOGLEVEL_WARNING | LOGLEVEL_INFO | LOGLEVEL_DETAILEDINFO)                  /**< Default log level. Displays everything except of debug logging. */
        };

        /**
         * Default constructor.
         */
        LogCallback();

        /**
         * Destructor.
         */
        virtual ~LogCallback(){}

        /**
         * The abstract callback logging method.
         * @param text The text to log.
         * @param logLevel The log message priority.
         */
        virtual void Log(const char* text, ELogLevel logLevel) = 0;

        /**
         * Get the unique log callback type ID.
         * @result The log callback type identification number.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Get the log levels this callback will accept and pass through.
         * To check if a log level is enabled use logical bitwise and comparison, example: if (GetLogLevels() & LOGLEVEL_EXAMPLE).
         * @result The log levels packed as bit flags which are enabled on the callback.
         */
        MCORE_INLINE ELogLevel GetLogLevels() const                             { return m_logLevels; }

        /**
         * Set the log levels this callback will accept and pass through.
         * To enable multiple log levels use the logical bitwise or operation, example: (LOGLEVEL_WARNING | LOGLEVEL_INFO).
         * The LogManager will automatically informed about the log level flags change by calling the LogManager::InitLogLevels() function.
         * @param logLevels The log levels packed as bit flags which are enabled on the callback.
         */
        void SetLogLevels(ELogLevel logLevels);

    protected:
        ELogLevel   m_logLevels; /**< The log levels that will pass the callback. All messages from log flags which are disabled won't be logged. The default value of the log level will be LOGLEVEL_DEFAULT. */
    };

    //----------------------------------------------------------------------------

    class MCORE_API AzLogCallback
        : public LogCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(AzLogCallback, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_LOGFILECALLBACK)

    public:
        // the type returned by GetType()
        enum
        {
            TYPE_ID = 0x0000001
        };

        /**
         * Get the unique log callback type ID.
         * @result The log callback type identification number.
         */
        uint32 GetType() const override { return TYPE_ID; }

        /**
         * The concrete callback logging method.
         * @param text The text to log.
         * @param logLevel The log message priority.
         */
        void Log(const char* text, ELogLevel logLevel) override;
    };

    //----------------------------------------------------------------------------

    /**
     * Logging macros that use the MCORE_HERE macro from SourceDeclaration.h.
     * Make sure that you are inside a class and that you have declared
     * the class via MCORE_DECLARE_CLASS, in addition you have to ensure that you
     * have declared your function. You will get a compile error if you do not
     * follow these instructions. If you are in global space or if you do not
     * want to declare your classes you can use the LOG function.
     */
    void MCORE_API LogFatalError(const char* what, ...);
    void MCORE_API LogError(const char* what, ...);
    void MCORE_API LogWarning(const char* what, ...);
    void MCORE_API LogInfo(const char* what, ...);
    void MCORE_API LogDetailedInfo(const char* what, ...);
    void MCORE_API LogDebug(const char* what, ...);
    void MCORE_API LogDebugMsg(const char* what);

    /**
     * Log a message to the console, visual studio output, or similar.
     * On Windows this will call a OutputDebugString and some other platforms a printf.
     * It is safe to use this method even if MCore has not been initialized or has already been shut down.
     * A line end using \n will automatically be added.
     * @param message The message to output. A line end \n will automatically be added.
     */
    void MCORE_API Print(const char* message);

    /**
     * Format a given std string like sprintf.
     * @param fmt The format string like "FloatValue = %f".
     * @result The formatted string.
     */
    AZStd::string MCORE_API FormatStdString(const char* fmt, ...);

    //----------------------------------------------------------------------------

    /**
     * The log manager handles the creation and retrieval of logs for the
     * application. This class is responsible for managing the log callbacks
     * and for logging. Each time you call the log function the manager will
     * iterate through all log callbacks and call their Log() functions so that you
     * can create and add your own log callbacks to the manager. This can be used to for
     * instance log into your in-game console or to some mfc combo box.
     */
    class MCORE_API LogManager
    {
        MCORE_MEMORYOBJECTCATEGORY(LogManager, MCORE_DEFAULT_ALIGNMENT, MCORE_MEMCATEGORY_LOGMANAGER)

    public:
        /**
         * Default constructor.
         */
        LogManager();

        /**
         * Destructor.
         */
        ~LogManager();

        /**
         * Add the given callback to the stack.
         * Don't delete the callbacks yourself, the manager will keep track of the callbacks in the stack.
         * @param callback The callback to add.
         */
        void AddLogCallback(LogCallback* callback);

        /**
         * Remove the given callback from the stack.
         * @param index The index of the callback to remove.
         */
        void RemoveLogCallback(size_t index);

        /**
         * Remove all given log callbacks by type from the stack.
         * @param type The type ID of the log callbacks to remove.
         */
        void RemoveAllByType(uint32 type);

        /**
         * Remove all log callbacks from the stack.
         */
        void ClearLogCallbacks();

        /**
         * Return a pointer to the given log callback.
         * @param index The index of the callback.
         * @return A pointer to the callback.
         */
        LogCallback* GetLogCallback(size_t index);

        /**
         * Find the index of a given callback.
         * @param callback The callback object to find.
         * @result Returns the index value, or MCORE_INVALIDINDEX32 when not found.
         */
        size_t FindLogCallback(LogCallback* callback) const;

        /**
         * Return the number of log callbacks managed by this class.
         * @return Number of log callbacks.
         */
        size_t GetNumLogCallbacks() const;

        /**
         * Force set the log levels of all callbacks in the log manager.
         * To enable multiple log levels use the logical bitwise or operation, example: (LOGLEVEL_WARNING | LOGLEVEL_INFO).
         * @param logLevels The log levels packed as bit flags which are enabled on the callbacks.
         */
        void SetLogLevels(LogCallback::ELogLevel logLevels);

        /**
         * Get the log levels any of the callbacks will accept and pass through.
         * To check if a log level is enabled by one of the callbacks use logical bitwise and comparison, example: if (GetLogLevels() & LOGLEVEL_EXAMPLE).
         * @result The log levels packed as bit flags which are enabled on the callback.
         */
        MCORE_INLINE LogCallback::ELogLevel GetLogLevels() const                                    { return m_logLevels; }

        /**
         * Iterate over all callbacks and collect the enabled log levels.
         * This will automatically be called by the LogCallback::SetLogLevels() function. Do not call this function manually.
         */
        void InitLogLevels();

        /**
         * Log a message.
         * This iterates through all callback instances and log to each of them.
         * @param message The message to write into the log file.
         * @param logLevel The log message priority.
         */
        void LogMessage(const char* message, LogCallback::ELogLevel logLevel = LogCallback::LOGLEVEL_INFO);

    public:
        static Mutex            s_globalMutex;       /**< The multithread mutex, used by some global Log functions. */

    private:
        AZStd::vector<LogCallback*>     m_logCallbacks;      /**< A collection of log callback instances. */
        LogCallback::ELogLevel  m_logLevels;         /**< The log levels that will pass one of the callbacks. All messages from log flags which are disabled won't be logged. */
        Mutex                   m_mutex;             /**< The mutex for logging locally. */
    };
} // namespace MCore
