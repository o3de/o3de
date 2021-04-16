// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include "stdafx.h"
#include "Misc.h"

//
// Get current time in milliseconds
//
double MillisecondsNow()
{
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    double milliseconds = 0;

    if (s_use_qpc)
    {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        milliseconds = double(1000.0 * now.QuadPart) / s_frequency.QuadPart;
    }
    else
    {
        milliseconds = double(GetTickCount());
    }

    return milliseconds;
}

//
// Compute a hash of an array
//
size_t Hash(const void *ptr, size_t size, size_t result)
{
    for (size_t i = 0; i < size; ++i)
    {
        result = (result * 16777619) ^ ((char *)ptr)[i];
    }

    return result;
}



//
// Formats a string
//
std::string format(const char* format, ...)
{
    va_list args;
    va_start(args, format);
#ifndef _MSC_VER
    size_t size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    std::vsnprintf(buf.get(), size, format, args);
    va_end(args);
    return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
#else
    int size = _vscprintf(format, args);
    std::string result(++size, 0);
    vsnprintf_s((char*)result.data(), size, _TRUNCATE, format, args);
    va_end(args);
    return result;
#endif
}

void Trace(const std::string &str)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    OutputDebugStringA(str.c_str());
}

void Trace(const char* pFormat, ...)
{
    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);

    constexpr uint32_t MaxBufferSize = 2048;
    char str[MaxBufferSize];
    va_list args;

    // generate formatted string
    va_start(args, pFormat);
    vsnprintf_s(str, MaxBufferSize, pFormat, args);
    va_end(args);
    strcat_s(str, "\n");

    OutputDebugStringA(str);
}

//
//  Reads a file into a buffer
//
bool ReadFile(const char *name, char **data, size_t *size, bool isbinary)
{
    FILE *file;

    //Open file
    fopen_s(&file, name, isbinary ? "rb" : "r");
    if (!file)
    {
        return false;
    }

    //Get file length
    fseek(file, 0, SEEK_END);
    size_t fileLen = ftell(file);
    fseek(file, 0, SEEK_SET);

    // if ascii add one more char to accomodate for the \0
    if (!isbinary)
        fileLen++;

    //Allocate memory
    char *buffer = (char *)malloc(fileLen);
    if (!buffer)
    {
        fclose(file);
        return false;
    }

    //Read file contents into buffer
    size_t bytesRead = fread(buffer, 1, fileLen, file);
    fclose(file);

    if (!isbinary)
    {
        buffer[bytesRead] = 0;    
        fileLen = bytesRead;
    }

    *data = buffer;
    if (size != NULL)
        *size = fileLen;

    return true;
}

bool SaveFile(const char *name, void const*data, size_t size, bool isbinary)
{
    FILE *file;
    fopen_s(&file, name, isbinary ? "wb" : "w");
    if (file != NULL)
    {
        fwrite(data, size, 1, file);
        fclose(file);
        return true;
    }

    return false;
}



//
// Launch a process, captures stderr into a file
//
bool LaunchProcess(const std::string &commandLine, const std::string &filenameErr)
{
    char cmdLine[1024];
    strcpy_s<1024>(cmdLine, commandLine.c_str());

    // create a pipe to get possible errors from the compiler
    //
    HANDLE g_hChildStd_OUT_Rd = NULL;
    HANDLE g_hChildStd_OUT_Wr = NULL;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;
    if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
        return false;

    // launch compiler
    //
    PROCESS_INFORMATION pi = {};
    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = g_hChildStd_OUT_Wr;
    si.hStdOutput = g_hChildStd_OUT_Wr;
    si.wShowWindow = SW_HIDE;

    if (CreateProcessA(NULL, cmdLine, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(g_hChildStd_OUT_Wr);

        ULONG rc;
        if (GetExitCodeProcess(pi.hProcess, &rc))
        {
            if (rc == 0)
            {
                DeleteFileA(filenameErr.c_str());
                return true;
            }
            else
            {
                Trace(format("*** Process %s returned an error, see %s ***\n\n", cmdLine, filenameErr.c_str()));

                // save errors to disk
                std::ofstream ofs(filenameErr.c_str(), std::ofstream::out);

                for (;;)
                {
                    DWORD dwRead;
                    char chBuf[2049];
                    BOOL bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, 2048, &dwRead, NULL);
                    chBuf[dwRead] = 0;
                    if (!bSuccess || dwRead == 0) break;

                    Trace(chBuf);

                    ofs << chBuf;
                }

                ofs.close();
            }
        }

        CloseHandle(g_hChildStd_OUT_Rd);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        Trace(format("*** Can't launch: %s \n", cmdLine));
    }

    return false;
}

void GetXYZ(float *f, XMVECTOR v)
{
    f[0] = XMVectorGetX(v);
    f[1] = XMVectorGetY(v);
    f[2] = XMVectorGetZ(v);
}
