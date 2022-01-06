/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// windows include must be first so we get the full version (AZCore bring the trimmed one)
#ifdef _WIN32
#include <AzCore/PlatformIncl.h>
#include <winnls.h>
#include <shobjidl.h>
#include <objbase.h>
#include <objidl.h>
#include <shlguid.h>
#include <shlobj.h>
#include <shlwapi.h>
#endif

#include "gridhub.hxx"
AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QtWidgets/QApplication>
#include <QtWidgets/QMessageBox>
#include <QtCore/QFile>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QAbstractNativeEventFilter>
#include <QtCore/QSharedMemory>
#include <QtCore/QProcess>
AZ_POP_DISABLE_WARNING

#include <AzCore/EBus/EBus.h>
#include <AzCore/Memory/AllocatorManager.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/Utils/Utils.h>

#ifdef AZ_PLATFORM_WINDOWS
#include <Shlwapi.h>
#elif defined AZ_PLATFORM_APPLE
#include <mach-o/dyld.h>
#include <errno.h>
#include <sys/param.h>
#include <libgen.h>
#else
#include <sys/param.h>
#include <errno.h>
#endif

#ifdef AZ_PLATFORM_WINDOWS
#define GRIDHUB_TSR_SUFFIX L"_copyapp_"
#define GRIDHUB_TSR_NAME L"GridHub_copyapp_.exe"
#define GRIDHUB_IMAGE_NAME L"GridHub.exe"
#else
#define GRIDHUB_TSR_SUFFIX "_copyapp_"
#define GRIDHUB_TSR_NAME "GridHub_copyapp_"
#define GRIDHUB_IMAGE_NAME "GridHub"
#endif

#if AZ_TRAIT_OS_PLATFORM_APPLE
#ifdef _DEBUG
#include <assert.h>
#include <stdbool.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/sysctl.h>

static bool IsDebuggerPresent()
    // Returns true if the current process is being debugged (either
    // running under the debugger or has a debugger attached post facto).
{
    int                 junk;
    int                 mib[4];
    struct kinfo_proc   info;
    size_t              size;

    // Initialize the flags so that, if sysctl fails for some bizarre
    // reason, we get a predictable result.

    info.kp_proc.p_flag = 0;

    // Initialize mib, which tells sysctl the info we want, in this case
    // we're looking for information about a specific process ID.

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PID;
    mib[3] = getpid();

    // Call sysctl.

    size = sizeof(info);
    junk = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
    assert(junk == 0);

    // We're being debugged if the P_TRACED flag is set.

    return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
}
#else
static bool IsDebuggerPresent()
{
    return false;
}
#endif
#elif defined AZ_PLATFORM_LINUX
static bool IsDebuggerPresent()
{
    //KDAB_TODO
    return false;
}
#endif

/**
 * Editor Application \ref ComponentApplication.
 */
class GridHubApplication : public AZ::ComponentApplication, AZ::SystemTickBus::Handler
{
public:
    GridHubApplication() : m_monitorForExeChanges(false), m_needToRelaunch(false), m_timeSinceLastCheckForChanges(0.0f) {}
    /// Create application, if systemEntityFileName is NULL, we will create with default settings.
    AZ::Entity* Create(const Descriptor& descriptor, const StartupParameters& startupParameters = StartupParameters()) override;
    void Destroy() override;

    bool            IsNeedToRelaunch() const        { return m_needToRelaunch; }
    bool            IsValidModuleName() const       { return m_monitorForExeChanges; }
    const QString   GetModuleName() const           { return m_originalExeFileName; }
protected:
    /**
     * This is the function that will be called instantly after the memory
     * manager is created. This is where we should register all core component
     * factories that will participate in the loading of the bootstrap file
     * or all factories in general.
     * When you create your own application this is where you should FIRST call
     * ComponentApplication::RegisterCoreComponents and then register the application
     * specific core components.
     */
     void RegisterCoreComponents() override;

     /**
          AZ::SystemTickBus::Handler
     */
     void OnSystemTick() override
     {
         // Calculate deltaTime
         AZStd::chrono::system_clock::time_point now = AZStd::chrono::system_clock::now();
         static AZStd::chrono::system_clock::time_point lastUpdate = now;

         AZStd::chrono::duration<float> delta = now - lastUpdate;
         float deltaTime = delta.count();

         lastUpdate = now;

        /// This is called from a 'safe' main sync point and should originate all messages that need to be synced to the 'main' thread.
        // {
        //  if (AZ::IO::Streamer::IsReady())
        //  AZ::IO::StreamerAPI::v1_5::GetStreamer()->ReceiveRequests(); // activate callbacks on main thread!
        // }

         // check to see if we got a newer version of our executable, if so run it.
         if( m_monitorForExeChanges )
         {
             m_timeSinceLastCheckForChanges += deltaTime;
             if( m_timeSinceLastCheckForChanges > 5.0f ) // check every 5 seconds
             {
                 m_timeSinceLastCheckForChanges -= 5.0f;
                 const QDateTime fileLastModified = QFileInfo(m_originalExeFileName).lastModified();
                 if (!fileLastModified.isNull())
                 {
                     if (fileLastModified != m_originalExeLastModified)
                     {
                         AZ_Printf("GridHub","Detected exe file change quitting...");
                         // we need to quit the APP and do copy and run
                         m_needToRelaunch = true;
                         qApp->quit();
                     }
                 }
             }
         }
     }

     void SetSettingsRegistrySpecializations(AZ::SettingsRegistryInterface::Specializations& specializations) override
     {
         ComponentApplication::SetSettingsRegistrySpecializations(specializations);
         specializations.Append("tools");
         specializations.Append("gridhub");
     }

     QString        m_originalExeFileName;
     QDateTime      m_originalExeLastModified;
     bool           m_monitorForExeChanges;
     bool           m_needToRelaunch;
     float          m_timeSinceLastCheckForChanges;
};

/**
 * Gridhub application.
 */
class QGridHubApplication : public QApplication, public QAbstractNativeEventFilter
{
public:
    QGridHubApplication(int &argc, char **argv) : QApplication(argc,argv)
    {
#ifdef AZ_PLATFORM_WINDOWS
        CoInitialize(NULL);
#endif
        m_systemEntity = NULL;
        m_gridHubComponent = NULL;
        installNativeEventFilter(this);
    }

    ~QGridHubApplication()
    {
        removeNativeEventFilter(this);
    }

    //virtual bool QGridHubApplication::winEventFilter( MSG *msg , long *result)
    bool nativeEventFilter(const QByteArray &eventType, void *message, [[maybe_unused]] long *result) override
    {
#ifdef AZ_PLATFORM_WINDOWS
        if ((eventType == "windows_generic_MSG")||(eventType == "windows_dispatcher_MSG"))
        {
            MSG* msg = reinterpret_cast<MSG*>(message);
            switch (msg->message) {
                case WM_QUERYENDSESSION:
                    Finalize();
                    break;
            }
        }
#endif
        return false;
    }

    void Initialize()
    {
        m_systemEntity = m_componentApp.Create({});
        AZ_Assert(m_systemEntity, "Unable to create the system entity!");

        if( !m_systemEntity->FindComponent<AZ::MemoryComponent>() )
        {
            m_systemEntity->CreateComponent<AZ::MemoryComponent>();
        }

        if( !m_systemEntity->FindComponent<GridHubComponent>() )
            m_systemEntity->CreateComponent<GridHubComponent>();

        m_gridHubComponent = m_systemEntity->FindComponent<GridHubComponent>();

        if (m_componentApp.IsValidModuleName())
            AddToStartupFolder(m_componentApp.GetModuleName(),m_gridHubComponent->IsAddToStartupFolder());

        m_systemEntity->Init();
        m_systemEntity->Activate();

        if (styleSheet().isEmpty())
        {
            QDir::addSearchPath("UI", ":/GridHub/Resources/StyleSheetImages");
            QFile file(":/GridHub/Resources/style_dark.qss");
            file.open(QFile::ReadOnly);
            QString styleSheet = QLatin1String(file.readAll());
            setStyleSheet(styleSheet);
        }
    }

    int Execute()
    {
        GridHub mainWnd(&m_componentApp,m_gridHubComponent);
        m_gridHubComponent->SetUI(&mainWnd);
        // show the window only when we debug
        if( !m_componentApp.IsValidModuleName() )
            mainWnd.show();
        return exec();
    }

    void Finalize()
    {
        if( m_systemEntity )
        {
            m_systemEntity->Deactivate();

            if (m_componentApp.IsValidModuleName())
                AddToStartupFolder(m_componentApp.GetModuleName(),m_gridHubComponent->IsAddToStartupFolder());

            m_componentApp.Destroy();
            m_systemEntity = NULL;
            m_gridHubComponent = NULL;
        }
    }

    bool IsNeedToRelaunch()
    {
        if (m_systemEntity)
        {
            return m_componentApp.IsNeedToRelaunch();
        }
        return false;
    }

    static void AddToStartupFolder(const QString& moduleFilename, bool isAdd)
    {
#ifdef AZ_PLATFORM_WINDOWS
        HRESULT hres;
        wchar_t startupFolder[MAX_PATH] = {0};
        wchar_t fullLinkName[MAX_PATH] = {0};

        LPITEMIDLIST pidlFolder = NULL;
        hres = SHGetFolderLocation(0,/*CSIDL_COMMON_STARTUP all users required admin access*/CSIDL_STARTUP,NULL,0,&pidlFolder);
        if (SUCCEEDED(hres))
        {
            if (SHGetPathFromIDList(pidlFolder, startupFolder))
            {
                wcscat_s(fullLinkName, startupFolder);
                wcscat_s(fullLinkName, L"\\Amazon Grid Hub.lnk");
            }
            CoTaskMemFree(pidlFolder);
        }

        if( moduleFilename.isEmpty() || wcslen(fullLinkName) == 0 )
            return;

        // for development, never autoadd to startup
        if( moduleFilename.contains("gridmate\\development", Qt::CaseInsensitive))
            isAdd = false;

        if( isAdd )
        {
            // add to start up folder

            // get my file full name
            IShellLink* psl;

            // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
            // has already been called.
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
            if (SUCCEEDED(hres))
            {
                IPersistFile* ppf;

                // Set the path to the shortcut target and add the description.
                psl->SetPath(moduleFilename.toStdWString().c_str());
                psl->SetDescription(L"Amazon Grid Hub");

                // Query IShellLink for the IPersistFile interface, used for saving the
                // shortcut in persistent storage.
                hres = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf);

                if (SUCCEEDED(hres))
                {
                    // Save the link by calling IPersistFile::Save.
                    hres = ppf->Save(fullLinkName, TRUE);
                    ppf->Release();
                }
                psl->Release();
            }
        }
        else
        {
            // remove from start up folder
            DeleteFileW(fullLinkName);
        }
#endif
#if AZ_TRAIT_OS_PLATFORM_APPLE
        if (isAdd)
        {
            const QString command = "tell application \"System Events\" to make login item at end with properties {path:\"%1\"}";
            QString path = moduleFilename;
            QDir dir(QFileInfo(path).absoluteDir());
            dir.cdUp();
            dir.cdUp();
            if (dir.dirName() == GRIDHUB_IMAGE_NAME".app")
            {
                path = dir.path();
            }
            QProcess::startDetached("/usr/bin/osascript", {"-e", command.arg(path)});
        }
        else
        {
            const QString command = "tell application \"System Events\" to delete login item \"%1\"";
            QProcess::startDetached("/usr/bin/osascript", {"-e", command.arg(GRIDHUB_IMAGE_NAME)});
        }
#endif
    }

    AZ::Entity* m_systemEntity;
    GridHubComponent* m_gridHubComponent;
    GridHubApplication m_componentApp;
};


//=========================================================================
// Create
// [6/18/2012]
//=========================================================================
AZ::Entity*
GridHubApplication::Create(const Descriptor& descriptor, const StartupParameters& startupParameters /* = StartupParameters() */)
{
    m_monitorForExeChanges = IsDebuggerPresent() ? false : true;
    if (m_monitorForExeChanges)
    {
        bool isError = false;
#ifdef AZ_PLATFORM_WINDOWS
        char originalExeFileName[MAX_PATH];
        if (AZ::Utils::GetExecutablePath(originalExeFileName, AZ_ARRAY_SIZE(originalExeFileName)).m_pathStored == AZ::Utils::ExecutablePathResult::Success)
        {
            wchar_t originalExeFileNameW[MAX_PATH];
            AZStd::to_wstring(originalExeFileNameW, MAX_PATH, originalExeFileName);
            PathRemoveFileSpec(originalExeFileNameW);
            PathAppend(originalExeFileNameW, GRIDHUB_IMAGE_NAME);

            AZStd::string finalExeFileName;
            AZStd::to_string(finalExeFileName, originalExeFileNameW);
            m_originalExeFileName = finalExeFileName.c_str();

            m_originalExeLastModified = QFileInfo(m_originalExeFileName).lastModified();
        }
#elif defined AZ_PLATFORM_LINUX
        // KDAB_TODO
        if (0) //Avoid compile error
        {
        }
#else
        char path[MAXPATHLEN];
        unsigned int pathSize = MAXPATHLEN;
        if (_NSGetExecutablePath(path, &pathSize) == 0)
        {
            m_originalExeFileName = QDir(dirname(path)).absoluteFilePath(GRIDHUB_IMAGE_NAME);
            m_originalExeLastModified = QFileInfo(m_originalExeFileName).lastModified();
        }
#endif
        else
        {
            isError = true;
        }

        if (isError)
        {
#ifdef AZ_PLATFORM_WINDOWS
            AZ_Printf("GridHub", "Failed to get module file name %d\n", GetLastError());
#else
            AZ_Printf("GridHub", "Failed to get module file name %d\n", strerror(errno));
#endif
            m_monitorForExeChanges = false;
        }
    }

    AZ::Entity* sysEntity = ComponentApplication::Create(descriptor, startupParameters);
    if (sysEntity)
    {
        AZ::SystemTickBus::Handler::BusConnect();
    }
    return sysEntity;
}

//=========================================================================
// Destroy
// [7/6/2012]
//=========================================================================
void GridHubApplication::Destroy()
{
    AZ::SystemTickBus::Handler::BusDisconnect();
    AZ::ComponentApplication::Destroy();
}

//=========================================================================
// RegisterCoreComponents
// [6/18/2012]
//=========================================================================
void GridHubApplication::RegisterCoreComponents()
{
    ComponentApplication::RegisterCoreComponents();

    // GridHub components
    GridHubComponent::CreateDescriptor();
}

//=========================================================================
// CopyAndRun
// [10/24/2012]
//=========================================================================
void CopyAndRun(bool failSilently)
{
#ifdef AZ_PLATFORM_WINDOWS
    char myFileName[MAX_PATH] = { 0 };
    if (AZ::Utils::GetExecutablePath(myFileName, MAX_PATH).m_pathStored == AZ::Utils::ExecutablePathResult::Success)
    {
        wchar_t myFileNameW[MAX_PATH] = { 0 };
        AZStd::to_wstring(myFileNameW, MAX_PATH, myFileName);
        wchar_t sourceProcPath[MAX_PATH] = { 0 };
        wchar_t targetProcPath[MAX_PATH] = { 0 };
        wchar_t procDrive[MAX_PATH] = { 0 };
        wchar_t procDir[MAX_PATH] = { 0 };
        wchar_t procFname[MAX_PATH] = { 0 };
        wchar_t procExt[MAX_PATH] = { 0 };
        _wsplitpath_s(myFileNameW, procDrive, procDir, procFname, procExt);
        _wmakepath_s(sourceProcPath, procDrive, procDir, GRIDHUB_IMAGE_NAME, NULL);
        _wmakepath_s(targetProcPath, procDrive, procDir, GRIDHUB_TSR_NAME, NULL);
        if (CopyFileEx(sourceProcPath, targetProcPath, NULL, NULL, NULL, 0))
        {
            STARTUPINFO si;
            PROCESS_INFORMATION pi;

            memset(&si, 0, sizeof(si));
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = SW_SHOWNORMAL;

            CreateProcess(
                NULL,                       // No module name (use command line).
                targetProcPath,             // Command line.
                NULL,                       // Process handle not inheritable.
                NULL,                       // Thread handle not inheritable.
                FALSE,                      // Set handle inheritance to FALSE.
                CREATE_NEW_PROCESS_GROUP,   // Don't use the same process group as the current process!
                NULL,                       // Use parent's environment block.
                NULL,                       // Startup in the same directory as the executable
                &si,                        // Pointer to STARTUPINFO structure.
                &pi);                       // Pointer to PROCESS_INFORMATION structure.
        }
        else
        {
            if (!failSilently)
            {
                wchar_t errorMsg[1024] = { 0 };
                swprintf_s(errorMsg, L"Failed to copy GridHub. Make sure that %s%s is writable!", procFname, procExt);
                MessageBoxW(NULL, errorMsg, NULL, MB_ICONSTOP|MB_OK);
            }
        }
    }
#elif defined AZ_PLATFORM_LINUX
    // KDAB_TODO
#else
    char path[MAXPATHLEN];
    unsigned int pathSize = MAXPATHLEN;
    if (_NSGetExecutablePath(path, &pathSize) == 0)
    {
        QString sourceProcPath = path;
        QString targetProcPath = QFileInfo(path).dir().absoluteFilePath(GRIDHUB_TSR_NAME);
        QFile::remove(targetProcPath);
        if (QFile::copy(sourceProcPath, targetProcPath))
        {
            QProcess::startDetached(targetProcPath, QStringList());
        }
        else
        {
            if (!failSilently)
            {
                QMessageBox::critical(nullptr, QString(), QGridHubApplication::tr("Failed to copy GridHub. Make sure that %1 is writable!"));
            }
        }
    }
#endif
}

//=========================================================================
// RelaunchImage
// [10/19/2016]
//=========================================================================
void RelaunchImage()
{
#ifdef AZ_PLATFORM_WINDOWS
    char myFileName[MAX_PATH] = { 0 };
    if (AZ::Utils::GetExecutablePath(myFileName, MAX_PATH).m_pathStored == AZ::Utils::ExecutablePathResult::Success)
    {
        wchar_t myFileNameW[MAX_PATH] = { 0 };
        AZStd::to_wstring(myFileNameW, MAX_PATH, myFileName);
        wchar_t targetProcPath[MAX_PATH] = { 0 };
        wchar_t procDrive[MAX_PATH] = { 0 };
        wchar_t procDir[MAX_PATH] = { 0 };
        wchar_t procFname[MAX_PATH] = { 0 };
        wchar_t procExt[MAX_PATH] = { 0 };
        _wsplitpath_s(myFileNameW, procDrive, procDir, procFname, procExt);
        _wmakepath_s(targetProcPath, procDrive, procDir, GRIDHUB_IMAGE_NAME, NULL);

        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        memset(&si, 0, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_SHOWNORMAL;

        CreateProcess(
            NULL,                       // No module name (use command line).
            targetProcPath,             // Command line.
            NULL,                       // Process handle not inheritable.
            NULL,                       // Thread handle not inheritable.
            FALSE,                      // Set handle inheritance to FALSE.
            CREATE_NEW_PROCESS_GROUP,   // Don't use the same process group as the current process!
            NULL,                       // Use parent's environment block.
            NULL,                       // Startup in the same directory as the executable
            &si,                        // Pointer to STARTUPINFO structure.
            &pi);                       // Pointer to PROCESS_INFORMATION structure.
    }
#elif defined AZ_PLATFORM_LINUX
    // KDAB_TODO
#else
    char path[MAXPATHLEN];
    unsigned int pathSize = MAXPATHLEN;
    if (_NSGetExecutablePath(path, &pathSize) == 0)
    {
        QProcess::startDetached(path, QStringList());
    }
#endif
}

//=========================================================================
// main
// [10/24/2012]
//=========================================================================
int main(int argc, char *argv[])
{
    bool failSilently = false;

    // We are launching this from application all the time.
    // When launching from the application, we don't want to show
    // any of our error messages, which might be useful at other times.
    // So, it passes along this fail_silently flag which lets us suppress
    // our messages without disrupting any other flow.
    for (int i=0; i < argc; ++i)
    {
        if (strcmp(argv[i],"-fail_silently") == 0)
        {
            failSilently = true;
        }
    }

    bool isCopyAndRunOnExit = false;

    if( IsDebuggerPresent() == false )
    {

#ifdef AZ_PLATFORM_WINDOWS
        char exeFileName[MAX_PATH];
        if (AZ::Utils::GetExecutablePath(exeFileName, AZ_ARRAY_SIZE(exeFileName)).m_pathStored == AZ::Utils::ExecutablePathResult::Success)
#elif defined AZ_PLATFORM_LINUX
        //KDAB_TODO
        char exeFileName[MAXPATHLEN];
#else
        char exeFileName[MAXPATHLEN];
        unsigned int pathSize = MAXPATHLEN;
        if (_NSGetExecutablePath(exeFileName, &pathSize) == 0)
#endif
        {
            if (!QString(exeFileName).contains(GRIDHUB_TSR_SUFFIX))
            {
                // this is a first time we run we need to run copy and run tool.
                isCopyAndRunOnExit = true;
            }
        }
    }

    if (isCopyAndRunOnExit == false)     // if we need to exit and run copy and run, just go there
    {
        bool isNeedToRelaunch = false;
        
        {
#ifdef AZ_PLATFORM_WINDOWS
            // Create a OS named mutex while the OS is running
            HANDLE hInstanceMutex = CreateMutex(NULL, TRUE, L"Global\\GridHub-Instance");
            AZ_Assert(hInstanceMutex!=NULL,"Failed to create OS mutex [GridHub-Instance]\n");
            if( hInstanceMutex != NULL && GetLastError() == ERROR_ALREADY_EXISTS)
            {
                return 0;
            }
#else
            {
                QSharedMemory fix("Global\\GridHub-Instance");
                fix.attach();
            }
            // Create a OS named shared memory segment while the OS is running
            QSharedMemory mem("Global\\GridHub-Instance");
            bool created = mem.create(32);
            AZ_Assert(created,"Failed to create OS mutex [GridHub-Instance]\n");
            if( !created )
            {
                return 0;
            }
#endif

            QGridHubApplication qtApp(argc, argv);
            qtApp.Initialize();
            qtApp.Execute();
            isNeedToRelaunch = qtApp.IsNeedToRelaunch();
            qtApp.Finalize();
#ifdef AZ_PLATFORM_WINDOWS
            ReleaseMutex(hInstanceMutex);
#endif
        }

        if (isNeedToRelaunch)
        {
            // Launch the original image, which will take care
            // of overwriting us and relaunch us in turn.
            RelaunchImage();
        }
    }
    else
    {
        // We may have been launched by the TSR due to image change,
        // so wait a little bit to give the TSR time to shutdown.
        AZStd::this_thread::sleep_for(AZStd::chrono::milliseconds(500));
        CopyAndRun(failSilently);
    }

    return 0;
}
