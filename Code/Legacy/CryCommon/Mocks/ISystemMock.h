/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <ISystem.h>

#ifdef GetUserName
#undef GetUserName
#endif

class SystemMock
    : public ISystem
{
public:
    MOCK_METHOD0(Release,
        void());
    MOCK_METHOD0(GetGlobalEnvironment,
        SSystemGlobalEnvironment * ());
    MOCK_METHOD2(UpdatePreTickBus,
        bool(int, int));
    MOCK_METHOD2(UpdatePostTickBus,
        bool(int, int));
    MOCK_METHOD0(UpdateLoadtime,
        bool());
    MOCK_METHOD0(RenderStatistics,
        void());
    MOCK_METHOD0(GetUserName,
        const char*());
    MOCK_METHOD0(Quit,
        void());
    MOCK_METHOD1(Relaunch,
        void(bool bRelaunch));
    MOCK_CONST_METHOD0(IsQuitting,
        bool());
    MOCK_METHOD1(SerializingFile,
        void(int mode));
    MOCK_CONST_METHOD0(IsSerializingFile,
        int());
    MOCK_CONST_METHOD0(IsRelaunch,
        bool());

    void FatalError([[maybe_unused]] const char* sFormat, ...) override {}
    void ReportBug([[maybe_unused]] const char* sFormat, ...) override {}

    MOCK_METHOD6(WarningV,
        void(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args));

    void Warning([[maybe_unused]] EValidatorModule module, [[maybe_unused]] EValidatorSeverity severity, [[maybe_unused]] int flags, [[maybe_unused]] const char* file, [[maybe_unused]] const char* format, ...) override {}

    MOCK_METHOD3(ShowMessage,
        void(const char* text, const char* caption, unsigned int uType));
    MOCK_METHOD1(CheckLogVerbosity,
        bool(int verbosity));
    MOCK_METHOD0(GetILevelSystem,
        ILevelSystem * ());
    MOCK_METHOD0(GetICmdLine,
        ICmdLine * ());
    MOCK_METHOD0(GetILog,
        ILog * ());
    MOCK_METHOD0(GetIPak,
        AZ::IO::IArchive * ());
    MOCK_METHOD0(GetICryFont,
        ICryFont * ());
    MOCK_METHOD0(GetIMovieSystem,
        IMovieSystem * ());
    MOCK_METHOD0(GetIAudioSystem,
        Audio::IAudioSystem * ());
    MOCK_METHOD0(GetIConsole,
        ::IConsole * ());
    MOCK_METHOD0(GetIRemoteConsole,
        IRemoteConsole * ());
    MOCK_METHOD0(GetISystemEventDispatcher,
        ISystemEventDispatcher * ());
    MOCK_CONST_METHOD0(IsDevMode,
        bool());
    MOCK_METHOD3(CreateXmlNode,
        XmlNodeRef(const char*, bool, bool));
    MOCK_METHOD4(LoadXmlFromBuffer,
        XmlNodeRef(const char*, size_t, bool, bool));
    MOCK_METHOD2(LoadXmlFromFile,
        XmlNodeRef(const char*, bool));
    MOCK_METHOD0(GetXmlUtils,
        IXmlUtils * ());
    MOCK_METHOD1(IgnoreUpdates,
        void(bool bIgnore));
    MOCK_CONST_METHOD0(IsTestMode,
        bool());
    MOCK_METHOD3(SetFrameProfiler,
        void(bool on, bool display, char* prefix));
    MOCK_METHOD0(GetFileVersion,
        const SFileVersion&());
    MOCK_METHOD0(GetProductVersion,
        const SFileVersion&());
    MOCK_METHOD0(GetBuildVersion,
        const SFileVersion&());

    MOCK_METHOD1(AddCVarGroupDirectory,
        void(const AZStd::string&));
    MOCK_METHOD0(SaveConfiguration,
        void());
    MOCK_METHOD3(LoadConfiguration,
        void(const char*, ILoadConfigurationEntrySink*, bool));

    MOCK_CONST_METHOD0(GetConfigPlatform,
        ESystemConfigPlatform());
    MOCK_METHOD1(SetConfigPlatform,
        void(ESystemConfigPlatform platform));
    MOCK_CONST_METHOD0(IsPaused,
        bool());
    MOCK_METHOD0(GetLocalizationManager,
        ILocalizationManager * ());
    MOCK_METHOD0(GetNoiseGen,
        CPNoise3 * ());
    MOCK_METHOD1(RegisterErrorObserver,
        bool(IErrorObserver * errorObserver));
    MOCK_METHOD1(UnregisterErrorObserver,
        bool(IErrorObserver * errorObserver));
    MOCK_METHOD4(OnAssert,
        void(const char* condition, const char* message, const char* fileName, unsigned int fileLineNumber));
    MOCK_CONST_METHOD0(IsAssertDialogVisible,
        bool());
    MOCK_METHOD1(SetAssertVisible,
        void(bool bAssertVisble));
    MOCK_METHOD0(GetApplicationInstance,
        int());
    MOCK_METHOD1(GetApplicationLogInstance,
        int(const char* logFilePath));
    MOCK_METHOD0(ClearErrorMessages,
        void());
    MOCK_METHOD2(debug_GetCallStack,
        void(const char** pFunctions, int& nCount));
    MOCK_METHOD2(debug_LogCallStack,
        void(int, int));
    MOCK_METHOD1(ExecuteCommandLine,
        void(bool));
    MOCK_METHOD1(GetUpdateStats,
        void(SSystemUpdateStats & stats));
    MOCK_METHOD0(GetSystemGlobalState,
        ESystemGlobalState(void));
    MOCK_METHOD1(SetSystemGlobalState,
        void(ESystemGlobalState systemGlobalState));

#if !defined(_RELEASE)
    MOCK_CONST_METHOD0(IsSavingResourceList,
        bool());
#endif

    MOCK_METHOD1(RegisterWindowMessageHandler,
        void(IWindowMessageHandler * pHandler));
    MOCK_METHOD1(UnregisterWindowMessageHandler,
        void(IWindowMessageHandler * pHandler));

    MOCK_METHOD2(ForceMaxFps, void(bool, int));
};
