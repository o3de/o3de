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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#pragma once

#include "IRCLog.h"
#include "IResCompiler.h"
#include "CfgFile.h"
#include "Config.h"
#include "DependencyList.h"
#include "ExtensionManager.h"
#include "MultiplatformConfig.h"
#include "PakSystem.h"
#include "PakManager.h"
#include "RcFile.h"
#include "CryVersion.h"
#include "IProgress.h"
#include <ISystem.h>
#include <AzFramework/Archive/Codec.h>

#include <map>                 // stl multimap<>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

class CPropertyVars;
class XmlNodeRef;
class ICryXML;
ICryXML* LoadICryXML();

/** Implementation of IResCompiler interface.
*/
class ResourceCompiler
    : public IResourceCompiler
    , public IProgress
    , public IRCLog
    , public IConfigKeyRegistry
{
public:
    struct FilesToConvert
    {
        std::vector<RcFile> m_allFiles;
        std::vector<RcFile> m_inputFiles;
        std::vector<RcFile> m_outOfMemoryFiles;
        std::vector<RcFile> m_failedFiles;
        std::vector<RcFile> m_convertedFiles;
    };

    struct RcCompileFileInfo
    {
        ResourceCompiler* rc;
        FilesToConvert* pFilesToConvert;
        IConvertor* convertor;
        ICompiler* compiler;

        bool bLogMemory;
        bool bWarningHeaderLine;
        bool bErrorHeaderLine;
        string logHeaderLine;
    };

    ResourceCompiler();
    virtual ~ResourceCompiler();

    //! e.g. print statistics
    void PostBuild();

    int GetNumWarnings() const {return m_numWarnings; }
    int GetNumErrors() const {return m_numErrors; }

    // interface IProgress --------------------------------------------------

    void RegisterDefaultKeys();

    virtual void StartProgress();
    virtual void ShowProgress(const char* pMessage, size_t progressValue, size_t maxProgressValue);
    virtual void FinishProgress();

    // interface IConfigKeyRegistry ------------------------------------------

    virtual void VerifyKeyRegistration(const char* szKey) const;
    virtual bool HasKeyRegistered(const char* szKey) const;
    // -----------------------------------------------------------------------

    // interface IResourceCompiler -------------------------------------------

    virtual void RegisterKey(const char* key, const char* helptxt);

    virtual const char* GetExePath() const;
    virtual const char* GetTmpPath() const;
    virtual const char* GetInitialCurrentDir() const;
    const char* GetAppRoot() const override;


    virtual void RegisterConvertor(const char* name, IConvertor* conv);

    void AddPluginDLL(HMODULE pluginDLL);
    void RemovePluginDLL(HMODULE pluginDLL);

    virtual IPakSystem* GetPakSystem();

    virtual const ICfgFile* GetIniFile() const;

    virtual int GetPlatformCount() const;
    virtual const PlatformInfo* GetPlatformInfo(int index) const;
    virtual int FindPlatform(const char* name) const;

    virtual XmlNodeRef LoadXml(const char* filename);
    virtual XmlNodeRef CreateXml(const char* tag);

    virtual void AddInputOutputFilePair(const char* inputFilename, const char* outputFilename);
    virtual void MarkOutputFileForRemoval(const char* sOutputFilename);

    virtual void AddExitObserver(IExitObserver* p);
    virtual void RemoveExitObserver(IExitObserver* p);

    virtual IRCLog* GetIRCLog()
    {
        return this;
    }

    virtual int GetVerbosityLevel() const
    {
        return m_verbosityLevel;
    }

    virtual bool UseFastestDecompressionCodec() const
    {
        return m_bUseFastestDecompressionCodec;
    }

    virtual const SFileVersion& GetFileVersion() const
    {
        return m_fileVersion;
    }

    virtual const void GetGenericInfo(char* buffer, size_t bufferSize, const char* rowSeparator) const;
    string GetResourceCompilerGenericInfo(const string& newline) const;

    static bool CheckCommandLineOptions(const IConfig& config, const std::vector<string>* keysToIgnore);

    static void CopyStringToClipboard(const string& s);

    virtual bool CompileSingleFileBySingleProcess(const char* filename);

    virtual void SetAssetWriter(IAssetWriter* pAssetWriter)
    {
        m_pAssetWriter = pAssetWriter;
    }

    virtual IAssetWriter* GetAssetWriter() const
    {
        return m_pAssetWriter;
    }
    // -----------------------------------------------------------------------

    // interface IRCLog ------------------------------------------------------

    virtual void LogV(const IRCLog::EType eType, const char* szFormat, va_list args);
    virtual void Log(const IRCLog::EType eType, const char* szMessage);
    // -----------------------------------------------------------------------

    //////////////////////////////////////////////////////////////////////////
    // Resource compiler implementation.
    //////////////////////////////////////////////////////////////////////////

    void Init(Config& config);
    bool LoadIniFile();
    void UnregisterConvertors();

    bool AddPlatform(const string& names, bool bBigEndian, int pointerSize);

    MultiplatformConfig& GetMultiplatformConfig();

    void SetComplilingFileInfo(RcCompileFileInfo* compileFileInfo);

    bool CollectFilesToCompile(const string& filespec, std::vector<RcFile>& files);
    bool CompileFilesBySingleProcess(const std::vector<RcFile>& files);
    int  ProcessJobFile();
    void RemoveOutputFiles();     // to remove old files for less confusion
    void CleanTargetFolder(bool bUseOnlyInputFiles);

    bool CompileFile();

    const char* GetLogPrefix() const;

    //! call this if user asks for help
    void ShowHelp(bool bDetailed);
    //////////////////////////////////////////////////////////////////////////

    void SetAppRootPath(const string& appRootPath);

    static string GetAppRootPathFromGameRoot(const string& gameRootPath);

    void QueryVersionInfo();

    void InitPaths();

    void NotifyExitObservers();

    void InitLogs(Config& config);
    string FormLogFileName(const char* suffix) const;
    const string& GetMainLogFileName() const;
    const string& GetErrorLogFileName() const;

    clock_t GetStartTime() const;
    bool GetTimeLogging() const;
    void SetTimeLogging(bool enable);

    void LogMemoryUsage(bool bReportProblemsOnly);

    void InitPakManager();

    static string FindSuitableSourceRoot(const std::vector<string>& sourceRootsReversed, const string& fileName);
    static void GetSourceRootsReversed(const IConfig* config, std::vector<string>& sourceRootsReversed);

    void InitSystem(SSystemInitParams& startupParams);

private:
    void FilterExcludedFiles(std::vector<RcFile>& files);

    void CopyFiles(const std::vector<RcFile>& files, bool bNoOverwrite = false, bool recompress = false);
    /*!
      Files discovered at the \p directory node will be added to \p filenames with full path, if other
      directories are found inside \p directory then more files will be added recursively.
      \param directory The input directory node
      \param directoryName Full directory name of \p directory starting from the root directory.
                           It is expected that only for the root directory this string is "".
                           This function assumes that if this string is NOT empty, it includes the trailing "\\".
      \param filenames Output vector where filepaths will be added as files are recursively discovered.
      \returns void
    */
    void GetFileListRecursively(const ZipDir::FileEntryTree* directory, const AZStd::string& directoryName, AZStd::vector<AZStd::string>& filenames) const;
    bool RecompressFiles(const string& sourceFileName, const string& destinationFileName);

    bool    m_bUseFastestDecompressionCodec;

    void ExtractJobDefaultProperties(std::vector<string>& properties, const XmlNodeRef& jobNode);
    int  EvaluateJobXmlNode(CPropertyVars& properties, XmlNodeRef& jobNode, bool runJobs);
    int  RunJobByName(CPropertyVars& properties, XmlNodeRef& anyNode, const char* name);

    void ScanForAssetReferences(std::vector<string>& outReferences, const string& refsRoot);
    void SaveAssetReferences(const std::vector<string>& references, const string& filename, const string& includeMasks, const string& excludeMasks);

    void LogLine(const IRCLog::EType eType, const char* szText);

    // to log multiple lines (\n separated) with padding before
    void LogMultiLine(const char* szText);

    // -----------------------------------------------------------------------
public:
    static const char* const m_filenameRcExe;
    static const char* const m_filenameRcIni;
    static const char* const m_filenameOptions;
    static const char* const m_filenameLog;
    static const char* const m_filenameLogWarnings;
    static const char* const m_filenameLogErrors;
    static const char* const m_filenameCrashDump;
    static const char* const m_filenameOutputFileList;  //!< list of source=target filenames that rc processed, used for cleaning target folder
    static const char* const m_filenameDeletedFileList;
    static const char* const m_filenameCreatedFileList;
    static const char* const m_rcPluginSubfolder;       //!< The name of the subfolder where the Resource Compiler plugins are built to relative of rc

    bool m_bQuiet;                                      //!< true= don't log anything to console

private:
    static const size_t s_internalBufferSize = 4 * 1024;
    static const size_t s_environmentBufferSize = 64 * 1024;

    enum
    {
        kMaxPlatformCount = 20
    };
    int m_platformCount;
    PlatformInfo m_platforms[kMaxPlatformCount];

    ExtensionManager        m_extensionManager;

    IAssetWriter*             m_pAssetWriter;

    AZStd::mutex m_inputOutputFilesLock;
    CDependencyList         m_inputOutputFileList;

    std::vector<IExitObserver*> m_exitObservers;
    AZStd::mutex m_exitObserversLock;
    
    float                   m_memorySizePeakMb;
    AZStd::mutex m_memorySizeLock;
    
    // log files
    string                  m_logPrefix;
    string                  m_mainLogFileName;      //!< for all messages, might be empty (no file logging)
    string                  m_warningLogFileName;   //!< for warnings only, might be empty (no file logging)
    string                  m_errorLogFileName;     //!< for errors only, might be empty (no file logging)
    string                  m_logHeaderLine;
    AZStd::mutex m_logLock;
    
    clock_t                 m_startTime;
    bool                    m_bTimeLogging;

    float                   m_progressLastPercent;
    int                     m_verbosityLevel;

    CfgFile                 m_iniFile;
    MultiplatformConfig     m_multiConfig;

    bool                    m_bWarningHeaderLine;   //!< true= header was already printed, false= header needs to be printed
    bool                    m_bErrorHeaderLine;     //!< true= header was already printed, false= header needs to be printed
    bool                    m_bWarningsAsErrors;    //!< true= treat any warning as error.

    RcCompileFileInfo*      m_currentRcCompileFileInfo;

    SFileVersion            m_fileVersion;
    string                  m_exePath;
    string                  m_tempPath;
    string                  m_initialCurrentDir;
    string                  m_appRoot;

    std::map<string, string> m_KeyHelp;              // [lower key] = help, created from RegisterKey

    // Files to delete
    std::vector<RcFile>     m_inputFilesDeleted;

    PakManager*             m_pPakManager;

    int                     m_numWarnings;
    int                     m_numErrors;

    std::vector<HMODULE> m_loadedPlugins;

#if AZ_TRAIT_OS_PLATFORM_APPLE
    bool                    m_isRunningFromBundle;
    string                  m_bundleRoot;
#endif
};

