/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : implementation of the CLogFile class.

#include "EditorDefs.h"

#include "LogFile.h"

// Qt
#include <QTextEdit>
#include <QScrollBar>
#include <QListWidget>
#include <QGuiApplication>
#include <QScreen>

// Editor
#include "CryEdit.h"                // for CCryEditApp
#include "ProcessInfo.h"            // for ProcessMemInfo
#include "Controls/ConsoleSCB.h"    // for CConsoleSCB


#include <AzCore/std/ranges/ranges_algorithm.h>
#include <stdarg.h>

#if !defined(AZ_PLATFORM_WINDOWS)
#include <time.h>
#include <QTime>
#else
#include <Wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")
#endif

#if defined(AZ_PLATFORM_MAC)
#include <mach/clock.h>
#include <mach/mach.h>

#include <Carbon/Carbon.h>
#endif


// Static member variables
SANDBOX_API QListWidget* CLogFile::m_hWndListBox = nullptr;
SANDBOX_API QTextEdit* CLogFile::m_hWndEditBox = nullptr;
bool CLogFile::m_bShowMemUsage = false;
bool CLogFile::m_bIsQuitting = false;

//////////////////////////////////////////////////////////////////////////
SANDBOX_API void Error(const char* format, ...)
{
    va_list ArgList;

    va_start(ArgList, format);
    ErrorV(format, ArgList);
    va_end(ArgList);
}


//////////////////////////////////////////////////////////////////////////
SANDBOX_API void ErrorV(const char* format, va_list argList)
{
    char        szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);

    QString str = "####-ERROR-####: ";
    str += szBuffer;

    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_ERROR, "%s", str.toUtf8().data());

    if (!CCryEditApp::instance()->IsInTestMode() && !CCryEditApp::instance()->IsInExportMode() && !CCryEditApp::instance()->IsInLevelLoadTestMode())
    {
        if (gEnv && gEnv->pSystem)
        {
            gEnv->pSystem->ShowMessage(szBuffer, "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
        }
    }
}

//////////////////////////////////////////////////////////////////////////
SANDBOX_API void Warning(const char* format, ...)
{
    va_list ArgList;

    va_start(ArgList, format);
    WarningV(format, ArgList);
    va_end(ArgList);
}


//////////////////////////////////////////////////////////////////////////
SANDBOX_API void WarningV(const char* format, va_list argList)
{
    char        szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);

    CryWarning(VALIDATOR_MODULE_EDITOR, VALIDATOR_WARNING, "%s", szBuffer);

    bool bNoUI = false;
    ICVar* pCVar = gEnv->pConsole->GetCVar("sys_no_crash_dialog");
    if (pCVar->GetIVal() != 0)
    {
        bNoUI = true;
    }

    if (!CCryEditApp::instance()->IsInTestMode() && !CCryEditApp::instance()->IsInExportMode() && !bNoUI)
    {
        if (gEnv && gEnv->pSystem)
        {
            gEnv->pSystem->ShowMessage(szBuffer, "Warning", MB_OK | MB_ICONWARNING | MB_APPLMODAL);
        }
    }
}


//////////////////////////////////////////////////////////////////////////
SANDBOX_API void Log(const char* format, ...)
{
    va_list ArgList;
    va_start(ArgList, format);
    LogV(format, ArgList);
    va_end(ArgList);
}

//////////////////////////////////////////////////////////////////////////
SANDBOX_API void LogV(const char* format, va_list argList)
{
    char        szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);
    CLogFile::WriteLine(szBuffer);
}



//////////////////////////////////////////////////////////////////////////
const char* CLogFile::GetLogFileName()
{
    if (gEnv && gEnv->pLog)
    {
        // Return the path
        return gEnv->pLog->GetFileName();
    }
    else
    {
        return "";
    }
}

void CLogFile::AttachListBox(QListWidget* hWndListBox)
{
    m_hWndListBox = hWndListBox;
}

void CLogFile::AttachEditBox(QTextEdit* hWndEditBox)
{
    m_hWndEditBox = hWndEditBox;
}

void CLogFile::FormatLine(const char * format, ...)
{
    ////////////////////////////////////////////////////////////////////////
    // Write a line with printf style formatting
    ////////////////////////////////////////////////////////////////////////

    va_list ArgList;
    va_start(ArgList, format);
    FormatLineV(format, ArgList);
    va_end(ArgList);
}

void CLogFile::FormatLineV(const char * format, va_list argList)
{
    char szBuffer[MAX_LOGBUFFER_SIZE];
    azvsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, format, argList);
    CryLog("%s", szBuffer);
}

namespace LogFileInternal
{
#if defined(AZ_PLATFORM_WINDOWS)
    // Stores information about the OS queried from WMI
    struct OSInfo
    {
        // Stores the Name property from Win32_OperatingSystem
        AZStd::string m_name;
        // Stores the Version property from Win32_OperatingSystem
        AZStd::string m_version;
    };

    // This uses the GetVersionEx API which was deprecated in Windows 10
    // as a fall back to query the version information for Windows
    // On Windows 10 and after, this always returns the version as 6.2
    OSInfo QueryOSInfoUsingGetVersionEx()
    {
        OSInfo osInfo;

        OSVERSIONINFO osVersionInfo;
        osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        AZ_PUSH_DISABLE_WARNING(4996, "-Wunknown-warning-option")
            GetVersionEx(&osVersionInfo);
        AZ_POP_DISABLE_WARNING;

        // Default the name of the operating system to just Windows as the version information
        // is based on the manifest at the time application was built, which probably
        // does not match the current version of Windows that is running
        osInfo.m_name = "Windows";
        osInfo.m_version = AZStd::string::format("%ld.%ld", osVersionInfo.dwMajorVersion, osVersionInfo.dwMinorVersion);
        return osInfo;
    }

    // This specifically avoids using the GetVersion/GetVersionEx WinAPI
    // functions because those will only return the version of Windows
    // that was manifested at the time the application was built
    // and not the version of Windows that is currently running
    // https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getversion
    AZ::Outcome<OSInfo, AZStd::string> QueryOSInfoUsingWMI()
    {
        // Get the thread Id as an numeric value
        static_assert(sizeof(AZStd::thread_id) <= sizeof(uintptr_t), "Thread Id should less than a size of a pointer");
        // Using curly braces to zero initalize all bytes the uintptr_t
        // If the thread Id is < sizeof(uintptr_t), the high bytes would be zeroed out
        uintptr_t numericThreadId{};
        *reinterpret_cast<AZStd::thread_id*>(&numericThreadId) = AZStd::this_thread::get_id();

        OSInfo osInfo;
        // Query the Windows version using WMI
        HRESULT hResult = CoInitialize(nullptr);
        if (FAILED(hResult))
        {
            return AZ::Failure(AZStd::string::format(
                "Failed to initialize the Com library on thread %#" PRIxPTR ": %lu", numericThreadId, static_cast<unsigned long>(hResult)));
        }

        // Obtain the initial locator to Windows Management on a particular host computer.
        IWbemLocator* locator = nullptr;
        hResult = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
        if (FAILED(hResult))
        {
            return AZ::Failure(
                AZStd::string::format("Failed to create the Windows Management COM class: %lu", static_cast<unsigned long>(hResult)));
        }
        // Connect to the root\cimv2 namespace with the current user and obtain pointer pSvc to make IWbemServices calls.
        IWbemServices* services = nullptr;
        auto serverPath = SysAllocString(TEXT(R"(ROOT\CIMV2)"));
        hResult = locator->ConnectServer(serverPath, nullptr, nullptr, 0, 0, nullptr, nullptr, &services);
        SysFreeString(serverPath);

        if (FAILED(hResult))
        {
            return AZ::Failure(
                AZStd::string::format("Could not connect the WMI on the local machine: %lu", static_cast<unsigned long>(hResult)));
        }

        // Set the IWbemServices proxy so that impersonation of the user (client) occurs.
        hResult = CoSetProxyBlanket(
            services,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            nullptr,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            nullptr,
            EOAC_NONE);

        if (FAILED(hResult))
        {
            return AZ::Failure(
                AZStd::string::format("Cannot impersonate current user for proxy call: %lu", static_cast<unsigned long>(hResult)));
        }

        IEnumWbemClassObject* classObjectEnumerator = nullptr;

        // query the Name and Version property from the Win32_OperatingSystem class
        // https://learn.microsoft.com/en-us/windows/win32/cimwin32prov/win32-operatingsystem
        AZStd::wstring propertyQuery{ L"SELECT Name,Version FROM Win32_OperatingSystem" };

        // ExecQuery expects BSTR types
        auto query = SysAllocString(propertyQuery.c_str());
        auto language = SysAllocString(L"WQL");
        hResult =
            services->ExecQuery(language, query, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &classObjectEnumerator);
        SysFreeString(language);
        SysFreeString(query);

        if (FAILED(hResult))
        {
            return AZ::Failure(AZStd::string::format(
                "WQL query from Win32_OperatingSystem WMI class has failed: %lu", static_cast<unsigned long>(hResult)));
        }

        // prepare to enumerate the object, blocking until the objects are ready
        IWbemClassObject* classObject = nullptr;
        ULONG numResults = 0;
        hResult = classObjectEnumerator->Next(WBEM_INFINITE, 1, &classObject, &numResults);

        if (FAILED(hResult))
        {
            return AZ::Failure(AZStd::string::format(
                "Enumerating the CIM objects has failed with result code: %lu", static_cast<unsigned long>(hResult)));
        }
        else if (classObject == nullptr || numResults == 0)
        {
            return AZ::Failure(AZStd::string(
                "There are no CIM objects found when querying the Win32_OperatingSystem class"));
        }

        constexpr const wchar_t* namePropertyName{ L"Name" };
        constexpr const wchar_t* versionPropertyName{ L"Version" };
        // get the class object's property value
        VARIANT propertyValue;
        // Query the Name field
        if (FAILED(classObject->Get(namePropertyName, 0, &propertyValue, nullptr, nullptr)))
        {
            return AZ::Failure(AZStd::string::format(
                R"(Could not retrieve the ")" AZ_TRAIT_FORMAT_STRING_PRINTF_WSTRING R"(" property from the CIM object: %lu)", namePropertyName, static_cast<unsigned long>(hResult)));
        }

        // If the name is a binary string copy it over
        if ((propertyValue.vt & VT_BSTR) == VT_BSTR)
        {
            AZStd::to_string(osInfo.m_name, V_BSTR(&propertyValue));
        }
        // VariantClear must be called on the variant retrieved from `IWbemClassObject::Get`
        // according to the documentation:
        // https://learn.microsoft.com/en-us/windows/win32/api/wbemcli/nf-wbemcli-iwbemclassobject-get
        VariantClear(&propertyValue);

        // Query the Version field
        if (FAILED(classObject->Get(versionPropertyName, 0, &propertyValue, nullptr, nullptr)))
        {
            {
                return AZ::Failure(AZStd::string::format(
                    R"(Could not retrieve the ")" AZ_TRAIT_FORMAT_STRING_PRINTF_WSTRING R"(" property from the CIM object: %lu)", versionPropertyName, static_cast<unsigned long>(hResult)));
            }
        }

        // If the name is a binary string copy it over
        if ((propertyValue.vt & VT_BSTR) == VT_BSTR)
        {
            AZStd::to_string(osInfo.m_version, V_BSTR(&propertyValue));
        }
        // VariantClear must be called on the variant retrieved from `IWbemClassObject::Get`
        // according to the documentation:
        // https://learn.microsoft.com/en-us/windows/win32/api/wbemcli/nf-wbemcli-iwbemclassobject-get
        VariantClear(&propertyValue);

        if (classObject)
        {
            classObject->Release();
        }

        if (classObjectEnumerator)
        {
            classObjectEnumerator->Release();
        }

        if (services)
        {
            services->Release();
        }

        if (locator)
        {
            locator->Release();
        }

        CoUninitialize();

        return osInfo;
    }

    OSInfo QueryOSInfo()
    {
        auto queryOSInfoOutcome = QueryOSInfoUsingWMI();
        if (!queryOSInfoOutcome)
        {
            CryLog("Failed to query Windows version info using WMI with error: %s.\n"
                "Falling back to using GetVersionEx", queryOSInfoOutcome.GetError().c_str());
            return QueryOSInfoUsingGetVersionEx();
        }
        return queryOSInfoOutcome.GetValue();
    }

#endif
}

void CLogFile::AboutSystem()
{
#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
    //////////////////////////////////////////////////////////////////////
    // Write the system informations to the log
    //////////////////////////////////////////////////////////////////////
    char szBuffer[MAX_LOGBUFFER_SIZE];
    //wchar_t szCPUModel[64];
    MEMORYSTATUS MemoryStatus;
#endif // defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)

#if defined(AZ_PLATFORM_WINDOWS)
    wchar_t szLanguageBufferW[64];
    DEVMODE DisplayConfig;

    //////////////////////////////////////////////////////////////////////
    // Display editor and Windows version
    //////////////////////////////////////////////////////////////////////

    // Get system language
    GetLocaleInfoW(LOCALE_SYSTEM_DEFAULT, LOCALE_SENGLANGUAGE,
        szLanguageBufferW, sizeof(szLanguageBufferW));
    AZStd::string szLanguageBuffer;
    AZStd::to_string(szLanguageBuffer, szLanguageBufferW);

    // Format and send OS information line
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, "Current Language: %s ", szLanguageBuffer.c_str());
    CryLog("%s", szBuffer);
#else
    QLocale locale;
    CryLog("Current Language: %s (%s)", qPrintable(QLocale::languageToString(locale.language())), qPrintable(QLocale::countryToString(locale.country())));
#endif


#if defined(AZ_PLATFORM_WINDOWS)
    // Format and send OS version line
    auto osInfo = LogFileInternal::QueryOSInfo();
    QString str = QString("%1 %2").arg(osInfo.m_name.c_str()).arg(osInfo.m_version.c_str());

    //////////////////////////////////////////////////////////////////////
    // Show Windows directory
    //////////////////////////////////////////////////////////////////////

    CryLog("%s", str.toUtf8().data());
#elif defined(AZ_PLATFORM_LINUX)
    // TODO: Add more detail about the current Linux Distro
    CryLog("Linux");
#elif AZ_TRAIT_OS_PLATFORM_APPLE
    QString operatingSystemName;
    if (QSysInfo::MacintoshVersion >= Q_MV_OSX(10, 12))
    {
        operatingSystemName = "macOS ";
    }
    else
    {
        operatingSystemName = "OS X ";
    }

    int majorVersion = 0;
    int minorVersion = 0;
    AZ_PUSH_DISABLE_WARNING(, "-Wdeprecated-declarations")
    Gestalt(gestaltSystemVersionMajor, &majorVersion);
    Gestalt(gestaltSystemVersionMinor, &minorVersion);
    AZ_POP_DISABLE_WARNING

    CryLog("%s - %d.%d", qPrintable(operatingSystemName), majorVersion, minorVersion);
#else
    CryLog("Unknown Operating System");
#endif

    //////////////////////////////////////////////////////////////////////
    // Send system time & date
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS)
    str = "Local time is ";
    azstrtime(szBuffer);
    str += szBuffer;
    str += " ";
    azstrdate(szBuffer);
    str += szBuffer;
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, ", system running for %ld minutes", GetTickCount() / 60000);
    str += szBuffer;
    CryLog("%s", str.toUtf8().data());
#else
    struct timespec ts;
#if AZ_TRAIT_OS_PLATFORM_APPLE
    clock_serv_t cclock;
    mach_timespec_t mts;
    host_get_clock_service(mach_host_self(), SYSTEM_CLOCK, &cclock);
    clock_get_time(cclock, &mts);
    mach_port_deallocate(mach_task_self(), cclock);
    ts.tv_sec = mts.tv_sec;
#else
    clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
    CryLog("Local time is %s, system running for %ld minutes", qPrintable(QTime::currentTime().toString("hh:mm:ss")), ts.tv_sec / 60);
#endif

    //////////////////////////////////////////////////////////////////////
    // Send system memory status
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS) || defined(AZ_PLATFORM_LINUX)
    GlobalMemoryStatus(&MemoryStatus);
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, "%zdMB phys. memory installed, %zdMB paging available",
        MemoryStatus.dwTotalPhys / 1048576 + 1,
        MemoryStatus.dwAvailPageFile / 1048576);
    CryLog("%s", szBuffer);
#elif defined(AZ_PLATFORM_LINUX)
    //KDAB_TODO
#else

    SInt32 mb = 0, lmb = 0;
    AZ_PUSH_DISABLE_WARNING(, "-Wdeprecated-declarations")
    Gestalt(gestaltPhysicalRAMSizeInMegabytes, &mb);
    Gestalt(gestaltLogicalRAMSize, &lmb);
    AZ_POP_DISABLE_WARNING
    CryLog("%dMB phys. memory installed, %dMB paging available", mb, lmb);
#endif

    //////////////////////////////////////////////////////////////////////
    // Send display settings
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS)
    EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &DisplayConfig);
    GetPrivateProfileStringW(L"boot.description", L"display.drv",
        L"(Unknown graphics card)", szLanguageBufferW, sizeof(szLanguageBufferW),
        L"system.ini");
    AZStd::to_string(szLanguageBuffer, szLanguageBufferW);
    azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, "Current display mode is %ldx%ldx%ld, %s",
        DisplayConfig.dmPelsWidth, DisplayConfig.dmPelsHeight,
        DisplayConfig.dmBitsPerPel, szLanguageBuffer.c_str());
    CryLog("%s", szBuffer);
#else
    auto screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        CryLog("Current display mode is %dx%dx%d, %s", screen->size().width(), screen->size().height(), screen->depth(), qPrintable(screen->name()));
    }
#endif

    //////////////////////////////////////////////////////////////////////
    // Send input device configuration
    //////////////////////////////////////////////////////////////////////

#if defined(AZ_PLATFORM_WINDOWS)
    str = "";
    // Detect the keyboard type
    switch (GetKeyboardType(0))
    {
    case 1:
        str = "IBM PC/XT (83-key)";
        break;
    case 2:
        str = "ICO (102-key)";
        break;
    case 3:
        str = "IBM PC/AT (84-key)";
        break;
    case 4:
        str = "IBM enhanced (101/102-key)";
        break;
    case 5:
        str = "Nokia 1050";
        break;
    case 6:
        str = "Nokia 9140";
        break;
    case 7:
        str = "Japanese";
        break;
    default:
        str = "Unknown";
        break;
    }

    // Any mouse attached ?
    if (!GetSystemMetrics(SM_MOUSEPRESENT))
    {
        CryLog("%s", (str + " keyboard and no mouse installed").toUtf8().data());
    }
    else
    {
        azsnprintf(szBuffer, MAX_LOGBUFFER_SIZE, " keyboard and %i+ button mouse installed",
            GetSystemMetrics(SM_CMOUSEBUTTONS));
        str += szBuffer;
        CryLog("%s", str.toUtf8().data());
    }

    CryLog("--------------------------------------------------------------------------------");
#endif
}

//////////////////////////////////////////////////////////////////////////
QString CLogFile::GetMemUsage()
{
    AZ::ProcessMemInfo mi;
    AZ::QueryMemInfo(mi);
    constexpr int MB = 1024 * 1024;

    QString str;
    str = QStringLiteral("Memory=%1Mb, Pagefile=%2Mb").arg(mi.m_workingSet / MB).arg(mi.m_pagefileUsage / MB);
    //FormatLine( "PeakWorkingSet=%dMb, PeakPagefileUsage=%dMb",pc.PeakWorkingSetSize/MB,pc.PeakPagefileUsage/MB );
    //FormatLine( "PagedPoolUsage=%d",pc.QuotaPagedPoolUsage/MB );
    //FormatLine( "NonPagedPoolUsage=%d",pc.QuotaNonPagedPoolUsage/MB );

    return str;
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::WriteLine(const char* pszString)
{
    CryLog("%s", pszString);
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::WriteString(const char* pszString)
{
    if (gEnv && gEnv->pLog)
    {
        gEnv->pLog->LogAppendWithPrevLine("%s", pszString);
    }
}

static inline QString CopyAndRemoveColorCode(AZStd::string_view sText)
{
    QByteArray ret(sText.data(), static_cast<int>(sText.size()));      // alloc and copy

    char* s, * d;

    s = ret.data();
    d = s;

    // remove color code in place
    while (*s != 0)
    {
        if (*s == '$' && *(s + 1) >= '0' && *(s + 1) <= '9')
        {
            s += 2;
            continue;
        }

        *d++ = *s++;
    }

    ret.resize(static_cast<int>(d - ret.data()));

    return QString::fromLatin1(ret);
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::OnWriteToConsole(AZStd::string_view sText, bool bNewLine)
{
    if (!gEnv)
    {
        return;
    }
    //  if (GetIEditor()->IsInGameMode())
    //  return;

    // Skip any non character.
    auto searchIt = AZStd::ranges::find_if(sText, [](char element) { return element >= 32; });
    sText = AZStd::string_view(searchIt, sText.end());

    // If we have a listbox attached, also output the string to this listbox
    if (m_hWndListBox)
    {
        QString str = CopyAndRemoveColorCode(sText);        // editor prinout doesn't support color coded log messages

        if (m_bShowMemUsage)
        {
            str = QString("(") + GetMemUsage() + ")" + str;
        }

        // Add the string to the listbox
        m_hWndListBox->addItem(str);

        // Make sure the recently added string is visible
        m_hWndListBox->scrollToItem(m_hWndListBox->item(m_hWndListBox->count() - 1));

        if (m_hWndEditBox)
        {
            static int nCounter = 0;
            if (nCounter++ > 500)
            {
                // Clean Edit box every 500 lines.
                nCounter = 0;
                m_hWndEditBox->clear();
            }

            // remember selection and the top row
            int len = m_hWndEditBox->document()->toPlainText().length();
            int top = 0;
            int from = m_hWndEditBox->textCursor().selectionStart();
            int to = from + m_hWndEditBox->textCursor().selectionEnd();
            bool keepPos = false;

            if (from != len || to != len)
            {
                keepPos = m_hWndEditBox->hasFocus();
                if (keepPos)
                {
                    top = m_hWndEditBox->verticalScrollBar()->value();

                }
                QTextCursor cur = m_hWndEditBox->textCursor();
                cur.setPosition(len);
                m_hWndEditBox->setTextCursor(cur);
            }
            if (bNewLine)
            {
                str = QString("\r\n") + str;
                str = str.trimmed();
            }
            m_hWndEditBox->textCursor().insertText(str);

            // restore selection and the top line
            if (keepPos)
            {
                QTextCursor cur = m_hWndEditBox->textCursor();
                cur.setPosition(from);
                cur.setPosition(to, QTextCursor::KeepAnchor);
                m_hWndEditBox->setTextCursor(cur);
                m_hWndEditBox->verticalScrollBar()->setValue(top);
            }
        }
    }

    if (CConsoleSCB::GetCreatedInstance())
    {
        const QString qtText = QString::fromUtf8(sText.data(), static_cast<int>(sText.size()));
        QString sOutLine(qtText);

        if (gSettings.bShowTimeInConsole)
        {
            char sTime[128];
            time_t ltime;
            time(&ltime);
            struct tm today;
#if AZ_TRAIT_USE_SECURE_CRT_FUNCTIONS
            localtime_s(&today, &ltime);
#else
            today = *localtime(&ltime);
#endif
            strftime(sTime, sizeof(sTime), "<%H:%M:%S> ", &today);
            sOutLine = sTime;
            sOutLine += qtText;
        }

        CConsoleSCB::GetCreatedInstance()->AddToConsole(sOutLine, bNewLine);
    }
    else
    {
        // add to intermediate messages until an instance of CConsoleSCB exists
        CConsoleSCB::AddToPendingLines(QString::fromUtf8(sText.data(), static_cast<int>(sText.size())), bNewLine);
    }

    //////////////////////////////////////////////////////////////////////////
    // Look for exit messages while writing to the console
    //////////////////////////////////////////////////////////////////////////
    if (gEnv && gEnv->pSystem && !gEnv->pSystem->IsQuitting() && !m_bIsQuitting)
    {
        if (QCoreApplication::closingDown())
        {
            m_bIsQuitting = true;
            CCryEditApp::instance()->ExitInstance();
            m_bIsQuitting = false;
        }
    }
}

//////////////////////////////////////////////////////////////////////////
void CLogFile::OnWriteToFile([[maybe_unused]] AZStd::string_view sText, [[maybe_unused]] bool bNewLine)
{
}
