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


#include "ProjectDefines.h"

#if defined(CRY_ENABLE_RC_HELPER)

#include "ResourceCompilerHelper.h"
#include "EngineSettingsManager.h"

// When complining CryTiffPlugin the mayaAssert.h is included that defined
// Assert as _Assert. This wreaks havoc with AZ_Assert since under the covers
// it calls AzCore::Debug::Trace::Assert, which gets transformed bo
// Trace::_Assert, which does not exist. Gotta love macros. Undefine Assert
// before we include semaphore so that it can compile correctly
#if defined(Assert)
#undef Assert
#endif

#include <AzCore/std/parallel/semaphore.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Utils/Utils.h>

#if defined(AZ_PLATFORM_WINDOWS)
#include <windows.h>
#include <shellapi.h> // ShellExecuteW()
#endif

#if AZ_TRAIT_OS_PLATFORM_APPLE
#include "AppleSpecific.h"
#include <CoreFoundation/CoreFoundation.h>
#else
#undef RC_EXECUTABLE
#define RC_EXECUTABLE "rc.exe"
#endif

#include <assert.h>
#include <string> // lawsonn - we use std::string internally
#include <sstream>

namespace
{
    class LineStreamBuffer
    {
    public:
        template <typename T>
        LineStreamBuffer(T* object, void (T::* method)(const char* line))
            : m_charCount(0)
            , m_bTruncated(false)
        {
            m_target = new Target<T>(object, method);
        }

        ~LineStreamBuffer()
        {
            Flush();
            delete m_target;
        }

        void HandleText(const char* text, int length)
        {
            const char* pos = text;
            while (pos - text < length)
            {
                const char* start = pos;

                while (pos - text < length && *pos != '\n' && *pos != '\r')
                {
                    ++pos;
                }

                size_t n = pos - start;
                if (m_charCount + n > kMaxCharCount)
                {
                    n = kMaxCharCount - m_charCount;
                    m_bTruncated = true;
                }
                memcpy(&m_buffer[m_charCount], start, n);
                m_charCount += n;

                if (pos - text < length)
                {
                    Flush();
                    while (pos - text < length && (*pos == '\n' || *pos == '\r'))
                    {
                        ++pos;
                    }
                }
            }
        }

        void Flush()
        {
            if (m_charCount > 0)
            {
                m_buffer[m_charCount] = 0;
                m_target->Call(m_buffer);
                m_charCount = 0;
            }
        }

        bool IsTruncated() const
        {
            return m_bTruncated;
        }

    private:
        struct ITarget
        {
            virtual ~ITarget() {}
            virtual void Call(const char* line) = 0;
        };
        template <typename T>
        struct Target
            : public ITarget
        {
        public:
            Target(T* object, void (T::* method)(const char* line))
                : object(object)
                , method(method) {}
            virtual void Call(const char* line)
            {
                (object->*method)(line);
            }
        private:
            T* object;
            void (T::* method)(const char* line);
        };

        ITarget* m_target;
        size_t m_charCount;
        static const size_t kMaxCharCount = 2047;
        char m_buffer[kMaxCharCount + 1];
        bool m_bTruncated;
    };

#if !defined(AZ_PLATFORM_WINDOWS)
    void MessageBoxW(int, const wchar_t* header, const wchar_t* message, unsigned long)
    {
#if AZ_TRAIT_OS_PLATFORM_APPLE
        CFStringEncoding encoding = (CFByteOrderLittleEndian == CFByteOrderGetCurrent()) ?
            kCFStringEncodingUTF32LE : kCFStringEncodingUTF32BE;
        CFStringRef header_ref = CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(header), wcslen(header) * sizeof(wchar_t), encoding, false);
        CFStringRef message_ref = CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(message), wcslen(message) * sizeof(wchar_t), encoding, false);

        CFOptionFlags result;  //result code from the message box

        CFUserNotificationDisplayAlert(0, kCFUserNotificationStopAlertLevel, 0, 0, 0, header_ref, message_ref, 0, 0, 0, &result);

        CFRelease(header_ref);
        CFRelease(message_ref);
#endif
    }
#endif
}

namespace
{
    class RcLock
    {
    public:
        RcLock()
            : m_cs(0u, 1u)
        {
            m_cs.release();
        }
        ~RcLock()
        {
        }

        void Lock()
        {
            m_cs.acquire();
        }
        void Unlock()
        {
            m_cs.release();
        }

    private:
        AZStd::semaphore m_cs;
    };


    template<class LockClass>
    class RcAutoLock
    {
    public:
        RcAutoLock(LockClass& lock)
            : m_lock(lock)
        {
            m_lock.Lock();
        }
        ~RcAutoLock()
        {
            m_lock.Unlock();
        }

    private:
        RcAutoLock();
        RcAutoLock(const RcAutoLock<LockClass>&);
        RcAutoLock<LockClass>& operator =(const RcAutoLock<LockClass>&);

    private:
        LockClass& m_lock;
    };


    HANDLE s_rcProcessHandle = 0;
    RcLock s_rcProcessHandleLock;
}

//////////////////////////////////////////////////////////////////////////
static void ShowMessageBoxRcNotFound([[maybe_unused]] const wchar_t* const szCmdLine, [[maybe_unused]] const wchar_t* const szDir)
{
    SettingsManagerHelpers::CFixedString<wchar_t, MAX_PATH* 4 + 150> tmp;

    tmp.append(L"The resource compiler (RC.EXE) was not found.");
    MessageBoxW(0, tmp.c_str(), L"Error", MB_ICONERROR | MB_OK);
}


//////////////////////////////////////////////////////////////////////////
namespace
{
    class ResourceCompilerLineHandler
    {
    public:
        ResourceCompilerLineHandler(IResourceCompilerListener* listener)
            : m_listener(listener)
        {
        }

        void HandleLine(const char* line)
        {
            if (!m_listener || !line)
            {
                return;
            }

            // check the first three characters to see if it's a warning or error.
            bool bHasPrefix;
            IResourceCompilerListener::MessageSeverity severity;
            if ((line[0] == 'E') && (line[1] == ':') && (line[2] == ' '))
            {
                bHasPrefix = true;
                severity = IResourceCompilerListener::MessageSeverity_Error;
                line += 3; // skip the prefix
            }
            else if ((line[0] == 'W') && (line[1] == ':') && (line[2] == ' '))
            {
                bHasPrefix = true;
                severity = IResourceCompilerListener::MessageSeverity_Warning;
                line += 3; // skip the prefix
            }
            else if ((line[0] == ' ') && (line[1] == ' ') && (line[2] == ' '))
            {
                bHasPrefix = true;
                severity = IResourceCompilerListener::MessageSeverity_Info;
                line += 3; // skip the prefix
            }
            else
            {
                bHasPrefix = false;
                severity = IResourceCompilerListener::MessageSeverity_Info;
            }

            if (bHasPrefix)
            {
                // skip thread info "%d>", if present
                {
                    const char* p = line;
                    while (*p == ' ')
                    {
                        ++p;
                    }
                    if (isdigit(*p))
                    {
                        while (isdigit(*p))
                        {
                            ++p;
                        }
                        if (*p == '>')
                        {
                            line = p + 1;
                        }
                    }
                }

                // skip time info "%d:%d", if present
                {
                    const char* p = line;
                    while (*p == ' ')
                    {
                        ++p;
                    }
                    if (isdigit(*p))
                    {
                        while (isdigit(*p))
                        {
                            ++p;
                        }
                        if (*p == ':')
                        {
                            ++p;
                            if (isdigit(*p))
                            {
                                while (isdigit(*p))
                                {
                                    ++p;
                                }
                                while (*p == ' ')
                                {
                                    ++p;
                                }
                                line = p;
                            }
                        }
                    }
                }
            }

            m_listener->OnRCMessage(severity, line);
        }

    private:
        IResourceCompilerListener* m_listener;
    };

    // we now support macros like #ENGINEROOT# in the string:
    void replaceAllInStringInPlace(std::string& inOut, const char* findValue, const char* replaceValue)
    {
        if (!findValue)
        {
            return;
        }

        if (!replaceValue)
        {
            return;
        }

        std::string::size_type pos = std::string::npos;
        std::string::size_type replaceLen = strlen(findValue);

        while ((pos = inOut.find(findValue)) != std::string::npos)
        {
            inOut.replace(pos, replaceLen, replaceValue);
        }
    }

    // given a string that contains macros (like #ENGINEROOT#), eliminate the macros and replace them with the real data.
    // note that in the 'remote' implementation, these macros are sent to the remote RC.  It can then expand them for its own environment
    // but in a local RC, these macros are expanded by the local environment.
    void expandMacros(const char* inputString, char* outputString, std::size_t bufferSize)
    {
        if (!inputString)
        {
            return;
        }

        if (!outputString)
        {
            return;
        }

        AZStd::string_view rootFolder;
        AZ::ComponentApplicationBus::BroadcastResult(rootFolder, &AZ::ComponentApplicationRequests::GetAppRoot);

        std::string finalString(inputString);
        const AZStd::string rootFolderStr = rootFolder.data();
        replaceAllInStringInPlace(finalString, "#ENGINEROOT#", rootFolderStr.c_str());
        // put additional replacements here.

        azstrcpy(outputString, bufferSize, finalString.c_str());
    }
}

//////////////////////////////////////////////////////////////////////////
IResourceCompilerHelper::ERcCallResult CResourceCompilerHelper::CallResourceCompiler(
    const char* szFileName,
    const char* szAdditionalSettings,
    IResourceCompilerListener* listener,
    bool bMayShowWindow,
    bool bSilent,
    bool bNoUserDialog,
    const wchar_t* szWorkingDirectory,
    [[maybe_unused]] const wchar_t* szRootPath)
{
#if defined(AZ_PLATFORM_WINDOWS)
    HANDLE hChildStdOutRd, hChildStdOutWr;
    HANDLE hChildStdInRd, hChildStdInWr;
    PROCESS_INFORMATION pi;
#else
    FILE* hChildStdOutRd;
#endif

    {
        RcAutoLock<RcLock> lock(s_rcProcessHandleLock);

        // make command for execution
        SettingsManagerHelpers::CFixedString<wchar_t, MAX_PATH* 3> wRemoteCmdLine;


        if (!szAdditionalSettings)
        {
            szAdditionalSettings = "";
        }

        // expand the additioanl settings.
        char szActualFileName[512] = {0};
        char szActualAdditionalSettings[512] = {0};

        expandMacros(szFileName, szActualFileName, 512);
        expandMacros(szAdditionalSettings, szActualAdditionalSettings, 512);

        CSettingsManagerTools smTools = CSettingsManagerTools(); // moved this line to after macro expansion to avoid multiple of these existing at once.

        AZStd::string_view exeFolderName;
        AZ::ComponentApplicationBus::BroadcastResult(exeFolderName, &AZ::ComponentApplicationRequests::GetExecutableFolder);

        wchar_t szRegSettingsBuffer[1024];
        smTools.GetEngineSettingsManager()->GetValueByRef("RC_Parameters", SettingsManagerHelpers::CWCharBuffer(szRegSettingsBuffer, sizeof(szRegSettingsBuffer)));
        bool enableSourceControl = true;
        smTools.GetEngineSettingsManager()->GetValueByRef("RC_EnableSourceControl", enableSourceControl);

        wRemoteCmdLine.appendAscii("\"");
        wRemoteCmdLine.appendAscii(exeFolderName.data(), exeFolderName.size());
        wRemoteCmdLine.appendAscii("/");
        wRemoteCmdLine.appendAscii(RC_EXECUTABLE);
        wRemoteCmdLine.appendAscii("\"");

        if (!enableSourceControl)
        {
            wRemoteCmdLine.appendAscii(" -nosourcecontrol ");
        }

        if (!szFileName)
        {
            wRemoteCmdLine.appendAscii(" -userdialog=0 ");
            wRemoteCmdLine.appendAscii(szActualAdditionalSettings);
            wRemoteCmdLine.appendAscii(" ");
            wRemoteCmdLine.append(szRegSettingsBuffer);
        }
        else
        {
            wRemoteCmdLine.appendAscii(" \"");
            wRemoteCmdLine.appendAscii(szActualFileName);
            wRemoteCmdLine.appendAscii("\"");
            wRemoteCmdLine.appendAscii(bNoUserDialog ? " -userdialog=0 " : " -userdialog=1 ");
            wRemoteCmdLine.appendAscii(szActualAdditionalSettings);
            wRemoteCmdLine.appendAscii(" ");
            wRemoteCmdLine.append(szRegSettingsBuffer);
        }

        // Create a pipe to read the stdout of the RC.
        SECURITY_ATTRIBUTES saAttr;
        if (listener)
        {
#if defined(AZ_PLATFORM_WINDOWS)
            ZeroMemory(&saAttr, sizeof(saAttr));
            saAttr.bInheritHandle = TRUE;
            saAttr.lpSecurityDescriptor = 0;
            CreatePipe(&hChildStdOutRd, &hChildStdOutWr, &saAttr, 0);
            SetHandleInformation(hChildStdOutRd, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN
            CreatePipe(&hChildStdInRd, &hChildStdInWr, &saAttr, 0);
            SetHandleInformation(hChildStdInWr, HANDLE_FLAG_INHERIT, 0); // Need to do this according to MSDN
#endif
        }

#if defined(AZ_PLATFORM_WINDOWS)
        STARTUPINFOW si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwX = 100;
        si.dwY = 100;
        if (listener)
        {
            si.hStdError = hChildStdOutWr;
            si.hStdOutput = hChildStdOutWr;
            si.hStdInput = hChildStdInRd;
            si.dwFlags = STARTF_USEPOSITION | STARTF_USESTDHANDLES;
        }
        else
        {
            si.dwFlags = STARTF_USEPOSITION;
        }

        ZeroMemory(&pi, sizeof(pi));
#endif

        bool bShowWindow;
        if (bMayShowWindow)
        {
            wchar_t buffer[20];
            smTools.GetEngineSettingsManager()->GetValueByRef("ShowWindow", SettingsManagerHelpers::CWCharBuffer(buffer, sizeof(buffer)));
            bShowWindow = (wcscmp(buffer, L"true") == 0);
        }
        else
        {
            bShowWindow = false;
        }

#if defined(AZ_PLATFORM_WINDOWS)
        const wchar_t* szStartingDirectory = szWorkingDirectory;
        if (!szStartingDirectory)
        {
            char currentDirectory[MAX_PATH];
            AZ::Utils::GetExecutableDirectory(currentDirectory, MAX_PATH);
            SettingsManagerHelpers::CFixedString<wchar_t, MAX_PATH> wCurrentDirectory;
            wCurrentDirectory.appendAscii(currentDirectory);
            szStartingDirectory = wCurrentDirectory.c_str();
        }
        

        if (!CreateProcessW(
                NULL,               // No module name (use command line).
                const_cast<wchar_t*>(wRemoteCmdLine.c_str()), // Command line.
                NULL,               // Process handle not inheritable.
                NULL,               // Thread handle not inheritable.
                TRUE,               // Set handle inheritance to TRUE.
                bShowWindow ? 0 : CREATE_NO_WINDOW, // creation flags.
                NULL,               // Use parent's environment block.
                szStartingDirectory, // Set starting directory.
                &si,                // Pointer to STARTUPINFO structure.
                &pi))               // Pointer to PROCESS_INFORMATION structure.
        {
            // The following  code block is commented out instead of being deleted
            // because it's good to have at hand for a debugging session.
    #if 0
            const size_t charsInMessageBuffer = 32768;   // msdn about FormatMessage(): "The output buffer cannot be larger than 64K bytes."
            wchar_t szMessageBuffer[charsInMessageBuffer] = L"";
            FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, szMessageBuffer, charsInMessageBuffer, NULL);
            GetCurrentDirectoryW(charsInMessageBuffer, szMessageBuffer);
    #endif

            if (!bSilent)
            {
                ShowMessageBoxRcNotFound(wRemoteCmdLine.c_str(), szStartingDirectory);
            }

            return eRcCallResult_notFound;
        }

        s_rcProcessHandle = pi.hProcess;
#else
        int fd = open(".", O_RDONLY);
        char remoteCmdLineUtf8[MAX_PATH * 8];
        char workingDirectory[MAX_PATH * 8];
        ConvertUtf16ToUtf8(wRemoteCmdLine.c_str(), SettingsManagerHelpers::CCharBuffer(remoteCmdLineUtf8, MAX_PATH * 8));
        if (szWorkingDirectory)
        {
            ConvertUtf16ToUtf8(szWorkingDirectory, SettingsManagerHelpers::CCharBuffer(workingDirectory, MAX_PATH * 8));
            chdir(workingDirectory);
        }
        hChildStdOutRd = popen(remoteCmdLineUtf8, "r");
        fchdir(fd);
        if (hChildStdOutRd == nullptr)
        {
            if (!bSilent)
            {
                ShowMessageBoxRcNotFound(wRemoteCmdLine.c_str(), szWorkingDirectory);
            }
            return eRcCallResult_notFound;
        }
#endif
    }

    bool bFailedToReadOutput = false;

    if (listener)
    {
#if defined(AZ_PLATFORM_WINDOWS)
        // Close the pipe that writes to the child process, since we don't actually have any input for it.
        CloseHandle(hChildStdInWr);

        // Read all the output from the child process.
        CloseHandle(hChildStdOutWr);
#endif
        ResourceCompilerLineHandler lineHandler(listener);
        LineStreamBuffer lineBuffer(&lineHandler, &ResourceCompilerLineHandler::HandleLine);
        for (;; )
        {
            char buffer[2048];
            DWORD bytesRead;
#if defined(AZ_PLATFORM_WINDOWS)
            if (!ReadFile(hChildStdOutRd, buffer, sizeof(buffer), &bytesRead, NULL) || (bytesRead == 0))
#else
            if (fgets(buffer, sizeof(buffer), hChildStdOutRd) == nullptr || (bytesRead = strlen(buffer) == 0))
#endif
            {
                break;
            }
            lineBuffer.HandleText(buffer, bytesRead);
        }

        bFailedToReadOutput = lineBuffer.IsTruncated();
    }

#if defined(AZ_PLATFORM_WINDOWS)
    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);
#else
    DWORD exitCode = pclose(hChildStdOutRd);
#endif

#if defined(AZ_PLATFORM_WINDOWS)
    RcAutoLock<RcLock> lock(s_rcProcessHandleLock);
    s_rcProcessHandle = 0;

    DWORD exitCode = eRcExitCode_Error;
    if (bFailedToReadOutput || GetExitCodeProcess(pi.hProcess, &exitCode) == 0)
    {
        exitCode = eRcExitCode_Error;
    }

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
#endif

    return ConvertResourceCompilerExitCodeToResultCode(exitCode);
}


#endif //(CRY_ENABLE_RC_HELPER)
