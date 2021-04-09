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

// Description : Defines the entry point for the console application.


#include "ResourceCompiler_precompiled.h"

#include <time.h>
#include <stdio.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include "MathHelpers.h"
#include <psapi.h>       // GetProcessMemoryInfo()
#endif //AZ_PLATFORM_WINDOWS

#include "ResourceCompiler.h"

#include "Config.h"
#include "CfgFile.h"
#include "FileUtil.h"
#include "IConvertor.h"

#include "CryPath.h"
#include "StringHelpers.h"
#include "ListFile.h"
#include "Util.h"
#include "ICryXML.h"
#include "IXMLSerializer.h"

#include "NameConvertor.h"
#include "CryCrc32.h"
#include "PropertyVars.h"
#include "StlUtils.h"
#include "TextFileReader.h"
#include "IResourceCompilerHelper.h"
#include "SettingsManagerHelpers.h"
#include "CryLibrary.h"

#include <AzCore/Module/Environment.h>
#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/CommandLine/CommandLine.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/Debug/TraceMessagesDrillerBus.h>
#include <AzCore/base.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/std/parallel/thread.h>
#include <AzCore/std/parallel/binary_semaphore.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Utils/Utils.h>

#include <QThread>
#include <QApplication>
#include <AzCore/Memory/AllocatorManager.h>

#if AZ_TRAIT_OS_PLATFORM_APPLE
#include <mach-o/dyld.h>  // Needed for _NSGetExecutablePath
#endif

#pragma comment( lib, "Version.lib" )

const char* const ResourceCompiler::m_filenameRcExe            = "rc.exe";
const char* const ResourceCompiler::m_filenameRcIni            = "rc.ini";
const char* const ResourceCompiler::m_filenameOptions          = "rc_options.txt";
const char* const ResourceCompiler::m_filenameLog              = "rc_log.log";
const char* const ResourceCompiler::m_filenameLogWarnings      = "rc_log_warnings.log";
const char* const ResourceCompiler::m_filenameLogErrors        = "rc_log_errors.log";
const char* const ResourceCompiler::m_filenameCrashDump        = "rc_crash.dmp";
const char* const ResourceCompiler::m_filenameOutputFileList   = "rc_outputfiles.txt";
const char* const ResourceCompiler::m_filenameDeletedFileList  = "rc_deletedfiles.txt";
const char* const ResourceCompiler::m_filenameCreatedFileList  = "rc_createdfiles.txt";
const char* const ResourceCompiler::m_rcPluginSubfolder        = "rc_plugins";

const size_t ResourceCompiler::s_internalBufferSize;
const size_t ResourceCompiler::s_environmentBufferSize;

namespace
{
    volatile bool g_gotCTRLBreakSignalFromOS = false;
#if defined(AZ_PLATFORM_WINDOWS)
    BOOL WINAPI CtrlHandlerRoutine([[maybe_unused]] DWORD dwCtrlType)
    {
        // we got this.
        RCLogError("CTRL-BREAK was pressed!");
        g_gotCTRLBreakSignalFromOS = true;
        return TRUE;
    }
#else
    void CtrlHandlerRoutine(int signo)
    {
        if (signo == SIGINT)
        {
            RCLogError("CTRL-BREAK was pressed!");
            g_gotCTRLBreakSignalFromOS = true;
        }
    }
#endif

}

// used to intercept asserts during shutdown.
class AssertInterceptor
    : public AZ::Debug::TraceMessageBus::Handler
{
public:
    AssertInterceptor()
    {
        BusConnect();
    }

    virtual ~AssertInterceptor()
    {
    }

    bool OnAssert(const char* message) override
    {
        RCLogWarning("An assert occurred during shutdown!  %s", message);
        return true;
    }
};

//////////////////////////////////////////////////////////////////////////
// ResourceCompiler implementation.
//////////////////////////////////////////////////////////////////////////
ResourceCompiler::ResourceCompiler()
    : m_platformCount(0)
    , m_bQuiet(false)
    , m_verbosityLevel(0)
    , m_numWarnings(0)
    , m_numErrors(0)
    , m_memorySizePeakMb(0)
    , m_pAssetWriter(nullptr)
    , m_pPakManager(nullptr)
    , m_currentRcCompileFileInfo(nullptr)
{
    // install our ctrl handler by default, before we load system modules.  Unfortunately
    // some modules may install their own, so we will install ours again after loading perforce and crysystem.
#if defined(AZ_PLATFORM_WINDOWS)
    SetConsoleCtrlHandler((PHANDLER_ROUTINE)CtrlHandlerRoutine, TRUE);
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
    signal(SIGINT, CtrlHandlerRoutine);
#endif
}

ResourceCompiler::~ResourceCompiler()
{
    delete m_pPakManager;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::StartProgress()
{
    m_progressLastPercent = -1;
}

void ResourceCompiler::ShowProgress(const char* pMessage, size_t progressValue, size_t maxProgressValue)
{
    float percent = (progressValue * 100.0f) / maxProgressValue;
    if ((percent <= 100.0f) && (percent != m_progressLastPercent))
    {
        m_progressLastPercent = percent;
        char str[1024];
        azsnprintf(str, sizeof(str), "Progress: %d.%d%% %s", int(percent), int(percent * 10.0f) % 10, pMessage);
#if defined(AZ_PLATFORM_WINDOWS)
        SetConsoleTitle(str);
#else
        //TODO:: Needs cross platform support.
#endif
    }
}

void ResourceCompiler::FinishProgress()
{
#if defined(AZ_PLATFORM_WINDOWS)
    SetConsoleTitle("Progress: 100%");
#else
    //TODO:: Needs cross platform support.
#endif
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::RegisterConvertor(const char* name, IConvertor* conv)
{
    m_extensionManager.RegisterConvertor(name, conv, this);
}

//////////////////////////////////////////////////////////////////////////
IPakSystem* ResourceCompiler::GetPakSystem()
{
    return (m_pPakManager ? m_pPakManager->GetPakSystem() : 0);
}

//////////////////////////////////////////////////////////////////////////
const ICfgFile* ResourceCompiler::GetIniFile() const
{
    return &m_iniFile;
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::GetPlatformCount() const
{
    return m_platformCount;
}

//////////////////////////////////////////////////////////////////////////
const PlatformInfo* ResourceCompiler::GetPlatformInfo(int index) const
{
    if (index < 0 || index > m_platformCount)
    {
        AZ_Assert(false, "assert");
        return 0;
    }
    return &m_platforms[index];
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::FindPlatform(const char* name) const
{
    for (int i = 0; i < m_platformCount; ++i)
    {
        if (m_platforms[i].HasName(name))
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::AddPlatform(const string& names, bool bBigEndian, int pointerSize)
{
    if (m_platformCount >= kMaxPlatformCount)
    {
        return false;
    }

    if (pointerSize != 4 && pointerSize != 8)
    {
        return false;
    }

    std::vector<string> arrNames;
    StringHelpers::SplitByAnyOf(names, ",; ", false, arrNames);

    if (arrNames.empty() || arrNames.size() > PlatformInfo::kMaxPlatformNames)
    {
        return false;
    }

    PlatformInfo& p = m_platforms[m_platformCount];

    p.Clear();

    for (size_t i = 0; i < arrNames.size(); ++i)
    {
        if (!p.SetName(i, arrNames[i].c_str()))
        {
            return false;
        }
    }

    p.index = m_platformCount++;
    p.bBigEndian = bBigEndian;
    p.pointerSize = pointerSize;

    return true;
}

//////////////////////////////////////////////////////////////////////////
const char* ResourceCompiler::GetLogPrefix() const
{
    return m_logPrefix.c_str();
}

void ResourceCompiler::RemoveOutputFiles()
{
    AZ::IO::SystemFile::Delete(FormLogFileName(m_filenameDeletedFileList));
    AZ::IO::SystemFile::Delete(FormLogFileName(m_filenameCreatedFileList));
}

static void ThreadFunc(ResourceCompiler::RcCompileFileInfo* data);

class CompileFileThread
    : public QThread
{
public:
    CompileFileThread(const ResourceCompiler::RcCompileFileInfo& compileInfo)
        : m_compileInfo(compileInfo)
    {}
private:
    void run()
    {
        ThreadFunc(&m_compileInfo);
    }
    ResourceCompiler::RcCompileFileInfo m_compileInfo;
};

static void CompileFilesMultiThreaded(
    ResourceCompiler* pRC,
    ResourceCompiler::FilesToConvert& a_files,
    IConvertor* convertor)
{
    AZ_Assert(pRC, "assert");

    bool bLogMemory = false;

    while (!a_files.m_inputFiles.empty())
    {
        // Initialize the convertor
        {
            ConvertorInitContext initContext;
            initContext.config = &pRC->GetMultiplatformConfig().getConfig();
            initContext.inputFiles = a_files.m_inputFiles.empty() ? 0 : &a_files.m_inputFiles[0];
            initContext.inputFileCount = a_files.m_inputFiles.size();
            initContext.appRootPath = pRC->GetAppRoot();
            convertor->Init(initContext);
        }

        // Initialize the thread data for each thread.
        ResourceCompiler::RcCompileFileInfo compileInfo;
        compileInfo.rc = pRC;
        compileInfo.convertor = convertor;
        compileInfo.compiler = convertor->CreateCompiler();
        compileInfo.pFilesToConvert = &a_files;
        compileInfo.bLogMemory = bLogMemory;
        compileInfo.bWarningHeaderLine = false;
        compileInfo.bErrorHeaderLine = false;
        compileInfo.logHeaderLine = "";

        AZStd::binary_semaphore waiter;

        // Spawn the thread. The old /threads option is no longer supported, and this should remain limited to one thread.
        // Although this is limited to a single thread, running ThreadFunc on the main thread leads to other issues (LY-85364, LY-85364)
        // so we should still be creating a new thread here
        QScopedPointer<CompileFileThread> thread(new CompileFileThread(compileInfo));
        bool done = false;
        QObject::connect(thread.data(), &QThread::finished, thread.data(), [&waiter, &done]()
        {
            // this makes the app break out of its event loop (below) ASAP.
            done = true;
            waiter.release();
        });
        thread->start();

        // Wait until the thread has exited
        while (!done)
        {
            // unfortunately the below call to processEvents(...),
            // even with a 'wait for more events' flag applied, causes it to still use up an entire CPU core
            // on windows platforms due to implementation details.
            // To avoid this, we will periodically pump events, but we will also wait on a semaphore to be raised to let us quit instantly
            // this allows us to spend most of our time with a sleeping main thread, but still instantly exit once the job is done.
            waiter.try_acquire_for(AZStd::chrono::milliseconds(50));
            qApp->processEvents(QEventLoop::AllEvents);
        }

        pRC->FinishProgress();

        AZ_Assert(a_files.m_inputFiles.empty(), "assert");

        // Release the compiler object
        compileInfo.compiler->Release();

        // Clean up the converter.
        convertor->DeInit();

        if (!a_files.m_outOfMemoryFiles.empty())
        {
            pRC->LogMemoryUsage(false);
            RCLogError("Run out of memory while processing %i file(s):", (int)a_files.m_outOfMemoryFiles.size());
            for (int i = 0; i < (int)a_files.m_outOfMemoryFiles.size(); ++i)
            {
                const RcFile& rf = a_files.m_outOfMemoryFiles[i];
                RCLogError(" \"%s\" \"%s\"", rf.m_sourceLeftPath.c_str(), rf.m_sourceInnerPathAndName.c_str());
            }

            a_files.m_failedFiles.insert(a_files.m_failedFiles.end(), a_files.m_outOfMemoryFiles.begin(), a_files.m_outOfMemoryFiles.end());
            a_files.m_outOfMemoryFiles.resize(0);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
typedef std::set<string, stl::less_stricmp<string> > TStringSet;
// Adds file into vector<RcFile> ensuring that only one sourceInnerPathAndName exists.
static void AddRcFile(
    std::vector<RcFile>& files,
    TStringSet& addedFilenames,
    const std::vector<string>& sourceRootsReversed,
    const string& sourceLeftPath,
    const string& sourceInnerPathAndName,
    const string& targetLeftPath)
{
    if (sourceRootsReversed.size() == 1)
    {
        files.push_back(RcFile(sourceLeftPath, sourceInnerPathAndName, targetLeftPath));
    }
    else
    {
        if (addedFilenames.find(sourceInnerPathAndName) == addedFilenames.end())
        {
            files.push_back(RcFile(sourceLeftPath, sourceInnerPathAndName, targetLeftPath));
            addedFilenames.insert(sourceInnerPathAndName);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::GetSourceRootsReversed(const IConfig* config, std::vector<string>& sourceRootsReversed)
{
    const int verbosityLevel = config->GetAsInt("verbose", 0, 1);
    const string sourceRootsStr = config->GetAsString("sourceroot", "", "");

    std::vector<string> sourceRoots;
    StringHelpers::Split(sourceRootsStr, ";", true, sourceRoots);

    const size_t sourceRootCount = sourceRoots.size();
    sourceRootsReversed.resize(sourceRootCount);

    if (verbosityLevel >= 2)
    {
        RCLog("Source Roots (%i):", (int)sourceRootCount);
    }

    for (size_t i = 0; i < sourceRootCount; ++i)
    {
        sourceRootsReversed[i] = PathHelpers::GetAbsoluteAsciiPath(sourceRoots[sourceRootCount - 1 - i]);
        sourceRootsReversed[i] = PathHelpers::ToPlatformPath(sourceRootsReversed[i]);

        if (verbosityLevel >= 3)
        {
            RCLog("  [%i]: '%s' (%s)", (int)i, sourceRootsReversed[i].c_str(), sourceRoots[sourceRootCount - 1 - i].c_str());
        }
        else if (verbosityLevel >= 2)
        {
            RCLog("  [%i]: '%s'", (int)i, sourceRootsReversed[i].c_str());
        }
    }

    if (sourceRootsReversed.empty())
    {
        sourceRootsReversed.push_back(string());
    }
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::CollectFilesToCompile(const string& filespec, std::vector<RcFile>& files)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    files.clear();

    const bool bVerbose = GetVerbosityLevel() > 0;
    const bool bRecursive = config->GetAsBool("recursive", true, true);
    const bool bSkipMissing = config->GetAsBool("skipmissing", false, true);

    // RC expects source filenames in command line in form
    // "<source left path><mask for recursion>" or "<source left path><non-masked name>".
    // After recursive subdirectory scan we will have a list with source filenames in
    // form "<source left path><source inner path><name>".
    // The target filename can be written as "<target left path><source inner path><name>".

    // Determine the target output path (may be a different directory structure).
    // If none is specified, the target path is the same as the <source left path>.
    const string targetLeftPath = PathHelpers::ToPlatformPath(PathHelpers::CanonicalizePath(config->GetAsString("targetroot", "", "")));

    std::vector<string> sourceRootsReversed;
    GetSourceRootsReversed(config, sourceRootsReversed);

    const string listFile = PathHelpers::ToPlatformPath(config->GetAsString("listfile", "", ""));

    TStringSet addedFiles;
    if (!listFile.empty())
    {
        const string listFormat = config->GetAsString("listformat", "", "");

        for (size_t i = 0; i < sourceRootsReversed.size(); ++i)
        {
            const string& sourceRoot = sourceRootsReversed[i];
            {
                std::vector< std::pair<string, string> > filenames;

                CListFile(this).Process(listFile, listFormat, filespec, sourceRoot, filenames);

                for (size_t i2 = 0; i2 < filenames.size(); ++i2)
                {
                    AddRcFile(files, addedFiles, sourceRootsReversed, filenames[i2].first, filenames[i2].second, targetLeftPath);
                }
            }
        }

        if (files.empty())
        {
            if (!bSkipMissing)
            {
                RCLogError("No files to convert found in list file \"%s\" (filter is \"%s\")", listFile.c_str(), filespec.c_str());
            }
            return bSkipMissing;
        }

        if (bVerbose)
        {
            RCLog("Contents of the list file \"%s\" (filter is \"%s\"):", listFile.c_str(), filespec.c_str());
            for (size_t i = 0; i < files.size(); ++i)
            {
                RCLog(" %3d: \"%s\" \"%s\"", i, files[i].m_sourceLeftPath.c_str(), files[i].m_sourceInnerPathAndName.c_str());
            }
        }
    }
    else
    {
        bool wildcardSearch = false;
        // It's a mask (path\*.mask). Scan directory and accumulate matching filenames in the list.
        // Multiple masks allowed, for example path\*.xml;*.dlg;path2\*.mtl

        std::vector<string> tokens;
        StringHelpers::Split(filespec, ";", false, tokens);

        for (size_t i = 0; i < sourceRootsReversed.size(); ++i)
        {
            const string& sourceRoot = sourceRootsReversed[i];
            for (size_t t = 0; t < tokens.size(); ++t)
            {
                if (tokens[t].find_first_of("*?") != string::npos)
                {
                    wildcardSearch = true;
                    // Scan directory and accumulate matching filenames in the list.
                    const string path = PathHelpers::ToPlatformPath(PathHelpers::Join(sourceRoot, PathHelpers::GetDirectory(tokens[t])));
                    const string pattern = PathHelpers::GetFilename(tokens[t]);
                    RCLog("Scanning directory '%s' for '%s'...", path.c_str(), pattern.c_str());
                    std::vector<string> filenames;
                    FileUtil::ScanDirectory(path, pattern, filenames, bRecursive, targetLeftPath);
                    for (size_t i2 = 0; i2 < filenames.size(); ++i2)
                    {
                        string sourceLeftPath;
                        string sourceInnerPathAndName;
                        if (sourceRoot.empty())
                        {
                            sourceLeftPath = PathHelpers::GetDirectory(tokens[t]);
                            sourceInnerPathAndName = filenames[i2];
                        }
                        else
                        {
                            sourceLeftPath = sourceRoot;
                            sourceInnerPathAndName = PathHelpers::Join(PathHelpers::GetDirectory(tokens[t]), filenames[i2]);
                        }

                        const DWORD dwFileSpecAttr = GetFileAttributes(PathHelpers::Join(sourceLeftPath, sourceInnerPathAndName));
                        if (dwFileSpecAttr & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            RCLog("Skipping adding file '%s' matched by wildcard '%s' because it is a directory", sourceInnerPathAndName.c_str(), filespec.c_str());
                        }
                        else
                        {
                            AddRcFile(files, addedFiles, sourceRootsReversed, sourceLeftPath, sourceInnerPathAndName, targetLeftPath);
                        }
                    }
                }
                else
                {
                    const auto& thisFile = tokens[t];
                    const DWORD dwFileSpecAttr = GetFileAttributes(PathHelpers::Join(sourceRoot, thisFile));

                    if (dwFileSpecAttr == INVALID_FILE_ATTRIBUTES)
                    {
                        // There's no such file
                        RCLog("RC did not find %s in %s", thisFile.c_str(), sourceRoot.c_str());
                        if (sourceRoot.empty())
                        {
                            AddRcFile(files, addedFiles, sourceRootsReversed, PathHelpers::GetDirectory(thisFile), PathHelpers::GetFilename(thisFile), targetLeftPath);
                        }
                        else
                        {
                            AddRcFile(files, addedFiles, sourceRootsReversed, sourceRoot, thisFile, targetLeftPath);
                        }
                    }
                    else
                    {
                        // The file exists

                        if (dwFileSpecAttr & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            // We found a file, but it's a directory, not a regular file.
                            // Let's assume that the user wants to export every file in the
                            // directory (with subdirectories if bRecursive == true) or
                            // that he wants to export a file specified in /file option.
                            const string path = PathHelpers::Join(sourceRoot, thisFile);
                            const string pattern = config->GetAsString("file", "*.*", "*.*");
                            RCLog("Scanning directory '%s' for '%s'...", path.c_str(), pattern.c_str());
                            std::vector<string> filenames;
                            FileUtil::ScanDirectory(path, pattern, filenames, bRecursive, targetLeftPath);
                            for (size_t i2 = 0; i2 < filenames.size(); ++i2)
                            {
                                string sourceLeftPath;
                                string sourceInnerPathAndName;
                                if (sourceRoot.empty())
                                {
                                    sourceLeftPath = thisFile;
                                    sourceInnerPathAndName = filenames[i2];
                                }
                                else
                                {
                                    sourceLeftPath = sourceRoot;
                                    sourceInnerPathAndName = PathHelpers::Join(thisFile, filenames[i2]);
                                }
                                AddRcFile(files, addedFiles, sourceRootsReversed, sourceLeftPath, sourceInnerPathAndName, targetLeftPath);
                            }
                        }
                        else
                        {
                            RCLog("Found %s in %s", thisFile.c_str(), sourceRoot.c_str());
                            // we found a regular file
                            if (sourceRoot.empty())
                            {
                                AddRcFile(files, addedFiles, sourceRootsReversed, PathHelpers::GetDirectory(thisFile), PathHelpers::GetFilename(thisFile), targetLeftPath);
                            }
                            else
                            {
                                AddRcFile(files, addedFiles, sourceRootsReversed, sourceRoot, thisFile, targetLeftPath);
                            }
                        }
                    }
                }
            }
        }

        if (files.empty())
        {
            if (wildcardSearch)
            {
                // We failed to find any file matching the mask specified by user.
                // Using mask (say, *.cgf) usually means that user doesn't know if
                // the file exists or not, so it's better to return "success" code.
                RCLog("RC can't find files matching '%s', 0 files converted", filespec.c_str());
                return true;
            }
            if (!bSkipMissing)
            {
                RCLogError("No files found to convert.");
            }
            return bSkipMissing;
        }
    }

    // Remove excluded files from the list of files to process.
    FilterExcludedFiles(files);

    if (files.empty())
    {
        if (!bSkipMissing)
        {
            RCLogError("No files to convert (all files were excluded by /exclude command).");
        }
        return bSkipMissing;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// Returns true if successfully converted at least one file
bool ResourceCompiler::CompileFilesBySingleProcess(const std::vector<RcFile>& files)
{
    const IConfig* const config = &m_multiConfig.getConfig();
    const clock_t compilationStartTime = clock();

    FilesToConvert filesToConvert;
    filesToConvert.m_allFiles = files;

    if (g_gotCTRLBreakSignalFromOS)
    {
        // abort!
        return false;
    }
    bool bRecompress = false;
    if (config->GetAsBool("recompress", false, false))
    {
        RCLog("Recompressing files with fastest decompressor for data");
        bRecompress = true;
    }
    if (config->GetAsBool("copyonly", false, true) || config->GetAsBool("copyonlynooverwrite", false, true))
    {
        const string targetroot = PathHelpers::ToPlatformPath(PathHelpers::CanonicalizePath(config->GetAsString("targetroot", "", "")));
        if (targetroot.empty() && config->GetAsBool("copyonly", false, true))
        {
            RCLogError("/copyonly: you must specify /targetroot.");
            return false;
        }
        else if (targetroot.empty() && config->GetAsBool("copyonlynooverwrite", false, true))
        {
            RCLogError("/copyonlynooverwrite: you must specify /targetroot.");
            return false;
        }
        CopyFiles(filesToConvert.m_allFiles, config->GetAsBool("copyonlynooverwrite", false, true), bRecompress);
        if (!config->GetAsBool("outputproductdependencies", false, true))
        {
            return true;
        }
    }
    else if (config->GetAsBool("outputproductdependencies", false, true))
    {
        RCLogError("/outputproductdependencies: you can only use this argument to output product dependencies for copy jobs.");
        return false;
    }

    const PakManager::ECallResult eResult = m_pPakManager->CompileFilesIntoPaks(config, filesToConvert.m_allFiles);

    if (eResult != PakManager::eCallResult_Skipped)
    {
        return eResult == PakManager::eCallResult_Succeeded;
    }

    //////////////////////////////////////////////////////////////////////////

    // Split up the files based on the convertor they are to use.
    typedef std::map<IConvertor*, std::vector<RcFile> > FileConvertorMap;
    FileConvertorMap fileConvertorMap;
    for (size_t i = 0; i < filesToConvert.m_allFiles.size(); ++i)
    {
        if (g_gotCTRLBreakSignalFromOS)
        {
            // abort!
            return false;
        }

        string filenameForConvertorSearch;
        {
            const string sOverWriteExtension = config->GetAsString("overwriteextension", "", "");
            if (!sOverWriteExtension.empty())
            {
                filenameForConvertorSearch = string("filename.") + sOverWriteExtension;
            }
            else
            {
                filenameForConvertorSearch = filesToConvert.m_allFiles[i].m_sourceInnerPathAndName;
            }
        }

        IConvertor* const convertor = m_extensionManager.FindConvertor(filenameForConvertorSearch.c_str());

        if (!convertor)
        {
            // it is an error to fail to find a convertor.
            RCLogError("Cannot find convertor for %s", filenameForConvertorSearch.c_str());
            filesToConvert.m_failedFiles.push_back(filesToConvert.m_allFiles[i]);
            filesToConvert.m_allFiles.erase(filesToConvert.m_allFiles.begin() + i);

            --i;
            continue;
        }

        FileConvertorMap::iterator convertorIt = fileConvertorMap.find(convertor);
        if (convertorIt == fileConvertorMap.end())
        {
            convertorIt = fileConvertorMap.insert(std::make_pair(convertor, std::vector<RcFile>())).first;
        }
        (*convertorIt).second.push_back(filesToConvert.m_allFiles[i]);
    }

    if (GetVerbosityLevel() > 0)
    {
        RCLog("%d file%s to convert:", filesToConvert.m_allFiles.size(), ((filesToConvert.m_allFiles.size() != 1) ? "s" : ""));
        for (size_t i = 0; i < filesToConvert.m_allFiles.size(); ++i)
        {
            RCLog("  \"%s\"  \"%s\" -> \"%s\"",
                filesToConvert.m_allFiles[i].m_sourceLeftPath.c_str(),
                filesToConvert.m_allFiles[i].m_sourceInnerPathAndName.c_str(),
                filesToConvert.m_allFiles[i].m_targetLeftPath.c_str());
        }
        RCLog("");
    }

    // Loop through all the convertors that we need to invoke.
    for (FileConvertorMap::iterator convertorIt = fileConvertorMap.begin(); convertorIt != fileConvertorMap.end(); ++convertorIt)
    {
        AZ_Assert(filesToConvert.m_inputFiles.empty(), "assert");
        AZ_Assert(filesToConvert.m_outOfMemoryFiles.empty(), "assert");

        IConvertor* const convertor = (*convertorIt).first;
        AZ_Assert(convertor, "assert");

        const std::vector<RcFile>& convertorFiles = (*convertorIt).second;
        AZ_Assert(convertorFiles.size() > 0, "assert");

        // implementation note: we insert filenames starting from last, because converting function will take filenames one by one from the end(!) of the array
        for (int i = convertorFiles.size() - 1; i >= 0; --i)
        {
            filesToConvert.m_inputFiles.push_back(convertorFiles[i]);
        }

        LogMemoryUsage(false);

        CompileFilesMultiThreaded(this, filesToConvert, convertor);

        AZ_Assert(filesToConvert.m_inputFiles.empty(), "assert");
        AZ_Assert(filesToConvert.m_outOfMemoryFiles.empty(), "assert");
    }

    const int numFilesConverted = filesToConvert.m_convertedFiles.size();
    const int numFilesFailed = filesToConvert.m_failedFiles.size();
    AZ_Assert(numFilesConverted + numFilesFailed == filesToConvert.m_allFiles.size(), "assert");

    const bool savedTimeLogging = GetTimeLogging();
    SetTimeLogging(false);

    char szTimeMsg[128];
    const float secondsElapsed = float(clock() - compilationStartTime) / CLOCKS_PER_SEC;
    azsnprintf(szTimeMsg, sizeof(szTimeMsg), " in %.1f sec", secondsElapsed);

    RCLog("");

    if (numFilesFailed <= 0)
    {
        RCLog("%d file%s processed%s.", numFilesConverted, (numFilesConverted > 1 ? "s" : ""), szTimeMsg);
        RCLog("");
    }
    else
    {
        RCLog("");
        RCLog(
            "%d of %d file%s were converted%s. Couldn't convert the following file%s:",
            numFilesConverted,
            numFilesConverted + numFilesFailed,
            (numFilesConverted + numFilesFailed > 1 ? "s" : ""),
            szTimeMsg,
            (numFilesFailed > 1 ? "s" : ""));
        RCLog("");
        for (size_t i = 0; i < numFilesFailed; ++i)
        {
            const string failedFilename = PathHelpers::Join(filesToConvert.m_failedFiles[i].m_sourceLeftPath, filesToConvert.m_failedFiles[i].m_sourceInnerPathAndName);
            RCLog("  %s", failedFilename.c_str());
        }
        RCLog("");
    }

    SetTimeLogging(savedTimeLogging);

    return numFilesConverted > 0;
}

bool ResourceCompiler::CompileSingleFileBySingleProcess(const char* filename)
{
    std::vector<RcFile> list;
    list.push_back(RcFile("", filename, ""));
    return CompileFilesBySingleProcess(list);
}

void ThreadFunc(ResourceCompiler::RcCompileFileInfo* data)
{
    AZ_Assert(data, "assert");

#if defined(AZ_PLATFORM_WINDOWS)
    MathHelpers::EnableFloatingPointExceptions(~(_CW_DEFAULT));
#endif
    data->rc->SetComplilingFileInfo(data);
    data->compiler->BeginProcessing(&data->rc->GetMultiplatformConfig().getConfig());

    while (!data->pFilesToConvert->m_inputFiles.empty())
    {
        if (g_gotCTRLBreakSignalFromOS)
        {
            RCLogError("Abort was requested during compilation.");
            data->pFilesToConvert->m_failedFiles.insert(data->pFilesToConvert->m_failedFiles.begin(), data->pFilesToConvert->m_inputFiles.begin(), data->pFilesToConvert->m_inputFiles.end());
            data->pFilesToConvert->m_inputFiles.clear();
        }

        enum EResult
        {
            eResult_Ok,
            eResult_Error,
            eResult_OutOfMemory,
            eResult_Exception
        };
        EResult eResult;

        const RcFile& fileToConvert = data->pFilesToConvert->m_inputFiles.back();
        if (data->rc->CompileFile())
        {
            eResult = eResult_Ok;
        }
        else
        {
            eResult = eResult_Error;
        }

        data->pFilesToConvert->m_inputFiles.pop_back();

        switch (eResult)
        {
        case eResult_Ok:
            data->pFilesToConvert->m_convertedFiles.push_back(fileToConvert);
            break;
        case eResult_Error:
            data->pFilesToConvert->m_failedFiles.push_back(fileToConvert);
            break;
        case eResult_OutOfMemory:
            data->rc->LogMemoryUsage(false);
            RCLogWarning("Run out of memory: \"%s\" \"%s\"", fileToConvert.m_sourceLeftPath.c_str(), fileToConvert.m_sourceInnerPathAndName.c_str());
            data->pFilesToConvert->m_outOfMemoryFiles.push_back(fileToConvert);
            break;
        case eResult_Exception:
            data->rc->LogMemoryUsage(false);
            RCLogError("Unknown failure: \"%s\" \"%s\"", fileToConvert.m_sourceLeftPath.c_str(), fileToConvert.m_sourceInnerPathAndName.c_str());
            data->pFilesToConvert->m_failedFiles.push_back(fileToConvert);
            break;
        default:
            AZ_Assert(false, "assert");
            break;
        }
    }

    data->compiler->EndProcessing();
    data->rc->SetComplilingFileInfo(nullptr);
}


const char* ResourceCompiler::GetExePath() const
{
    return m_exePath.c_str();
}

const char* ResourceCompiler::GetTmpPath() const
{
    return m_tempPath.c_str();
}

const char* ResourceCompiler::GetInitialCurrentDir() const
{
    return m_initialCurrentDir.c_str();
}

const char* ResourceCompiler::GetAppRoot() const
{
    return m_appRoot.c_str();
}


//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::CompileFile()
{
    if (!m_currentRcCompileFileInfo)
    {
        return false;
    }

    RcCompileFileInfo& compileFileInfo = *m_currentRcCompileFileInfo;
    const RcFile& fileToConvert = compileFileInfo.pFilesToConvert->m_allFiles.back();
    const string sourceInnerPath = PathHelpers::GetDirectory(fileToConvert.m_sourceInnerPathAndName);
    const string sourceFullFileName = PathHelpers::Join(fileToConvert.m_sourceLeftPath, fileToConvert.m_sourceInnerPathAndName);
    const string targetLeftPath = fileToConvert.m_targetLeftPath;
    const string targetFullFileName = PathHelpers::Join(targetLeftPath, sourceInnerPath);
    ICompiler* compiler = compileFileInfo.compiler;

    const bool bMemoryReportProblemsOnly = !compileFileInfo.bLogMemory;
    LogMemoryUsage(bMemoryReportProblemsOnly);

    MultiplatformConfig localMultiConfig = m_multiConfig;
    const IConfig* const config = &m_multiConfig.getConfig();

    if (GetVerbosityLevel() >= 2)
    {
        RCLog("CompileFile():");
        RCLog("  sourceFullFileName: '%s'", sourceFullFileName.c_str());
        RCLog("  targetLeftPath: '%s'", targetLeftPath.c_str());
        RCLog("  sourceInnerPath: '%s'", sourceInnerPath.c_str());
        RCLog("  targetPath: '%s'", targetFullFileName.c_str());
    }

    // Setup conversion context.
    IConvertContext* const pCC = compiler->GetConvertContext();

    pCC->SetMultiplatformConfig(&localMultiConfig);
    pCC->SetRC(this);

    {
        bool bRefresh = config->GetAsBool("refresh", false, true);

        // Force "refresh" to be true if user asked for dialog - it helps a lot
        // when a command line used, because users very often forget to specify /refresh
        // in such cases. Also, some tools, including CryTif exporter, call RC without
        // providing /refresh
        if (config->GetAsBool("userdialog", false, true))
        {
            bRefresh = true;
        }

        pCC->SetForceRecompiling(bRefresh);
    }

    {
        const string sourceExtension = PathHelpers::FindExtension(sourceFullFileName);
        const string convertorExtension = config->GetAsString("overwriteextension", sourceExtension, sourceExtension);
        pCC->SetConvertorExtension(convertorExtension);
    }

    pCC->SetSourceFileNameOnly(PathHelpers::GetFilename(sourceFullFileName));
    pCC->SetSourceFolder(PathHelpers::GetDirectory(PathHelpers::GetAbsoluteAsciiPath(sourceFullFileName)));

    const string outputFolder = PathHelpers::GetAbsoluteAsciiPath(targetFullFileName.c_str());
    pCC->SetOutputFolder(outputFolder);

    if (!FileUtil::EnsureDirectoryExists(outputFolder.c_str()))
    {
        RCLogError("Creating directory failed: %s", outputFolder.c_str());
        return false;
    }

    if (GetVerbosityLevel() >= 0)
    {
        RCLog("---------------------------------");
    }

    if (GetVerbosityLevel() >= 2)
    {
        RCLog("sourceFullFileName: '%s'", sourceFullFileName.c_str());
        RCLog("outputFolder: '%s'", outputFolder.c_str());
        RCLog("Path='%s'", PathHelpers::CanonicalizePath(sourceInnerPath).c_str());
        RCLog("File='%s'", PathHelpers::GetFilename(sourceFullFileName).c_str());
    }
    else if (GetVerbosityLevel() > 0)
    {
        RCLog("Path='%s'", PathHelpers::CanonicalizePath(sourceInnerPath).c_str());
        RCLog("File='%s'", PathHelpers::GetFilename(sourceFullFileName).c_str());
    }
    else if (GetVerbosityLevel() == 0)
    {
        const string sourceInnerPathAndName = PathHelpers::AddSeparator(sourceInnerPath) + PathHelpers::GetFilename(sourceFullFileName);
        RCLog("File='%s'", sourceInnerPathAndName.c_str());
    }

    // file name changed - print new header for warnings and errors
    compileFileInfo.bWarningHeaderLine = false;
    compileFileInfo.bErrorHeaderLine = false;
    compileFileInfo.logHeaderLine = sourceFullFileName;

    bool bRet;
    bool createJobs = this->GetMultiplatformConfig().getConfig().GetAsString("createjobs", "", "").empty() == false;

    if (createJobs)
    {
        bRet = compiler->CreateJobs();

        if (!bRet)
        {
            RCLogError("Failed to create jobs for file %s", sourceFullFileName.c_str());
        }
    }
    else
    {
        // Compile file(s)
        bRet = compiler->Process();

        if (!bRet)
        {
            RCLogError("Failed to convert file %s", sourceFullFileName.c_str());
        }
    }

    return bRet;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::AddInputOutputFilePair(const char* inputFilename, const char* outputFilename)
{
    AZ_Assert(outputFilename && outputFilename[0], "assert");
    AZ_Assert(inputFilename && inputFilename[0], "assert");

    AZStd::lock_guard<AZStd::mutex> lock(m_inputOutputFilesLock);

    m_inputOutputFileList.Add(inputFilename, outputFilename);
}

void ResourceCompiler::MarkOutputFileForRemoval(const char* outputFilename)
{
    AZ_Assert(outputFilename && outputFilename[0], "assert");

    AZStd::lock_guard<AZStd::mutex> lock(m_inputOutputFilesLock);

    // using empty input file name will force CleanTargetFolder(false) to delete the output file
    m_inputOutputFileList.Add("", outputFilename);

    if (GetVerbosityLevel() > 0)
    {
        RCLog("Marking file for removal: %s", outputFilename);
    }
}

void ResourceCompiler::LogLine(const IRCLog::EType eType, const char* szText)
{
    AZStd::lock_guard<AZStd::mutex> lock(m_logLock);

    if (eType == IRCLog::eType_Warning)
    {
        ++m_numWarnings;
    }
    else if (eType == IRCLog::eType_Error)
    {
        ++m_numErrors;
    }

    FILE* fLog = 0;
    if (!m_mainLogFileName.empty())
    {
        azfopen(&fLog, m_mainLogFileName.c_str(), "a+t");
    }


    if (m_bQuiet)
    {
        if (fLog)
        {
            fprintf(fLog, "%s\n", szText);
            fclose(fLog);
        }
        return;
    }

    char timeString[20];
    timeString[0] = 0;
    if (m_bTimeLogging)
    {
        const int seconds = int(float((clock() - m_startTime) / CLOCKS_PER_SEC) + 0.5f);
        const int minutes = seconds / 60;
        sprintf_s(timeString, "%2d:%02d ", minutes, seconds - (minutes * 60));
    }

    const char* prefix = "";
    const char* additionalLogFileName = 0;
    bool* pbAdditionalLogHeaderLine = 0;

    switch (eType)
    {
    case IRCLog::eType_Info:
        prefix = "   ";
        break;

    case IRCLog::eType_Warning:
        prefix = "W: ";
        if (!m_warningLogFileName.empty())
        {
            additionalLogFileName = m_warningLogFileName.c_str();
            if (m_currentRcCompileFileInfo)
            {
                pbAdditionalLogHeaderLine = &m_currentRcCompileFileInfo->bWarningHeaderLine;
            }
        }
        break;

    case IRCLog::eType_Error:
        prefix = "E: ";
        if (!m_errorLogFileName.empty())
        {
            additionalLogFileName = m_errorLogFileName.c_str();
            if (m_currentRcCompileFileInfo)
            {
                pbAdditionalLogHeaderLine = &m_currentRcCompileFileInfo->bErrorHeaderLine;
            }
        }
        break;

    case IRCLog::eType_Context:
        prefix = "C: ";
        break;

    case IRCLog::eType_Summary:
        prefix = "S: ";
        break;

    default:
        AZ_Assert(false, "assert");
        break;
    }

    FILE* fAdditionalLog = 0;
    if (additionalLogFileName)
    {
        azfopen(&fAdditionalLog, additionalLogFileName, "a+t");
    }

    if (fAdditionalLog)
    {
        if (pbAdditionalLogHeaderLine && (*pbAdditionalLogHeaderLine == false))
        {
            fprintf(fAdditionalLog, "------------------------------------\n");
            fprintf(fAdditionalLog, "%s%s%s\n", prefix, timeString, m_currentRcCompileFileInfo->logHeaderLine.c_str());
            *pbAdditionalLogHeaderLine = true;
        }
    }

    const char* line = szText;
    for (;; )
    {
        size_t lineLen = 0;
        while (line[lineLen] && line[lineLen] != '\n')
        {
            ++lineLen;
        }

        if (fAdditionalLog)
        {
            fprintf(fAdditionalLog, "%s%s%.*s\n", prefix, timeString, int(lineLen), line);
        }

        if (fLog)
        {
            fprintf(fLog, "%s%s%.*s\n", prefix, timeString, int(lineLen), line);
        }

        printf("%s%s%.*s\n", prefix, timeString, int(lineLen), line);
        fflush(stdout);

        line += lineLen;
        if (*line == '\0')
        {
            break;
        }
        ++line;
    }

    if (fAdditionalLog)
    {
        fclose(fAdditionalLog);
        fAdditionalLog = 0;
    }

    if (fLog)
    {
        fclose(fLog);
        fLog = 0;
    }

    if (m_bWarningsAsErrors && (eType == IRCLog::eType_Warning || eType == IRCLog::eType_Error))
    {
#if defined(AZ_PLATFORM_WINDOWS)
        ::MessageBoxA(NULL, szText, "RC Compilation Error", MB_OK | MB_ICONERROR);
#else
        //TODO:Implement alert box functionality with an 'ok' button for other platforms
#endif
        this->NotifyExitObservers();
        exit(eRcExitCode_Error);
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::LogMultiLine(const char* szText)
{
    const char* p = szText;
    char szLine[80], * pLine = szLine;

    for (;; )
    {
        if (*p == '\n' || *p == 0 || (pLine - szLine) >= sizeof(szLine) - (5 + 2 + 1)) // 5 spaces +2 (W: or E:) +1 to avoid nextline jump
        {
            *pLine = 0;                                                     // zero termination
            RCLog("     %s", szLine);                                      // 5 spaces
            pLine = szLine;

            if (*p == '\n')
            {
                ++p;
                continue;
            }
        }

        if (*p == 0)
        {
            return;
        }

        *pLine++ = *p++;
    }
    while (*p)
    {
        ;
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::ShowHelp(const bool bDetailed)
{
    RCLog("");
    RCLog("Usage: RC filespec --platform=<platform> [--Key1=Value1] [--Key2=Value2] etc...");

    if (bDetailed)
    {
        RCLog("");

        std::map<string, string>::const_iterator it, end = m_KeyHelp.end();

        for (it = m_KeyHelp.begin(); it != end; ++it)
        {
            const string& rKey = it->first;
            const string& rHelp = it->second;

            // hide internal keys (keys starting from '_')
            if (!rKey.empty() && rKey[0] != '_')
            {
                RCLog("/%s", rKey.c_str());
                LogMultiLine(rHelp.c_str());
                RCLog("");
            }
        }
    }
    else
    {
        RCLog("       RC /help             // will list all usable keys with description");
        RCLog("       RC /help >file.txt   // output help text to file.txt");
        RCLog("");
    }
}

void ResourceCompiler::AddPluginDLL(HMODULE pluginDLL)
{
    if (pluginDLL)
    {
        m_loadedPlugins.push_back(pluginDLL);
    }
}

void ResourceCompiler::RemovePluginDLL(HMODULE pluginDLL)
{
    if (pluginDLL)
    {
        stl::find_and_erase_if(m_loadedPlugins, [pluginDLL](const HMODULE moduleToCheck) { return moduleToCheck == pluginDLL; });
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::InitPakManager()
{
    delete m_pPakManager;

    m_pPakManager = new PakManager(this);
    m_pPakManager->RegisterKeys(this);
}

string ResourceCompiler::GetResourceCompilerGenericInfo(const string& newline) const
{
    string s;
    const SFileVersion& v = GetFileVersion();

#if defined(_WIN64)
    s += "ResourceCompiler  64-bit";
#else
    s += "ResourceCompiler  32-bit";
#endif
#if defined(_DEBUG)
    s += "  DEBUG";
#endif
    s += newline;

    s += "Platform support: PC";
#if defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
    s += ", " #PublicAuxName3;
    AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
#if defined(TOOLS_SUPPORT_POWERVR)
    s += ", PowerVR";
#endif
#if defined(TOOLS_SUPPORT_ETC2COMP)
    s += ", etc2Comp";
#endif
    s += newline;

    s += StringHelpers::Format("Version %d.%d.%d.%d  %s %s", v.v[3], v.v[2], v.v[1], v.v[0], __DATE__, __TIME__);
    s += newline;

    s += newline;

    s += "Portions of this file Copyright (c) Amazon.com, Inc. or its affiliates. All Rights Reserved. Original file Copyright (c) Crytek GMBH. Used under license by Amazon.com, Inc. and its affiliates.";
    s += newline;

    s += newline;

    s += "Exe directory:";
    s += newline;
    s += StringHelpers::Format("  \"%s\"", GetExePath());
    s += newline;
    s += "Temp directory:";
    s += newline;
    s += StringHelpers::Format("  \"%s\"", GetTmpPath());
    s += newline;
    s += "Current directory:";
    s += newline;
    s += StringHelpers::Format("  \"%s\"", GetInitialCurrentDir());
    s += newline;

    return s;
}


const void ResourceCompiler::GetGenericInfo(
    char* buffer,
    size_t bufferSize,
    const char* rowSeparator) const
{
    if (bufferSize <= 0 || buffer == 0)
    {
        return;
    }
    const string s = GetResourceCompilerGenericInfo(string(rowSeparator));
    cry_strcpy(buffer, bufferSize, s.c_str());
}


void ResourceCompiler::CopyStringToClipboard(const string& s)
{
#if defined(AZ_PLATFORM_WINDOWS)
    if (OpenClipboard(NULL))
    {
        const HGLOBAL hAllocResult = GlobalAlloc(GMEM_MOVEABLE, s.length() + 1);
        if (hAllocResult)
        {
            const LPTSTR lptstrCopy = (LPTSTR)GlobalLock(hAllocResult);
            memcpy(lptstrCopy, s.c_str(), s.length() + 1);
            GlobalUnlock(hAllocResult);

            EmptyClipboard();

            if (::SetClipboardData(CF_TEXT, hAllocResult) == NULL)
            {
                GlobalFree(hAllocResult);
            }
        }
        CloseClipboard();
    }
#endif
}


bool ResourceCompiler::CheckCommandLineOptions(const IConfig& config, const std::vector<string>* keysToIgnore)
{
    std::vector<string> unknownKeys;
    config.GetUnknownKeys(unknownKeys);

    if (keysToIgnore)
    {
        for (size_t i = 0; i < unknownKeys.size(); ++i)
        {
            if (std::find(keysToIgnore->begin(), keysToIgnore->end(), StringHelpers::MakeLowerCase(unknownKeys[i])) != keysToIgnore->end())
            {
                unknownKeys.erase(unknownKeys.begin() + i);
                --i;
            }
        }
    }

    if (!unknownKeys.empty())
    {
        RCLogWarning("Unknown command-line options (use \"RC /help\"):");

        for (size_t i = 0; i < unknownKeys.size(); ++i)
        {
            RCLogWarning("    /%s", unknownKeys[i].c_str());
        }

        if (config.GetAsBool("failonwarnings", false, true))
        {
            return false;
        }
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::PostBuild()
{
    const IConfig* const config = &m_multiConfig.getConfig();

    // Save list of created files.
    {
        m_inputOutputFileList.RemoveDuplicates();
        m_inputOutputFileList.SaveOutputOnly(FormLogFileName(m_filenameCreatedFileList));
    }

    switch (config->GetAsInt("clean_targetroot", 0, 1))
    {
    case 1:
        CleanTargetFolder(false);
        break;
    case 2:
        CleanTargetFolder(true);
        break;
    default:
        break;
    }

    const string dependenciesFilename = m_multiConfig.getConfig().GetAsString("dependencies", "", "");
    if (!dependenciesFilename.empty())
    {
        m_inputOutputFileList.RemoveDuplicates();
        m_inputOutputFileList.Save(dependenciesFilename.c_str());
    }
}
void ResourceCompiler::SetAppRootPath(const string& appRootPath)
{
    m_appRoot = appRootPath;
}

string ResourceCompiler::GetAppRootPathFromGameRoot(const string& gameRootPath)
{
    // Since we cannot rely on the current exe path or engine path to determine the app root path since
    // the RC request could be coming in from an external game, we need to calculate it from the game root path.
    // Typically, the game root path is one level above the app root (the app root and the engine path is the same
    // if the game project resides in the engine folder).  If its external, we need the app root for that external
    // project in order to negotiate communications back to the AP with the right branch token
    //
    // Example:
    // Game root path        :   /LyEngine/MyGame/
    // Derived app root path :   /LyEngine

    size_t index = gameRootPath.length();
    if (index == 0)
    {
        // Empty string, skip
        return "";
    }

    char lastChar = gameRootPath.at(--index);
    // Skip over any initial path delimiters or spaces (Reduce all trailing '/' for example:  c:\foo\bar\\\ -> c:\foo\bar)
    while (((lastChar == AZ_CORRECT_FILESYSTEM_SEPARATOR) || (lastChar == AZ_WRONG_FILESYSTEM_SEPARATOR) || (lastChar == ' ')) && index>0)
    {
        lastChar = gameRootPath.at(--index);
    }
    // Invalid path (may have been '\\\\\\\\')
    if (index == 0)
    {
        return "";
    }

    // Go to the next path delimeter
    while (((lastChar != AZ_CORRECT_FILESYSTEM_SEPARATOR) && (lastChar != AZ_WRONG_FILESYSTEM_SEPARATOR)) && index>0)
    {
        lastChar = gameRootPath.at(--index);
    }

    // Invalid path (no path delimiter found after the first group, ie:  c:root\ would not resolve a parent )
    if (index == 0)
    {
        return "";
    }
    return gameRootPath.substr(0, index);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::QueryVersionInfo()
{

    char moduleName[AZ_MAX_PATH_LEN] = {'\0'};

    AZ::Utils::GetExecutablePathReturnType executablePathResult =  AZ::Utils::GetExecutablePath(moduleName,AZ_ARRAY_SIZE(moduleName));
    switch (executablePathResult.m_pathStored)
    {
        case AZ::Utils::ExecutablePathResult::BufferSizeNotLargeEnough:
            printf("RC QueryVersionInfo(): Buffer size not large enough to store module path");
            exit(eRcExitCode_FatalError);

        case AZ::Utils::ExecutablePathResult::GeneralError:
            printf("RC QueryVersionInfo(): fatal error");
            exit(eRcExitCode_FatalError);

        case AZ::Utils::ExecutablePathResult::Success:
            m_exePath = moduleName;
            break;

        default:
            printf("RC QueryVersionInfo(): unknown error");
            exit(eRcExitCode_FatalError);
    }

    if (m_exePath.empty())
    {
        printf("RC module name: fatal error");
        exit(eRcExitCode_FatalError);
    }
    m_exePath = PathHelpers::AddSeparator(PathHelpers::GetDirectory(m_exePath));

#if defined(AZ_PLATFORM_WINDOWS)

    DWORD handle;
    char ver[1024 * 8];
    const int verSize = GetFileVersionInfoSizeA(moduleName, &handle);

    if (verSize > 0 && verSize <= sizeof(ver))
    {
        GetFileVersionInfoA(moduleName, 0, sizeof(ver), ver);
        VS_FIXEDFILEINFO* vinfo;
        UINT len;
        VerQueryValue(ver, "\\", (void**)&vinfo, &len);

        m_fileVersion.v[0] = vinfo->dwFileVersionLS & 0xFFFF;
        m_fileVersion.v[1] = vinfo->dwFileVersionLS >> 16;
        m_fileVersion.v[2] = vinfo->dwFileVersionMS & 0xFFFF;
        m_fileVersion.v[3] = vinfo->dwFileVersionMS >> 16;
    }
#endif
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::InitPaths()
{
    if (m_exePath.empty())
    {
        printf("RC InitPaths(): internal error");
        exit(eRcExitCode_FatalError);
    }
#if defined(AZ_PLATFORM_WINDOWS)
    // Get wide-character temp path from OS
    {
        static const DWORD bufferSize = AZ_MAX_PATH_LEN;
        wchar_t bufferWchars[bufferSize];

        DWORD resultLength = GetTempPathW(bufferSize, bufferWchars);
        if ((resultLength >= bufferSize) || (resultLength <= 0))
        {
            resultLength = 0;
        }
        bufferWchars[resultLength] = 0;

        m_tempPath = PathHelpers::GetAbsoluteAsciiPath(bufferWchars);
        if (m_tempPath.empty())
        {
            m_tempPath = m_exePath;
        }
        m_tempPath = PathHelpers::AddSeparator(m_tempPath);
    }
#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
    //The path supplied by the first environment variable found in the
    //list TMPDIR, TMP, TEMP, TEMPDIR. If none of these are found, "/tmp".
    //Another possibility is using QTemporaryDir for this work.
    char const *tmpDir = getenv("TMPDIR");
    if (tmpDir == 0)
    {
        tmpDir = getenv("TMP");
        if (tmpDir == 0)
        {
            tmpDir = getenv("TEMP");
            if (tmpDir == 0)
            {
                tmpDir = getenv("TEMPDIR");
                if (tmpDir == 0)
                {
                    tmpDir = "/tmp";
                }
            }
        }
    }
    m_tempPath = tmpDir;
#else
    #error Needs implementation!
#endif
    m_initialCurrentDir = PathHelpers::GetAbsoluteAsciiPath(".");
    if (m_initialCurrentDir.empty())
    {
        printf("RC InitPaths(): internal error");
        exit(eRcExitCode_FatalError);
    }
    m_initialCurrentDir = PathHelpers::AddSeparator(m_initialCurrentDir);

    // Prepend one level up from rc.exe to the path, so child dlls
    // can find engine or dependency dlls. -> Prepend so that our directory gets searched first!
#if defined(AZ_PLATFORM_WINDOWS)
    char pathEnv[s_environmentBufferSize] = {'\0'};
    GetEnvironmentVariable("PATH", pathEnv, s_environmentBufferSize);

    string pathEnvNew = m_exePath;
    pathEnvNew.append("..\\;");
    pathEnvNew.append(pathEnv);

    SetEnvironmentVariable("PATH", pathEnvNew.c_str());

#elif AZ_TRAIT_OS_PLATFORM_APPLE || defined(AZ_PLATFORM_LINUX)
    const char * pathEnv = getenv("PATH");
    string pathEnvNew = pathEnv;
    pathEnvNew.append(":");
    pathEnvNew.append(m_exePath);
    pathEnvNew.append("../");
    setenv("PATH", pathEnvNew.c_str(), 1);
#endif
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::LoadIniFile()
{
#if defined(AZ_PLATFORM_WINDOWS)
    const string filename = PathHelpers::ToDosPath(m_exePath) + m_filenameRcIni;
#elif defined(AZ_PLATFORM_MAC)
    // Handle the case where RC is inside an App Bundle (for the Editor or AssetProcessor)
    string filename = PathHelpers::ToUnixPath(m_exePath);
    string::size_type appBundleDirPosition = filename.rfind(".app");
    if (appBundleDirPosition != string::npos)
    {
        // ini file should be in the Resources directory. Doing the substr cuts
        // off the .app part so add it back in as well.
        filename = filename.substr(0, appBundleDirPosition);
        filename += ".app/Contents/Resources/";
    }
    filename = filename + m_filenameRcIni;
#else
    const string filename = PathHelpers::ToUnixPath(m_exePath) + m_filenameRcIni;
#endif

    RCLog("Loading \"%s\"", filename.c_str());

    if (!FileUtil::FileExists(filename.c_str()))
    {
        RCLogError("Resource compiler .ini file (%s) is missing.", filename.c_str());
        return false;
    }

    if (!m_iniFile.Load(filename))
    {
        RCLogError("Failed to load resource compiler .ini file (%s).", filename.c_str());
        return false;
    }

    RCLog("  Loaded \"%s\"", filename.c_str());
    RCLog("");

    return true;
}


//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::Init(Config& config)
{
    m_bQuiet = config.GetAsBool("quiet", false, true);
    m_verbosityLevel = config.GetAsInt("verbose", 0, 1);

    m_bWarningsAsErrors = config.GetAsBool("wx", false, true);

    m_startTime = clock();
    m_bTimeLogging = false;

    m_bUseFastestDecompressionCodec = config.GetAsBool("use_fastest", false,false);

    InitLogs(config);
    SetRCLog(this);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::UnregisterConvertors()
{
    m_extensionManager.UnregisterAll();

    // let the before unload functions for all DLLs complete before you unload any of them
    for (HMODULE pluginDll : m_loadedPlugins)
    {
        CryFreeLibrary(pluginDll);
    }

    m_loadedPlugins.resize(0);
}

//////////////////////////////////////////////////////////////////////////
MultiplatformConfig& ResourceCompiler::GetMultiplatformConfig()
{
    return m_multiConfig;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::SetComplilingFileInfo(RcCompileFileInfo* compileFileInfo)
{
    m_currentRcCompileFileInfo = compileFileInfo;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::VerifyKeyRegistration(const char* szKey) const
{
    AZ_Assert(szKey, "assert");
    const string sKey = StringHelpers::MakeLowerCase(string(szKey));

    if (m_KeyHelp.count(sKey) == 0)
    {
        RCLogWarning("Key '%s' was not registered, call RegisterKey() before using the key", szKey);
    }
}

//////////////////////////////////////////////////////////////////////////
bool ResourceCompiler::HasKeyRegistered(const char* szKey) const
{
    AZ_Assert(szKey, "assert");
    const string sKey = StringHelpers::MakeLowerCase(string(szKey));

    return (m_KeyHelp.count(sKey) != 0);
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::RegisterKey(const char* key, const char* helptxt)
{
    const string sKey = StringHelpers::MakeLowerCase(string(key));

    AZ_Assert(m_KeyHelp.count(sKey) == 0, "assert");       // registered twice

    m_KeyHelp[sKey] = helptxt;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::InitLogs(Config& config)
{
    m_logPrefix.clear();
    m_logPrefix = config.GetAsString("logprefix", "", "");
    if (m_logPrefix.empty())
    {
        m_logPrefix = m_exePath;
    }

    {
        const string logDir = PathHelpers::GetDirectory(m_logPrefix + "unused.name");
        if (!FileUtil::EnsureDirectoryExists(logDir.c_str()))
        {
            RCLogError("Creating directory failed: %s", logDir.c_str());
        }
    }

    m_mainLogFileName    = FormLogFileName(m_filenameLog);
    m_warningLogFileName = FormLogFileName(m_filenameLogWarnings);
    m_errorLogFileName   = FormLogFileName(m_filenameLogErrors);

    AZ::IO::LocalFileIO().Remove(m_mainLogFileName);
    AZ::IO::LocalFileIO().Remove(m_warningLogFileName);
    AZ::IO::LocalFileIO().Remove(m_errorLogFileName);

    //if logfiles is false, disable logging by setting the m_mainLofFileName to empty string
    if (config.GetAsBool("logfiles", false, true))
    {
        m_mainLogFileName = "";
    }
}

string ResourceCompiler::FormLogFileName(const char* suffix) const
{
    return (suffix && suffix[0]) ? m_logPrefix + suffix : string();
}

const string& ResourceCompiler::GetMainLogFileName() const
{
    return m_mainLogFileName;
}

const string& ResourceCompiler::GetErrorLogFileName() const
{
    return m_errorLogFileName;
}

//////////////////////////////////////////////////////////////////////////
clock_t ResourceCompiler::GetStartTime() const
{
    return m_startTime;
}

bool ResourceCompiler::GetTimeLogging() const
{
    return m_bTimeLogging;
}

void ResourceCompiler::SetTimeLogging(bool enable)
{
    m_bTimeLogging = enable;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::AddExitObserver(IResourceCompiler::IExitObserver* p)
{
    if (p == 0)
    {
        return;
    }

    AZStd::lock_guard<AZStd::mutex> lock(m_exitObserversLock);

    m_exitObservers.push_back(p);
}

void ResourceCompiler::RemoveExitObserver(IResourceCompiler::IExitObserver* p)
{
    if (p == 0)
    {
        return;
    }

    AZStd::lock_guard<AZStd::mutex> lock(m_exitObserversLock);

    for (size_t i = 0; i < m_exitObservers.size(); ++i)
    {
        if (m_exitObservers[i] == p)
        {
            m_exitObservers.erase(m_exitObservers.begin() + i);
            return;
        }
    }
}

void ResourceCompiler::NotifyExitObservers()
{
    AZStd::lock_guard<AZStd::mutex> lock(m_exitObserversLock);

    for (size_t i = 0; i < m_exitObservers.size(); ++i)
    {
        m_exitObservers[i]->OnExit();
    }

    m_exitObservers.clear();
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::LogV(const IRCLog::EType eType, const char* szFormat, va_list args)
{
    char str[s_internalBufferSize];
    azvsnprintf(str, s_internalBufferSize, szFormat, args);
    str[s_internalBufferSize - 1] = 0;

    // remove non-printable characters except newlines and tabs
    char* p = str;
    while (*p)
    {
        if ((*p < ' ') && (*p != '\n') && (*p != '\t'))
        {
            *p = ' ';
        }
        ++p;
    }

    LogLine(eType, str);
}

void ResourceCompiler::Log(const IRCLog::EType eType, const char* szMessage)
{
    char str[s_internalBufferSize];
    azstrcpy(str, s_internalBufferSize, szMessage);
    str[s_internalBufferSize - 1] = 0;

    // remove non-printable characters except newlines and tabs
    char* p = str;
    while (*p)
    {
        if ((*p < ' ') && (*p != '\n') && (*p != '\t'))
        {
            *p = ' ';
        }
        ++p;
    }

    LogLine(eType, str);
}

namespace
{
    struct CheckEmptySourceInnerPathAndName
    {
        bool operator()(const RcFile& file) const
        {
            return file.m_sourceInnerPathAndName.empty();
        }
    };
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::FilterExcludedFiles(std::vector<RcFile>& files)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    const bool bVerbose = GetVerbosityLevel() > 1;

    std::vector<string> excludes;
    {
        string excludeStr = config->GetAsString("exclude", "", "", eCP_PriorityAll & ~eCP_PriorityJob);
        if (!excludeStr.empty())
        {
            excludeStr = PathHelpers::ToPlatformPath(excludeStr);
            StringHelpers::Split(excludeStr, ";", false, excludes);
        }

        excludeStr = config->GetAsString("exclude", "", "", eCP_PriorityJob);
        if (!excludeStr.empty())
        {
            excludeStr = PathHelpers::ToPlatformPath(excludeStr);
            StringHelpers::Split(excludeStr, ";", false, excludes);
        }

        if (bVerbose)
        {
            excludeStr.clear();
            for (size_t i = 0; i < excludes.size(); ++i)
            {
                if (i > 0)
                {
                    excludeStr += ";";
                }
                excludeStr += excludes[i];
            }
            RCLog("   Exclude: %s", excludeStr.c_str());
        }
    }

    std::set<const char*, stl::less_stricmp<const char*> > excludedFiles;
    std::vector<std::pair<string, string> > excludedStrings;
    const string excludeListFile = config->GetAsString("exclude_listfile", "", "");
    if (!excludeListFile.empty())
    {
        const string listFormat = config->GetAsString("listformat", "", "");
        CListFile(this).Process(excludeListFile, listFormat, "*", string(), excludedStrings);
        const size_t filenameCount = excludedStrings.size();
        for (size_t i = 0; i < filenameCount; ++i)
        {
            string& name = excludedStrings[i].second;
            name = PathHelpers::ToPlatformPath(name);
            excludedFiles.insert(name.c_str());
        }
    }

    if (excludes.empty() && excludedFiles.empty())
    {
        return;
    }

    string name;
    for (size_t nFile = 0; nFile < files.size(); ++nFile)
    {
        name = files[nFile].m_sourceInnerPathAndName;
        name = PathHelpers::ToPlatformPath(name);

        if (excludedFiles.find(name.c_str()) != excludedFiles.end())
        {
            if (bVerbose)
            {
                RCLog("    Excluding file %s by %s", name.c_str(), excludeListFile.c_str());
            }
            files[nFile].m_sourceInnerPathAndName.clear();
            continue;
        }

        for (size_t i = 0; i < excludes.size(); ++i)
        {
            if (StringHelpers::MatchesWildcardsIgnoreCase(name, excludes[i]))
            {
                if (bVerbose)
                {
                    RCLog("    Excluding file %s by %s", name.c_str(), excludes[i].c_str());
                }
                files[nFile].m_sourceInnerPathAndName.clear();
                break;
            }
        }
    }

    const size_t sizeBefore = files.size();
    files.erase(std::remove_if(files.begin(), files.end(), CheckEmptySourceInnerPathAndName()), files.end());

    RCLog("Files excluded: %i", sizeBefore - files.size());
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::GetFileListRecursively(const ZipDir::FileEntryTree* directory, const AZStd::string& directoryName, AZStd::vector<AZStd::string>& filenames) const
{
    //Get all the files in the current directory.
    ZipDir::FileEntryTree::FileMap::const_iterator iterFile = directory->GetFileBegin();
    while (iterFile != directory->GetFileEnd())
    {
        AZStd::string fullFileName = directoryName.c_str() + AZStd::string(iterFile->first);
        filenames.push_back(fullFileName);
        ++iterFile;
    }

    //Loop through all the directories and recurse all the way to the end.
    ZipDir::FileEntryTree::SubdirMap::const_iterator iterDir = directory->GetDirBegin();
    while (iterDir != directory->GetDirEnd())
    {
        const ZipDir::FileEntryTree* subDirectory = iterDir->second;
        AZStd::string fullDirName;
        if (directoryName.empty())
        {
            fullDirName = iterDir->first;
        }
        else
        {
            fullDirName = directoryName + AZStd::string(iterDir->first);
        }
        AzFramework::StringFunc::Path::AppendSeparator(fullDirName);
        GetFileListRecursively(subDirectory, fullDirName, filenames);
        ++iterDir;
    }
}

bool ResourceCompiler::RecompressFiles(const string& sourceFileName, const string& destinationFileName)
{
    RCLog("Recompressing %s to %s", sourceFileName.c_str(), destinationFileName.c_str());

    AZ::IO::FileIOBase* pFileIO = AZ::IO::FileIOBase::GetInstance();
    if (pFileIO->Exists(destinationFileName.c_str()))
    {
        AZ::IO::Result removeResult = pFileIO->Remove(destinationFileName.c_str());
        if (removeResult.GetResultCode() != AZ::IO::ResultCode::Success)
        {
            RCLog("Recompression failed because Failed to remove %s", destinationFileName.c_str());
            return false;
        }
    }

    IPakSystem* pakSystem = GetPakSystem();
    AZ_Assert(pakSystem != nullptr, "Invalid IPakSystem in RecompressFiles");

    PakSystemArchive* pSourcePAK = pakSystem->OpenArchive(sourceFileName.c_str());
    PakSystemArchive* pDestinationPAK = pakSystem->OpenArchive(destinationFileName.c_str());

    AZStd::vector<AZStd::string> filesInPAK;
    if (pSourcePAK && pDestinationPAK)
    {
        RCLog("Opened PAK...");

        GetFileListRecursively(pSourcePAK->zip->GetRoot(), "", filesInPAK);
        RCLog("Got %d files from PAK for recompression", filesInPAK.size());

        bool success = true;
        for (AZStd::string& fileInsidePak : filesInPAK)
        {
            ZipDir::FileEntry* fileEntry = pSourcePAK->zip->FindFile(fileInsidePak.c_str());
            unsigned int fileSizeCompressed = fileEntry->desc.lSizeCompressed;
            unsigned int fileSizeUncompressed = fileEntry->desc.lSizeUncompressed;

            char* bufferCompressed = new char[fileSizeCompressed];
            char* bufferUncompressed = new char[fileSizeUncompressed];
            ZipDir::ErrorEnum readResult = pSourcePAK->zip->ReadFile(fileEntry, bufferCompressed, bufferUncompressed);
            delete[] bufferCompressed; //Not needed anymore.
            if (readResult != ZipDir::ZD_ERROR_SUCCESS)
            {
                success = false;
                delete[] bufferUncompressed;
                break;// Exit the for loop.
            }
            pDestinationPAK->zip->UpdateFile(fileInsidePak.c_str(), bufferUncompressed, fileSizeUncompressed, ZipFile::METHOD_DEFLATE, 1, fileEntry->GetModificationTime());
            delete[] bufferUncompressed;
        }

        pakSystem->CloseArchive(pSourcePAK);
        pakSystem->CloseArchive(pDestinationPAK);

        RCLog("Recompression completed %s", success ? "successfully" : "unsuccessfully");

        _flushall();
        return success;
    }
    return false;
}

//////////////////////////////////////////////////////////////////////////

void ResourceCompiler::CopyFiles(const std::vector<RcFile>& files, bool bNoOverwrite, bool recompress)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    const bool bSkipMissing = config->GetAsBool("skipmissing", false, true);
    const int srcMaxSize = config->GetAsInt("sourcemaxsize", -1, -1);

    const size_t numFiles = files.size();
    size_t numFilesCopied = 0;
    size_t numFilesUpToDate = 0;
    size_t numFilesSkipped = 0;
    size_t numFilesMissing = 0;
    size_t numFilesFailed = 0;

    RCLog("Starting copying %" PRISIZE_T " files", numFiles);

    string progressString;
    progressString.Format("Copying % " PRISIZE_T " files", numFiles);
    StartProgress();

    const bool bRefresh = config->GetAsBool("refresh", false, true);

    NameConvertor nc;
    if (!nc.SetRules(config->GetAsString("targetnameformat", "", "")))
    {
        return;
    }

    for (size_t i = 0; i < numFiles; ++i)
    {
        if (g_gotCTRLBreakSignalFromOS)
        {
            // abort!
            return;
        }

        ShowProgress(progressString.c_str(), i, numFiles);

        string srcFilename = PathHelpers::Join(files[i].m_sourceLeftPath, files[i].m_sourceInnerPathAndName);
        string trgFilename = PathHelpers::Join(files[i].m_targetLeftPath, files[i].m_sourceInnerPathAndName);
        if (nc.HasRules())
        {
            const string oldFilename = PathHelpers::GetFilename(trgFilename);
            const string newFilename = nc.GetConvertedName(oldFilename);
            if (newFilename.empty())
            {
                return;
            }
            if (!StringHelpers::EqualsIgnoreCase(oldFilename, newFilename))
            {
                if (GetVerbosityLevel() >= 2)
                {
                    RCLog("Target file name changed: %s -> %s", oldFilename.c_str(), newFilename.c_str());
                }
                trgFilename = PathHelpers::Join(PathHelpers::GetDirectory(trgFilename), newFilename);
            }
        }

        if (GetVerbosityLevel() > 1)
        {
            RCLog("Copying %s to %s", srcFilename.c_str(), trgFilename.c_str());
        }

        const bool bSourceFileExists = FileUtil::FileExists(srcFilename.c_str());
        const bool bTargetFileExists = FileUtil::FileExists(trgFilename.c_str());

        if (!bSourceFileExists)
        {
            ++numFilesMissing;
            if (!bSkipMissing)
            {
                RCLog("Source file %s does not exist", srcFilename.c_str());
            }
            continue;
        }
        else
        {
            if (!FileUtil::EnsureDirectoryExists(PathHelpers::GetDirectory(trgFilename).c_str()))
            {
                RCLog("Failed creating directory for %s", trgFilename.c_str());
                ++numFilesFailed;
                RCLog("Failed to copy %s to %s", srcFilename.c_str(), trgFilename.c_str());
                continue;
            }

            //////////////////////////////////////////////////////////////////////////
            // Compare source and target files modify timestamps.
            if (bTargetFileExists && FileUtil::FileTimesAreEqual(srcFilename, trgFilename) && !bRefresh)
            {
                // Up to date file already exists in target folder
                ++numFilesUpToDate;
                AddInputOutputFilePair(srcFilename, trgFilename);
                continue;
            }
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // no overwrite
            if (bTargetFileExists && bNoOverwrite && !bRefresh)
            {
                // Up to date file already exists in target folder
                ++numFilesUpToDate;
                AddInputOutputFilePair(srcFilename, trgFilename);
                continue;
            }
            //////////////////////////////////////////////////////////////////////////

            if (srcMaxSize >= 0)
            {
                const int64 fileSize = FileUtil::GetFileSize(srcFilename);
                if (fileSize > srcMaxSize)
                {
                    ++numFilesSkipped;
                    RCLog("Source file %s is bigger than %d bytes (size is %" PRId64 " ). Skipped.", srcFilename.c_str(), srcMaxSize, fileSize);
                    AddInputOutputFilePair(srcFilename, trgFilename);
                    continue;
                }
            }
#if defined(AZ_PLATFORM_WINDOWS)
            SetFileAttributes(trgFilename, FILE_ATTRIBUTE_ARCHIVE);
#endif
            bool bCopied = false;
            bool didAttemptRecompress = false;
            if (recompress)
            {
                //recompressing only makes sense when source is a PAK file.
                AZStd::string srcExtension;
                if (AzFramework::StringFunc::Path::GetExtension(srcFilename.c_str(), srcExtension, false))
                {
                    //Case Insensitive compare
                    if (AzFramework::StringFunc::Equal(srcExtension.c_str(), "pak"))
                    {
                        didAttemptRecompress = true;
                        AZ::IO::LocalFileIO localFileIO;
                        char szFullPathDir[AZ_MAX_PATH_LEN];
                        if (localFileIO.ConvertToAbsolutePath(trgFilename, szFullPathDir, sizeof(szFullPathDir)))
                        {
                            bCopied = RecompressFiles(srcFilename, szFullPathDir);
                        }
                    }
                }
            }

            if (!didAttemptRecompress)
            {
                AZ::IO::LocalFileIO localFileIO;
                bCopied = (localFileIO.Copy(srcFilename, trgFilename) != 0);
            }

            if (bCopied)
            {
                ++numFilesCopied;
#if defined(AZ_PLATFORM_WINDOWS)
                SetFileAttributes(trgFilename, FILE_ATTRIBUTE_ARCHIVE);
#endif
                FileUtil::SetFileTimes(srcFilename, trgFilename);
            }
            else
            {
                ++numFilesFailed;
                RCLog("Failed to copy %s to %s", srcFilename.c_str(), trgFilename.c_str());
            }
        }

        AddInputOutputFilePair(srcFilename, trgFilename);
    }

    RCLog("Finished copying %" PRISIZE_T " files: "
        "%" PRISIZE_T " copied, "
        "%" PRISIZE_T " up-to-date, "
        "%" PRISIZE_T " skipped, "
        "%" PRISIZE_T " missing, "
        "%" PRISIZE_T " failed",
        numFiles, numFilesCopied, numFilesUpToDate, numFilesSkipped, numFilesMissing, numFilesFailed);
}

//////////////////////////////////////////////////////////////////////////
string ResourceCompiler::FindSuitableSourceRoot(const std::vector<string>& sourceRootsReversed, const string& fileName)
{
    if (sourceRootsReversed.empty())
    {
        return string();
    }
    if (sourceRootsReversed.size() > 1)
    {
        for (size_t i = 0; i < sourceRootsReversed.size(); ++i)
        {
            const string& sourceRoot = sourceRootsReversed[i];
            const string fullPath = PathHelpers::Join(sourceRoot, fileName);
            const DWORD fileAttribs = GetFileAttributes(fullPath);
            if (fileAttribs == INVALID_FILE_ATTRIBUTES)
            {
                continue;
            }
            if ((fileAttribs & FILE_ATTRIBUTE_NORMAL) != 0)
            {
                return sourceRoot;
            }
        }
    }

    return sourceRootsReversed[0];
}

//////////////////////////////////////////////////////////////////////////
typedef std::set<const char*, stl::less_strcmp<const char*> > References;

void ResourceCompiler::ScanForAssetReferences(std::vector<string>& outReferences, const string& refsRoot)
{
    const char* const scanRoot = ".";

    RCLog("Scanning for asset references in \"%s\"", scanRoot);

    int numSources = 0;

    References references;

    std::list<string> strings; // storage for all strings
    std::vector<char*> lines;

    TextFileReader reader;

    std::vector<string> resourceListFiles;
    FileUtil::ScanDirectory(scanRoot, "auto_resource*.txt", resourceListFiles, true, string());
    FileUtil::ScanDirectory(scanRoot, "resourcelist.txt", resourceListFiles, true, string());
    for (size_t i = 0; i < resourceListFiles.size(); ++i)
    {
        const string& resourceListFile = resourceListFiles[i];
        if (reader.Load(resourceListFile.c_str(), lines))
        {
            for (size_t j = 0; j < lines.size(); ++j)
            {
                const char* const line = lines[j];
                if (references.find(line) == references.end())
                {
                    strings.push_back(line);
                    references.insert(strings.back().c_str());
                }
            }
            ++numSources;
        }
    }

    bool usePrefabSystemForLevels = false;
    AzFramework::ApplicationRequests::Bus::BroadcastResult(
        usePrefabSystemForLevels, &AzFramework::ApplicationRequests::IsPrefabSystemForLevelsEnabled);

    if (!usePrefabSystemForLevels)
    {
        std::vector<string> levelPaks;
        FileUtil::ScanDirectory(scanRoot, "level.pak", levelPaks, true, string());
        for (size_t i = 0; i < levelPaks.size(); ++i)
        {
            const string path = PathHelpers::GetDirectory(levelPaks[i].c_str()) + "\\resourcelist.txt";
            if (reader.LoadFromPak(GetPakSystem(), path, lines))
            {
                for (size_t j = 0; j < lines.size(); ++j)
                {
                    const char* const line = lines[j];
                    if (references.find(line) == references.end())
                    {
                        strings.push_back(line);
                        references.insert(strings.back().c_str());
                    }
                }
                ++numSources;
            }
        }
    }

    RCLog("Found %i unique references in %i sources", references.size(), numSources);
    RCLog("");

    if (refsRoot.empty())
    {
        outReferences.insert(outReferences.end(), references.begin(), references.end());
    }
    else
    {
        // include mipmaps together with referenced dds-textures
        const string ddsExt("dds");
        const string cgfExt("cgf");
        const string chrExt("chr");
        const string skinExt("skin");
        char extSuffix[16];

        for (References::iterator it = references.begin(); it != references.end(); ++it)
        {
            const string ext = PathHelpers::FindExtension(*it);
#if defined(AZ_PLATFORM_WINDOWS)
            const string dosPath = PathHelpers::ToDosPath(*it);
#else
            const string dosPath = PathHelpers::ToUnixPath(*it);
#endif

            if (StringHelpers::EqualsIgnoreCase(ext, ddsExt))
            {
                string fullPath;
                string mipPath;
                for (int mip = 0;; ++mip)
                {
                    azsnprintf(extSuffix, sizeof(extSuffix), ".%i", mip);
                    fullPath = PathHelpers::Join(refsRoot, dosPath + extSuffix);
                    if (!FileUtil::FileExists(fullPath.c_str()))
                    {
                        break;
                    }
                    outReferences.push_back(string(*it) + extSuffix);
                }
                for (int mip = 0;; ++mip)
                {
                    azsnprintf(extSuffix, sizeof(extSuffix), ".%ia", mip);
                    fullPath = PathHelpers::Join(refsRoot, dosPath + extSuffix);
                    if (!FileUtil::FileExists(fullPath.c_str()))
                    {
                        break;
                    }
                    outReferences.push_back(string(*it) + extSuffix);
                }
            }
            else if (StringHelpers::EqualsIgnoreCase(ext, cgfExt) || StringHelpers::EqualsIgnoreCase(ext, chrExt) || StringHelpers::EqualsIgnoreCase(ext, skinExt))
            {
                static const char* const cgfmext = "m";

                string fullPath;
                fullPath = PathHelpers::Join(refsRoot, dosPath + cgfmext);
                if (FileUtil::FileExists(fullPath.c_str()))
                {
                    outReferences.push_back(string(*it) + cgfmext);
                }
            }

            // we are interested only in existing files
            const string fullPath = PathHelpers::Join(refsRoot, dosPath);
            if (FileUtil::FileExists(fullPath))
            {
                outReferences.push_back(*it);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
static bool MatchesWildcardsSet(const string& str, const std::vector<string>& masks)
{
    const size_t numMasks = masks.size();

    for (size_t i = 0; i < numMasks; ++i)
    {
        if (StringHelpers::MatchesWildcardsIgnoreCase(str, masks[i]))
        {
            return true;
        }
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::SaveAssetReferences(const std::vector<string>& references, const string& filename, const string& includeMasksStr, const string& excludeMasksStr)
{
    std::vector<string> includeMasks;
    StringHelpers::Split(includeMasksStr, ";", false, includeMasks);

    std::vector<string> excludeMasks;
    StringHelpers::Split(excludeMasksStr, ";", false, excludeMasks);

    FILE* f = nullptr;
    azfopen(&f, filename.c_str(), "wt");
    if (!f)
    {
        RCLogError("Unable to open %s for writing", filename.c_str());
        return;
    }

    for (std::vector<string>::const_iterator it = references.begin(); it != references.end(); ++it)
    {
        if (MatchesWildcardsSet(*it, excludeMasks))
        {
            continue;
        }

        if (!includeMasks.empty() && !MatchesWildcardsSet(*it, includeMasks))
        {
            continue;
        }
        fprintf(f, "%s\n", it->c_str());
    }
    fclose(f);
}
//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::CleanTargetFolder(bool bUseOnlyInputFiles)
{
    const IConfig* const config = &m_multiConfig.getConfig();

    {
        const string targetroot = PathHelpers::ToPlatformPath(PathHelpers::CanonicalizePath(config->GetAsString("targetroot", "", "")));
        if (targetroot.empty())
        {
            return;
        }
        RCLog("Cleaning target folder %s", targetroot.c_str());
    }

    CDependencyList inputOutputFileList(m_inputOutputFileList);

    // Look at the list of processed files.
    {
        inputOutputFileList.RemoveDuplicates();

        const string filename = FormLogFileName(m_filenameOutputFileList);

        if (FileUtil::FileExists(filename.c_str()))
        {
            const size_t oldCount = inputOutputFileList.GetCount();
            inputOutputFileList.Load(filename);
            const size_t loadedCount = inputOutputFileList.GetCount() - oldCount;
            inputOutputFileList.RemoveDuplicates();
            const size_t addedCount = inputOutputFileList.GetCount() - oldCount;
            RCLog("%u entries (%u unique) found in list of processed files '%s'", (uint)loadedCount, (uint)addedCount, filename.c_str());
        }
        else
        {
            RCLog("List of processed files '%s' is not found", filename.c_str());
        }

        RCLog("%u entries in list of processed files", (uint)inputOutputFileList.GetCount());
    }

    // Prepare lists of files that were deleted.
    std::vector<string> deletedSourceFiles;
    std::vector<string> deletedTargetFiles;

    if (bUseOnlyInputFiles)
    {
        for (size_t nInput = 0; nInput < m_inputFilesDeleted.size(); ++nInput)
        {
            const string& deletedInputFilename = m_inputFilesDeleted[nInput].m_sourceInnerPathAndName;
            deletedSourceFiles.push_back(deletedInputFilename);
            for (size_t i = 0; i < inputOutputFileList.GetCount(); ++i)
            {
                const CDependencyList::SFile& of = inputOutputFileList.GetElement(i);
                if (deletedInputFilename == of.inputFile)
                {
                    deletedTargetFiles.push_back(of.outputFile);
                }
            }
        }
    }
    else
    {
        string lastInputFile;
        bool bSrcFileExists = false;
        for (size_t i = 0; i < inputOutputFileList.GetCount(); ++i)
        {
            // Check if input file exists
            const CDependencyList::SFile& of = inputOutputFileList.GetElement(i);
            if (of.inputFile != lastInputFile)
            {
                lastInputFile = of.inputFile;
                if (FileUtil::FileExists(of.inputFile.c_str()))
                {
                    bSrcFileExists = true;
                }
                else
                {
                    RCLog("Source file deleted: \"%s\"", of.inputFile.c_str());
                    deletedSourceFiles.push_back(of.inputFile);
                    bSrcFileExists = false;
                }
            }
            if (!bSrcFileExists)
            {
                deletedTargetFiles.push_back(of.outputFile);
            }
        }
    }

    std::sort(deletedSourceFiles.begin(), deletedSourceFiles.end());
    std::sort(deletedTargetFiles.begin(), deletedTargetFiles.end());

    //////////////////////////////////////////////////////////////////////////
    // Remove records with matching input filenames
    inputOutputFileList.RemoveInputFiles(deletedSourceFiles);

    //////////////////////////////////////////////////////////////////////////
    // Delete target files from disk
    for (size_t i = 0; i < deletedTargetFiles.size(); ++i)
    {
        const string& filename = deletedTargetFiles[i];
        if (i <= 0 || filename != deletedTargetFiles[i - 1])
        {
            RCLog("Deleting file \"%s\"", filename.c_str());
            AZ::IO::LocalFileIO().Remove(filename.c_str());

        }
    }

    StartProgress();

    m_pPakManager->DeleteFilesFromPaks(config, deletedTargetFiles);

    {
        const string filename = FormLogFileName(m_filenameOutputFileList);
        RCLog("Saving %s", filename.c_str());
        inputOutputFileList.RemoveDuplicates();
        inputOutputFileList.Save(filename.c_str());
    }

    // store deleted files list.
    {
        const string filename = FormLogFileName(m_filenameDeletedFileList);
        RCLog("Saving %s", filename.c_str());

        FILE* f = nullptr;
        azfopen(&f, filename.c_str(), "wt");
        if (f)
        {
            for (size_t i = 0; i < deletedTargetFiles.size(); ++i)
            {
                const string filename2 = CDependencyList::NormalizeFilename(deletedTargetFiles[i].c_str());
                fprintf(f, "%s\n", filename2.c_str());
            }
            fclose(f);
        }
        else
        {
            RCLogError("Failed to write %s", filename.c_str());
        }
    }
}

class CryXmlCleanup
{
public:
    ~CryXmlCleanup()
    {
        if (m_cryXmlModule)
        {
            CryFreeLibrary(m_cryXmlModule);
        }
    }

    HMODULE m_cryXmlModule = nullptr;
};
static CryXmlCleanup g_cryXmlCleanup;

//////////////////////////////////////////////////////////////////////////
ICryXML* LoadICryXML()
{
    HMODULE hXMLLibrary = CryLoadLibraryDefName("CryXML");
    if (NULL == hXMLLibrary)
    {
        RCLogError("Unable to load xml library (CryXML)");
        return 0;
    }
    g_cryXmlCleanup.m_cryXmlModule = hXMLLibrary;

    FnGetICryXML pfnGetICryXML = (FnGetICryXML)CryGetProcAddress(hXMLLibrary, "GetICryXML");
    if (pfnGetICryXML == 0)
    {
        RCLogError("Unable to load xml library (CryXML) - cannot find exported function GetICryXML().");
        return 0;
    }
    return pfnGetICryXML();
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef ResourceCompiler::LoadXml(const char* filename)
{
    ICryXML* pCryXML = LoadICryXML();
    if (!pCryXML)
    {
        return 0;
    }

    // Get the xml serializer.
    IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

    // Read in the input file.
    XmlNodeRef root;
    {
        const bool bRemoveNonessentialSpacesFromContent = false;
        char szErrorBuffer[1024];
        root = pSerializer->Read(FileXmlBufferSource(filename), false, sizeof(szErrorBuffer), szErrorBuffer);
        if (!root)
        {
            RCLogError("Failed to load XML file '%s': %s", filename, szErrorBuffer);
            return 0;
        }
    }
    return root;
}

//////////////////////////////////////////////////////////////////////////
XmlNodeRef ResourceCompiler::CreateXml(const char* tag)
{
    ICryXML* pCryXML = LoadICryXML();
    if (!pCryXML)
    {
        return 0;
    }

    // Get the xml serializer.
    IXMLSerializer* pSerializer = pCryXML->GetXMLSerializer();

    XmlNodeRef root;
    {
        root = pSerializer->CreateNode(tag);
        if (!root)
        {
            RCLogError("Cannot create new XML node '%s'\n", tag);
            return 0;
        }
    }
    return root;
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::ProcessJobFile()
{
    const MultiplatformConfig savedConfig = m_multiConfig;
    const IConfig* const config = &m_multiConfig.getConfig();

    // Job file is an XML with multiple jobs for the RC
    const string jobFile = config->GetAsString("job", "", "");
    if (jobFile.empty())
    {
        RCLogError("No job file specified");
        m_multiConfig = savedConfig;
        return eRcExitCode_Error;
    }

    const string runJob = config->GetAsString("jobtarget", "", "");
    const bool runJobFromCommandLine = !runJob.empty();

    CPropertyVars properties(this);

    properties.SetProperty("_rc_exe_folder", GetExePath());
    properties.SetProperty("_rc_tmp_folder", GetTmpPath());

    if (GetVerbosityLevel() >= 0)
    {
        RCLog("Pre-defined job properties:");
        properties.PrintProperties();
    }

    config->CopyToPropertyVars(properties);

    XmlNodeRef root = LoadXml(jobFile.c_str());
    if (!root)
    {
        RCLogError("Failed to load job XML file %s", jobFile.c_str());
        m_multiConfig = savedConfig;
        return eRcExitCode_Error;
    }

    // Check command line with respect to known DefaultProperties
    {
        std::vector<string> defaultProperties;

        for (int i = 0; i < root->getChildCount(); ++i)
        {
            ExtractJobDefaultProperties(defaultProperties, root->getChild(i));
        }

        if (!CheckCommandLineOptions(*config, &defaultProperties))
        {
            return eRcExitCode_Error;
        }
    }

    int result = eRcExitCode_Success;

    for (int i = 0; i < root->getChildCount(); ++i)
    {
        XmlNodeRef jobNode = root->getChild(i);
        const bool runNodes = !runJobFromCommandLine;
        const int tmpResult = EvaluateJobXmlNode(properties, jobNode, runNodes);
        if (result == eRcExitCode_Success)
        {
            result = tmpResult;
        }
    }

    if (runJobFromCommandLine)
    {
        const int tmpResult = RunJobByName(properties, root, runJob);
        if (result == eRcExitCode_Success)
        {
            result = tmpResult;
        }
    }

    m_multiConfig = savedConfig;
    return result;
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::ExtractJobDefaultProperties(std::vector<string>& properties, const XmlNodeRef& jobNode)
{
    if (jobNode->isTag("DefaultProperties"))
    {
        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); ++attr)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            if (std::find(properties.begin(), properties.end(), key) == properties.end())
            {
                properties.push_back(StringHelpers::MakeLowerCase(key));
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::EvaluateJobXmlNode(CPropertyVars& properties, XmlNodeRef& jobNode, bool runJobs)
{
    IConfig* const config = &m_multiConfig.getConfig();

    if (g_gotCTRLBreakSignalFromOS)
    {
        return eRcExitCode_Error;
    }

    if (jobNode->isTag("Properties"))
    {
        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            string strValue = value;
            properties.ExpandProperties(strValue);
            properties.SetProperty(key, strValue);
        }
        return eRcExitCode_Success;
    }

    // Default properties don't overwrite existing ones (e.g. specified from command line)
    if (jobNode->isTag("DefaultProperties"))
    {
        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            if (!properties.HasProperty(key))
            {
                string strValue = value;
                properties.ExpandProperties(strValue);
                properties.SetProperty(key, strValue);
            }
        }
        return eRcExitCode_Success;
    }

    if (jobNode->isTag("Run"))
    {
        if (!runJobs)
        {
            return eRcExitCode_Success;
        }

        const char* jobListName = jobNode->getAttr("Job");
        if (strlen(jobListName) == 0)
        {
            return eRcExitCode_Success;
        }

        // Attributes are config modifiers.
        const CPropertyVars previousProperties = properties;
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);
            string strValue = value;
            properties.ExpandProperties(strValue);
            properties.SetProperty(key, strValue);
        }

        int result = RunJobByName(properties, jobNode, jobListName);
        properties = previousProperties;
        return result;
    }

    if (jobNode->isTag("Include"))
    {
        const char* includeFile = jobNode->getAttr("file");
        if (strlen(includeFile) == 0)
        {
            return eRcExitCode_Success;
        }

        const string jobFile = config->GetAsString("job", "", "");
        const string includePath = PathHelpers::AddSeparator(PathHelpers::GetDirectory(jobFile)) + includeFile;

        XmlNodeRef root = LoadXml(includePath);
        if (!root)
        {
            RCLogError("Failed to load included job XML file '%s'", includePath.c_str());
            return eRcExitCode_Error;
        }

        // Add include sub-nodes
        XmlNodeRef parent = jobNode->getParent();
        while (root->getChildCount() != 0)
        {
            XmlNodeRef subJobNode = root->getChild(0);
            root->removeChild(subJobNode);
            parent->addChild(subJobNode);
        }
        return eRcExitCode_Success;
    }

    // Condition node.
    if (jobNode->isTag("if") || jobNode->isTag("ifnot"))
    {
        bool bIf = false;
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);

            string propValue;
            properties.GetProperty(key, propValue);
            if (azstricmp(value, propValue.c_str()) == 0)
            {
                // match.
                bIf = true;
            }
        }
        if (jobNode->isTag("ifnot"))
        {
            bIf = !bIf; // Invert
        }
        int result = eRcExitCode_Success;
        if (bIf)
        {
            // Exec sub-nodes
            for (int i = 0; i < jobNode->getChildCount(); i++)
            {
                XmlNodeRef subJobNode = jobNode->getChild(i);
                const int tmpResult = EvaluateJobXmlNode(properties, subJobNode, true);
                if (result == eRcExitCode_Success)
                {
                    result = tmpResult;
                }
            }
        }
        return result;
    }

    if (jobNode->isTag("Job"))
    {
        RCLog("-------------------------------------------------------------------");
        string jobLog = "Job: ";
        // Delete all config entries from previous job.
        config->ClearPriorityUsage(eCP_PriorityJob);

        bool bCleanTargetRoot = false;
        string refsSaveFilename;
        string refsSaveInclude;
        string refsSaveExclude;
        string refsRoot;

        // Attributes are config modifiers.
        for (int attr = 0; attr < jobNode->getNumAttributes(); attr++)
        {
            const char* key = "";
            const char* value = "";
            jobNode->getAttributeByIndex(attr, &key, &value);

            string valueStr = value;
            properties.ExpandProperties(valueStr);

            if (azstricmp(key, "input") == 0)
            {
                jobLog += string("/") + key + "=" + valueStr + " ";
                continue;
            }
            else if (azstricmp(key, "clean_targetroot") == 0)
            {
                bCleanTargetRoot = true;
                continue;
            }
            else if (azstricmp(key, "refs_scan") == 0)
            {
                RCLogError("refs_scan is not supported anymore");
                return eRcExitCode_Error;
            }
            else if (azstricmp(key, "refs_save") == 0)
            {
                if (valueStr.empty())
                {
                    RCLogError("Missing filename in refs_save option");
                    return eRcExitCode_Error;
                }
                refsSaveFilename = valueStr;
                continue;
            }
            else if (azstricmp(key, "refs_root") == 0)
            {
                refsRoot = valueStr;
                continue;
            }
            else if (azstricmp(key, "refs_save_include") == 0)
            {
                refsSaveInclude = valueStr;
                continue;
            }
            else if (azstricmp(key, "refs_save_exclude") == 0)
            {
                refsSaveExclude = valueStr;
                continue;
            }

            config->SetKeyValue(eCP_PriorityJob, key, valueStr);
            jobLog += string("/") + key + "=" + valueStr + " (attribute) ";
        }

        // Apply properties from RCJob to config
        properties.Enumerate([config, &jobLog](const string& propName, const string& propVal)
            {
                if (config->HasKeyRegistered(propName) && !StringHelpers::EqualsIgnoreCase(propName, "job"))
                {
                    config->SetKeyValue(eCP_PriorityProperty, propName, propVal);
                    jobLog += string("/") + propName + "=" + propVal + " (property) ";
                }
            });

        // Check current platform property against start-up platform setting
        // Reason: This setting cannot be modified after start-up
        const char* pCurrentPlatform = NULL;
        if (config->GetKeyValue("platform", pCurrentPlatform) && pCurrentPlatform)
        {
            int currentPlatformIndex = FindPlatform(pCurrentPlatform);
            if (GetMultiplatformConfig().getActivePlatform() != currentPlatformIndex)
            {
                RCLogWarning("The platform property '--platform=%s' is ignored because it can only be specified on the command-line", pCurrentPlatform);
            }
        }

        string fileSpec = jobNode->getAttr("input");
        properties.ExpandProperties(fileSpec);
        if (!fileSpec.empty())
        {
            RCLog(jobLog);
            RemoveOutputFiles();
            std::vector<RcFile> files;
            if (CollectFilesToCompile(fileSpec, files) && !files.empty())
            {
                bool result = CompileFilesBySingleProcess(files);
                if (!result)
                {
                    RCLogError("Error: Failed to compile files");
                    return eRcExitCode_Error;
                }
            }
        }
        else
        {
            if (!refsSaveFilename.empty())
            {
                properties.ExpandProperties(refsSaveFilename);
                if (refsSaveFilename.empty())
                {
                    RCLogError("Empty filename specified in refs_save option");
                    return eRcExitCode_Error;
                }

                properties.ExpandProperties(refsRoot);

                std::vector<string> references;
                ScanForAssetReferences(references, refsRoot);
                SaveAssetReferences(references, refsSaveFilename, refsSaveInclude, refsSaveExclude);
            }
            if (bCleanTargetRoot)
            {
                CleanTargetFolder(false);
            }
        }
        // Delete all config entries from our job.
        config->ClearPriorityUsage(eCP_PriorityJob | eCP_PriorityProperty);
        return eRcExitCode_Success;
    }

    return eRcExitCode_Success;
}

//////////////////////////////////////////////////////////////////////////
int ResourceCompiler::RunJobByName(CPropertyVars& properties, XmlNodeRef& anyNode, const char* name)
{
    // Find root
    XmlNodeRef root = anyNode;
    while (root->getParent())
    {
        root = root->getParent();
    }

    // Find JobList.
    XmlNodeRef jobListNode = root->findChild(name);
    if (!jobListNode)
    {
        RCLogError("Unable to find job \"%s\"", name);
        return eRcExitCode_Error;
    }
    {
        // Execute Job sub nodes.
        int result = eRcExitCode_Success;
        for (int i = 0; i < jobListNode->getChildCount(); i++)
        {
            XmlNodeRef subJobNode = jobListNode->getChild(i);
            const int tmpResult = EvaluateJobXmlNode(properties, subJobNode, true);
            if (result == eRcExitCode_Success)
            {
                result = tmpResult;
            }
        }
        return result;
    }
}

//////////////////////////////////////////////////////////////////////////
void ResourceCompiler::LogMemoryUsage(bool bReportProblemsOnly)
{
#if defined(AZ_PLATFORM_WINDOWS)
    PROCESS_MEMORY_COUNTERS p;
    ZeroMemory(&p, sizeof(p));
    p.cb = sizeof(p);


    if (!GetProcessMemoryInfo(GetCurrentProcess(), &p, sizeof(p)))
    {
        RCLogError("Cannot obtain memory info");
        return;
    }

    static const float megabyte = 1024 * 1024;
    const float peakSizeMb = p.PeakWorkingSetSize / megabyte;
#if defined(WIN64)
    static const float warningMemorySizePeakMb =  7500.0f;
    static const float errorMemorySizePeakMb   = 15500.0f;
#else
    static const float warningMemorySizePeakMb =  3100.0f;
    static const float errorMemorySizePeakMb   =  3600.0f;
#endif

    bool bReportProblem = false;
    {
        AZStd::lock_guard<AZStd::mutex> lock(m_memorySizeLock);
        if (peakSizeMb > m_memorySizePeakMb)
        {
            m_memorySizePeakMb = peakSizeMb;
            bReportProblem = (peakSizeMb >= warningMemorySizePeakMb);
        }
    }

    if (bReportProblem || !bReportProblemsOnly)
    {
        if (peakSizeMb >= warningMemorySizePeakMb)
        {
            ((peakSizeMb >= errorMemorySizePeakMb) ? RCLogError : RCLogWarning)(
                "Memory: working set %.1fMb (peak %.1fMb - DANGER!), pagefile %.1fMb (peak %.1fMb)",
                p.WorkingSetSize / megabyte, p.PeakWorkingSetSize / megabyte,
                p.PagefileUsage / megabyte, p.PeakPagefileUsage / megabyte);
        }
        else
        {
            RCLog(
                "Memory: working set %.1fMb (peak %.1fMb), pagefile %.1fMb (peak %.1fMb)",
                p.WorkingSetSize / megabyte, p.PeakWorkingSetSize / megabyte,
                p.PagefileUsage / megabyte, p.PeakPagefileUsage / megabyte);
        }
    }
#endif
}

void ResourceCompiler::RegisterDefaultKeys()
{
    RegisterKey("_debug", "");   // hidden key for debug-related activities. parsing is module-dependent and subject to change without prior notice.

    RegisterKey("project-path", "Path to project. Used to find files related to the project.");
    RegisterKey("project-name", R"(Name of the project. It's value is derived from the project.json file "project_name" field.)");
    RegisterKey("wait",
        "wait for an user action on start and/or finish of RC:\n"
        "0-don't wait (default),\n"
        "1 or empty-wait for a key pressed on finish,\n"
        "2-pop up a message box and wait for the button pressed on finish,\n"
        "3-pop up a message box and wait for the button pressed on start,\n"
        "4-pop up a message box and wait for the button pressed on start and on finish\n");
    RegisterKey("wx", "pause and display message box in case of warning or error");
    RegisterKey("recursive", "traverse input directory with sub folders");
    RegisterKey("refresh", "force recompilation of resources with up to date timestamp");
    RegisterKey("platform", "to specify platform (for supported names see [_platform] sections in ini)");
    RegisterKey("pi", "provides the platform id from the Asset Processor");
    RegisterKey("statistics", "log statistics to rc_stats_* files");
    RegisterKey("dependencies",
        "Use it to specify a file with dependencies to be written.\n"
        "Each line in the file will contain an input filename\n"
        "and an output filename for every file written by ");
    RegisterKey("clean_targetroot", "When 'targetroot' switch specified will clean up this folder after rc runs, to delete all files that were not processed");
    RegisterKey("verbose", "to control amount of information in logs: 0-default, 1-detailed, 2-very detailed, etc");
    RegisterKey("quiet", "to suppress all printouts");
    RegisterKey("skipmissing", "do not produce warnings about missing input files");
    RegisterKey("logfiles", "to suppress generating log file rc_log.log");
    RegisterKey("logprefix", "prepends this prefix to every log file name used (by default the prefix is the exe's folder).");
    RegisterKey("logtime", "logs time passed: 0=off, 1=on (default)");
    RegisterKey("watchfolder", "The watched root folder that this file is located in.  Used to produce the relative asset name.");
    RegisterKey("nosourcecontrol", "Boolean - if true, disables initialization of source control.  Disabling Source Control in the editor automatically disables it here, too.");
    RegisterKey("sourceroot", "list of source folders separated by semicolon");
    RegisterKey("targetroot", "to define the destination folder. note: this folder and its subtrees will be excluded from the source files scanning process");
    RegisterKey("targetnameformat",
        "Use it to specify format of the output filenames.\n"
        "syntax is /targetnameformat=\"<pair[;pair[;pair[...]]]>\" where\n"
        "<pair> is <mask>,<resultingname>.\n"
        "<mask> is a name consisting of normal and wildcard chars.\n"
        "<resultingname> is a name consisting of normal chars and special strings:\n"
        "{0} filename of a file matching the mask,\n"
        "{1} part of the filename matching the first wildcard of the mask,\n"
        "{2} part of the filename matching the second wildcard of the mask,\n"
        "and so on.\n"
        "A filename will be processed by first pair that has matching mask.\n"
        "If no any match for a filename found, then the filename stays\n"
        "unmodified.\n"
        "Example: /targetnameformat=\"*alfa*.txt,{1}beta{2}.txt\"");
    RegisterKey("filesperprocess",
        "to specify number of files converted by one process in one step\n"
        "default is 100. this option is unused if /processes is 0.");
    RegisterKey("failonwarnings", "return error code if warnings are encountered");

    RegisterKey("help", "lists all usable keys of the ResourceCompiler with description");
    RegisterKey("version", "shows version and exits");
    RegisterKey("overwriteextension", "ignore existing file extension and use specified convertor");
    RegisterKey("overwritefilename", "use the filename for output file (folder part is not affected)");

    RegisterKey("listfile", "Specify List file, List file can contain file lists from zip files like: @Levels\\Test\\level.pak|resourcelist.txt");
    RegisterKey("listformat",
        "Specify format of the file name read from the list file. You may use special strings:\n"
        "{0} the file name from the file list,\n"
        "{1} text matching first wildcard from the input file mask,\n"
        "{2} text matching second wildcard from the input file mask,\n"
        "and so on.\n"
        "Also, you can use multiple format strings, separated by semicolons.\n"
        "In this case multiple filenames will be generated, one for\n"
        "each format string.");
    RegisterKey("copyonly", "copy source files to target root without processing");
    RegisterKey("copyonlynooverwrite", "copy source files to target root without processing, will not overwrite if target file exists");
    RegisterKey("outputproductdependencies", "output product dependencies");
    RegisterKey("name_as_crc32", "When creating Pak File outputs target filename as the CRC32 code without the extension");
    RegisterKey("exclude", "List of file exclusions for the command, separated by semicolon, may contain wildcard characters");
    RegisterKey("exclude_listfile", "Specify a file which contains a list of files to be excluded from command input");

    RegisterKey("validate", "When specified RC is running in a resource validation mode");
    RegisterKey("MailServer", "SMTP Mail server used when RC needs to send an e-mail");
    RegisterKey("MailErrors", "0=off 1=on When enabled sends an email to the user who checked in asset that failed validation");
    RegisterKey("cc_email", "When sending mail this address will be added to CC, semicolon separates multiple addresses");
    RegisterKey("job", "Process a job xml file");
    RegisterKey("jobtarget", "Run only a job with specific name instead of whole job-file. Used only with /job option");
    RegisterKey("unittest", "Run the unit tests for resource compiler and nothing else");
    RegisterKey("unattended", "Prevents RC from opening any dialogs or message boxes");
    RegisterKey("createjobs", "Instructs RC to read the specified input file (a CreateJobsRequest) and output a CreateJobsResponse");
    RegisterKey("port", "Specifies the port that should be used to connect to the asset processor.  If not set, the default from the bootstrap cfg will be used instead");
    RegisterKey("branchtoken", "Specifies a branchtoken that should be used by the RC to negotiate with the asset processor. if not set it will be set from the bootstrap file.");
    RegisterKey("recompress", "Recompress a pack file during a copy job using the multi-variant process which picks the fastest decompressor");
    RegisterKey("use_fastest", "Checks every compressor and uses the one that decompresses the data fastest when adding files to a PAK");
    RegisterKey("skiplevelpaks", "Prevents RC from adding level related pak files to the auxiliary contents during auxiliary content creation step.");
}
