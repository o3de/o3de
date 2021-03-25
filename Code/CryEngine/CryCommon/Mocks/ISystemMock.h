/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once
#include <ISystem.h>
#include <Cry_Camera.h>

#ifdef GetUserName
#undef GetUserName
#endif

class SystemMock
    : public ISystem
{
public:
    MOCK_METHOD0(Release,
        void());
    MOCK_CONST_METHOD0(GetCVarsWhiteListConfigSink,
        ILoadConfigurationEntrySink * ());
    MOCK_METHOD0(GetGlobalEnvironment,
        SSystemGlobalEnvironment * ());
    MOCK_METHOD2(UpdatePreTickBus,
        bool(int, int));
    MOCK_METHOD2(UpdatePostTickBus,
        bool(int, int));
    MOCK_METHOD0(UpdateLoadtime,
        bool());
    MOCK_METHOD0(DoWorkDuringOcclusionChecks,
        void());
    MOCK_METHOD0(NeedDoWorkDuringOcclusionChecks,
        bool());
    MOCK_METHOD0(Render,
        void());
    MOCK_METHOD0(RenderBegin,
        void());
    MOCK_METHOD2(RenderEnd,
        void(bool, bool));
    MOCK_METHOD2(SynchronousLoadingTick,
        void(const char* pFunc, int line));
    MOCK_METHOD0(RenderStatistics,
        void());
    MOCK_METHOD0(GetUsedMemory,
        uint32());
    MOCK_METHOD0(GetUserName,
        const char*());
    MOCK_METHOD0(GetCPUFlags,
        int());
    MOCK_METHOD0(GetLogicalCPUCount,
        int());
    MOCK_CONST_METHOD0(GetAssetsPlatform,
        const char*());
    MOCK_CONST_METHOD0(GetRenderingDriverName,
        const char*());
    MOCK_METHOD1(DumpMemoryUsageStatistics,
        void(bool));
    MOCK_METHOD0(Quit,
        void());
    MOCK_METHOD1(Relaunch,
        void(bool bRelaunch));
    MOCK_CONST_METHOD0(IsQuitting,
        bool());
    MOCK_CONST_METHOD0(IsShaderCacheGenMode,
        bool());
    MOCK_METHOD1(SerializingFile,
        void(int mode));
    MOCK_CONST_METHOD0(IsSerializingFile,
        int());
    MOCK_CONST_METHOD0(IsRelaunch,
        bool());
    MOCK_METHOD4(DisplayErrorMessage,
        void(const char*, float, const float*, bool));

    void FatalError([[maybe_unused]] const char* sFormat, ...) override {}
    void ReportBug([[maybe_unused]] const char* sFormat, ...) override {}

    MOCK_METHOD6(WarningV,
        void(EValidatorModule module, EValidatorSeverity severity, int flags, const char* file, const char* format, va_list args));

    void Warning([[maybe_unused]] EValidatorModule module, [[maybe_unused]] EValidatorSeverity severity, [[maybe_unused]] int flags, [[maybe_unused]] const char* file, [[maybe_unused]] const char* format, ...) override {}

    MOCK_METHOD3(ShowMessage,
        int(const char* text, const char* caption, unsigned int uType));
    MOCK_METHOD1(CheckLogVerbosity,
        bool(int verbosity));
    MOCK_METHOD0(GetIZLibCompressor,
        IZLibCompressor * ());
    MOCK_METHOD0(GetIZLibDecompressor,
        IZLibDecompressor * ());
    MOCK_METHOD0(GetLZ4Decompressor,
        ILZ4Decompressor * ());
    MOCK_METHOD0(GetZStdDecompressor,
        IZStdDecompressor * ());
    MOCK_METHOD0(GetPerfHUD,
        ICryPerfHUD * ());
    MOCK_METHOD0(GetINotificationNetwork,
        INotificationNetwork * ());
    MOCK_METHOD0(GetIViewSystem,
        IViewSystem * ());
    MOCK_METHOD0(GetILevelSystem,
        ILevelSystem * ());
    MOCK_METHOD0(GetINameTable,
        INameTable * ());
    MOCK_METHOD0(GetIValidator,
        IValidator * ());
    MOCK_METHOD0(GetStreamEngine,
        IStreamEngine * ());
    MOCK_METHOD0(GetICmdLine,
        ICmdLine * ());
    MOCK_METHOD0(GetILog,
        ILog * ());
    MOCK_METHOD0(GetIPak,
        AZ::IO::IArchive * ());
    MOCK_METHOD0(GetICryFont,
        ICryFont * ());
    MOCK_METHOD0(GetIMemoryManager,
        IMemoryManager * ());
    MOCK_METHOD0(GetIMovieSystem,
        IMovieSystem * ());
    MOCK_METHOD0(GetIAudioSystem,
        Audio::IAudioSystem * ());
    MOCK_METHOD0(GetI3DEngine,
        I3DEngine * ());
    MOCK_METHOD0(GetIConsole,
        ::IConsole * ());
    MOCK_METHOD0(GetIRemoteConsole,
        IRemoteConsole * ());
    MOCK_METHOD0(GetIResourceManager,
        IResourceManager * ());
    MOCK_METHOD0(GetIThreadTaskManager,
        IThreadTaskManager * ());
    MOCK_METHOD0(GetIProfilingSystem,
        IProfilingSystem * ());
    MOCK_METHOD0(GetISystemEventDispatcher,
        ISystemEventDispatcher * ());
    MOCK_METHOD0(GetIVisualLog,
        IVisualLog * ());
    MOCK_METHOD0(GetIFileChangeMonitor,
        IFileChangeMonitor * ());
    MOCK_METHOD0(GetHWND,
        WIN_HWND());
    MOCK_METHOD0(GetIRenderer,
        IRenderer * ());
    MOCK_METHOD0(GetITimer,
        ITimer * ());
    MOCK_METHOD0(GetIThreadManager,
        IThreadManager * ());
    MOCK_METHOD1(SetLoadingProgressListener,
        void(ILoadingProgressListener * pListener));
    MOCK_CONST_METHOD0(GetLoadingProgressListener,
        ISystem::ILoadingProgressListener * ());
    MOCK_METHOD1(SetIMaterialEffects,
        void(IMaterialEffects * pMaterialEffects));
    MOCK_METHOD1(SetIOpticsManager,
        void(IOpticsManager * pOpticsManager));
    MOCK_METHOD1(SetIFileChangeMonitor,
        void(IFileChangeMonitor * pFileChangeMonitor));
    MOCK_METHOD1(SetIVisualLog,
        void(IVisualLog * pVisualLog));
    MOCK_METHOD2(DebugStats,
        void(bool checkpoint, bool leaks));
    MOCK_METHOD0(DumpWinHeaps,
        void());
    MOCK_METHOD1(DumpMMStats,
        int(bool log));
    MOCK_METHOD1(SetForceNonDevMode,
        void(bool bValue));
    MOCK_CONST_METHOD0(GetForceNonDevMode,
        bool());
    MOCK_CONST_METHOD0(WasInDevMode,
        bool());
    MOCK_CONST_METHOD0(IsDevMode,
        bool());
    MOCK_CONST_METHOD1(IsMODValid,
        bool(const char* szMODName));
    MOCK_CONST_METHOD0(IsMinimalMode,
        bool());
    MOCK_METHOD3(CreateXmlNode,
        XmlNodeRef(const char*, bool, bool));
    MOCK_METHOD4(LoadXmlFromBuffer,
        XmlNodeRef(const char*, size_t, bool, bool));
    MOCK_METHOD2(LoadXmlFromFile,
        XmlNodeRef(const char*, bool));
    MOCK_METHOD0(GetXmlUtils,
        IXmlUtils * ());
    MOCK_CONST_METHOD0(GetArchiveHost,
        Serialization::IArchiveHost * ());
    MOCK_METHOD1(SetViewCamera,
        void(CCamera & Camera));
    MOCK_METHOD0(GetViewCamera,
        CCamera & ());
    MOCK_METHOD1(IgnoreUpdates,
        void(bool bIgnore));
    MOCK_METHOD1(SetIProcess,
        void(IProcess * process));
    MOCK_METHOD0(GetIProcess,
        IProcess * ());
    MOCK_CONST_METHOD0(IsTestMode,
        bool());
    MOCK_METHOD3(SetFrameProfiler,
        void(bool on, bool display, char* prefix));
    MOCK_METHOD2(StartLoadingSectionProfiling,
        struct SLoadingTimeContainer*(CLoadingTimeProfiler * pProfiler, const char* szFuncName));
    MOCK_METHOD1(EndLoadingSectionProfiling,
        void(CLoadingTimeProfiler * pProfiler));
    MOCK_METHOD2(StartBootSectionProfiler,
        CBootProfilerRecord * (const char* name, const char* args));
    MOCK_METHOD1(StopBootSectionProfiler,
        void(CBootProfilerRecord * record));
    MOCK_METHOD1(StartBootProfilerSessionFrames,
        void(const char* pName));
    MOCK_METHOD0(StopBootProfilerSessionFrames,
        void());
    MOCK_METHOD0(OutputLoadingTimeStats,
        void());
    MOCK_METHOD0(GetLoadingProfilerCallstack,
        const char*());
    MOCK_METHOD0(GetFileVersion,
        const SFileVersion&());
    MOCK_METHOD0(GetProductVersion,
        const SFileVersion&());
    MOCK_METHOD0(GetBuildVersion,
        const SFileVersion&());

    MOCK_METHOD5(CompressDataBlock,
        bool(const void*, size_t, void*, size_t &, int));

    MOCK_METHOD4(DecompressDataBlock,
        bool(const void* input, size_t inputSize, void* output, size_t & outputSize));
    MOCK_METHOD1(AddCVarGroupDirectory,
        void(const string&));
    MOCK_METHOD0(SaveConfiguration,
        void());
    MOCK_METHOD3(LoadConfiguration,
        void(const char*, ILoadConfigurationEntrySink*, bool));

    MOCK_METHOD1(GetConfigSpec,
        ESystemConfigSpec(bool));
    MOCK_CONST_METHOD0(GetMaxConfigSpec,
        ESystemConfigSpec());
    MOCK_METHOD3(SetConfigSpec,
        void(ESystemConfigSpec spec, ESystemConfigPlatform platform, bool bClient));
    MOCK_CONST_METHOD0(GetConfigPlatform,
        ESystemConfigPlatform());
    MOCK_METHOD1(SetConfigPlatform,
        void(ESystemConfigPlatform platform));
    MOCK_METHOD1(AutoDetectSpec,
        void(bool detectResolution));
    MOCK_METHOD2(SetThreadState,
        int(ESubsystem subsys, bool bActive));
    MOCK_METHOD0(CreateSizer,
        ICrySizer * ());
    MOCK_CONST_METHOD0(IsPaused,
        bool());
    MOCK_METHOD0(GetLocalizationManager,
        ILocalizationManager * ());
    MOCK_METHOD0(GetITextModeConsole,
        ITextModeConsole * ());
    MOCK_METHOD0(GetNoiseGen,
        CPNoise3 * ());
    MOCK_METHOD0(GetUpdateCounter,
        uint64());
    MOCK_CONST_METHOD0(GetCryFactoryRegistry,
        ICryFactoryRegistry * ());
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
    MOCK_METHOD1(SetConsoleDrawEnabled,
        void(bool enabled));
    MOCK_METHOD1(SetUIDrawEnabled,
        void(bool enabled));
    MOCK_METHOD0(GetApplicationInstance,
        int());
    MOCK_METHOD1(GetApplicationLogInstance,
        int(const char* logFilePath));
    MOCK_METHOD0(GetCurrentUpdateTimeStats,
        sUpdateTimes & ());
    MOCK_METHOD2(GetUpdateTimeStats,
        const sUpdateTimes * (uint32 &, uint32 &));
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
    MOCK_METHOD0(DumpMemoryCoverage,
        void());
    MOCK_METHOD0(GetSystemGlobalState,
        ESystemGlobalState(void));
    MOCK_METHOD1(SetSystemGlobalState,
        void(ESystemGlobalState systemGlobalState));
    MOCK_METHOD5(AsyncMemcpy,
        void(void* dst, const void* src, size_t size, int nFlags, volatile int* sync));

#if defined(CVARS_WHITELIST)
    MOCK_CONST_METHOD0(GetCVarsWhiteList,
        ICVarsWhitelist * ());
#endif

#ifndef _RELEASE
    MOCK_METHOD1(GetCheckpointData,
        void(ICheckpointData & data));
    MOCK_METHOD0(IncreaseCheckpointLoadCount,
        void());
    MOCK_METHOD1(SetLoadOrigin,
        void(LevelLoadOrigin origin));
#endif

#if !defined(_RELEASE)
    MOCK_CONST_METHOD0(IsSavingResourceList,
        bool());
#endif

    MOCK_METHOD0(SteamInit,
        bool());
    MOCK_CONST_METHOD0(GetImageHandler,
        const IImageHandler * ());
    MOCK_METHOD3(InitializeEngineModule,
        bool(const char* dllName, const char* moduleClassName, const SSystemInitParams&initParams));
    MOCK_METHOD2(UnloadEngineModule,
        bool(const char* dllName, const char* moduleClassName));
    MOCK_METHOD0(GetRootWindowMessageHandler,
        void*());
    MOCK_METHOD1(RegisterWindowMessageHandler,
        void(IWindowMessageHandler * pHandler));
    MOCK_METHOD1(UnregisterWindowMessageHandler,
        void(IWindowMessageHandler * pHandler));
    MOCK_METHOD0(CreateLocalFileIO,
        std::shared_ptr<AZ::IO::FileIOBase>());

    MOCK_METHOD2(ForceMaxFps, void(bool, int));
};
