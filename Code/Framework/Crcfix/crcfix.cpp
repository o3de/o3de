/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/PlatformIncl.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Memory/AllocationRecords.h>
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/Utils/Utils.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <sys/stat.h>

int g_totalFixTimeMs = 0;
int g_longestFixTimeMs = 0;

class Filename
{
    char    fullpath[MAX_PATH];
    char    drive[_MAX_DRIVE];
    char    dir[_MAX_DIR];
    char    fname[_MAX_FNAME];
    char    ext[_MAX_EXT];

public:
    Filename()
    {
        fullpath[0] = drive[0] = dir[0] = fname[0] = ext[0] = 0;
    }

    Filename(const AZStd::string& filename)
    {
        _splitpath(filename.c_str(), drive, dir, fname, ext);
        strcpy(fullpath, filename.c_str());
    }

    void        SetExt(const char* pExt)        { strcpy(ext, pExt); _makepath(fullpath, drive, dir, fname, pExt); }
    const char* GetFullPath() const             { return fullpath; }
    bool        Exists() const                  { return _access(fullpath, 0) == 0; }
    bool        IsReadOnly() const              { return _access(fullpath, 6) == -1; }
    bool        SetReadOnly() const             { return _chmod(fullpath, _S_IREAD) == 0; }
    bool        SetWritable() const             { return _chmod(fullpath, _S_IREAD | _S_IWRITE) == 0; }
    bool        Delete() const                  { return remove(fullpath) == 0; }
    bool        Rename(const char* fn2) const   { return MoveFileEx(fullpath, fn2, MOVEFILE_COPY_ALLOWED|MOVEFILE_REPLACE_EXISTING) == 0; }
    bool        Copy(const char* dest) const    { return ::CopyFileA(fullpath, dest, FALSE) == TRUE; }
};

class CRCfix
{
    int             lastchar;
    int             linenum;

public:
    void    SkipToEOL(FILE* infile);
    char*   GetToken(FILE* infile, FILE* outfile);
    void    GetPreviousCRC(char* token, FILE* infile);
    int     Fix(Filename srce);
};

void FixFiles(const AZStd::string& dir, const AZStd::string& files, FILETIME* pLastRun, bool verbose, int& nFound, int& nProcessed, int& nFixed, int& nFailed)
{
    CRCfix fixer;
    WIN32_FIND_DATA wfd;
    HANDLE hFind;
    hFind = FindFirstFile((dir + files).c_str(), &wfd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
                if (verbose)
                {
                    AZ_TracePrintf("CrcFix", "\tProcessing %s ...", wfd.cFileName);
                }
                nFound++;

                int n = 0;
                if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) == 0 && (!pLastRun || CompareFileTime(pLastRun, &wfd.ftLastWriteTime) <= 0))
                {
                    n = fixer.Fix(Filename(dir + "\\" + wfd.cFileName));
                    nProcessed++;
                }
                if (n < 0)
                {
                    nFailed++;
                    if (verbose)
                    {
                        AZ_TracePrintf("CrcFix", "Failed\n");
                    }
                }
                else
                {
                    if (verbose)
                    {
                        AZ_TracePrintf("CrcFix", n > 0 ? "Done\n" : (wfd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0 ? "ReadOnly\n" : "Unchanged\n");
                    }
                    nFixed += n;
                }
            }
        } while (FindNextFile(hFind, &wfd));
    }
    FindClose(hFind);
}

void FixDirectories(const AZStd::string& dirs, const AZStd::string& files, FILETIME* pLastRun, bool verbose, int& nFound, int& nProcessed, int& nFixed, int& nFailed)
{
    if (verbose)
    {
        AZ_TracePrintf("CrcFix", "Processing %s ...\n", dirs.c_str());
    }

    // do files
    FixFiles(dirs, files, pLastRun, verbose, nFound, nProcessed, nFixed, nFailed);

    // do folders
    WIN32_FIND_DATA wfd;
    HANDLE hFind;
    hFind = FindFirstFile((dirs + "\\*").c_str(), &wfd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
            {
                if (wfd.cFileName[0] == '.')
                {
                    continue;
                }
                FixDirectories(AZStd::string(dirs + "\\" + wfd.cFileName), files, pLastRun, verbose, nFound, nProcessed, nFixed, nFailed);
            }
        } while (FindNextFile(hFind, &wfd));
    }
    FindClose(hFind);
}

int main(int argc, char* argv[])
{
    AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();

    AZ::SystemAllocator::Descriptor desc;
    //desc.m_stackRecordLevels = 15;
    AZ::AllocatorInstance<AZ::SystemAllocator>::Create(desc);
    //if (AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords()) {
    //  AZ::AllocatorInstance<AZ::SystemAllocator>::Get().GetRecords()->SetMode(AZ::Debug::AllocationRecords::RECORD_FULL);
    //}
    {
        if (argc < 2)
        {
            AZ_TracePrintf("CrcFix", "Usage:\n  crcfix [-v(erbose)] [-log:logfile] {path[\\*][\\*.*]}\n");
            AZ_TracePrintf("CrcFix", "\n  Ex:\n    crcfix -v -log:timestamp.log src\\*\\*.cpp src\\*\\*.h ..\\scripts\\*.*\n\n");
        }

        char root[MAX_PATH];
        AZ::Utils::GetExecutableDirectory(root, MAX_PATH);

        AZStd::vector<AZStd::string> entries;

        const char* logfilename = NULL;
        FILETIME    lastRun;
        FILETIME* pLastRun = NULL;

        bool verbose = false;

        for (int iArg = 1; iArg < argc; ++iArg)
        {
            const char* pArg = argv[iArg];
            if (!pArg)
            {
                continue;
            }
            if (_strnicmp(pArg, "-log:", 5) == 0)
            {
                logfilename = pArg + 5;
                HANDLE hFile = CreateFile(logfilename, 0, 0, NULL, OPEN_EXISTING, 0, NULL);
                if (hFile != INVALID_HANDLE_VALUE)
                {
                    pLastRun = &lastRun;
                    GetFileTime(hFile, NULL, NULL, pLastRun);
                    CloseHandle(hFile);
                }
            }
            else if (_stricmp(pArg, "-v") == 0)
            {
                verbose = true;
            }
            else
            {
                entries.push_back(AZStd::string(pArg));
            }
        }

        // for each entry from the command line...
        int nFound = 0;
        int nProcessed = 0;
        int nFixed = 0;
        int nFailed = 0;
        for (AZStd::vector<AZStd::string>::const_iterator iEntry = entries.begin(); iEntry != entries.end(); ++iEntry)
        {
            AZStd::string entry = (iEntry->at(0) == '\\' || iEntry->find(":") != iEntry->npos) ? *iEntry : AZStd::string(root) + "\\" + *iEntry;
            AZStd::string::size_type split = entry.find("*\\");
            bool doSubdirs = split != entry.npos;
            if (doSubdirs)
            {
                FixDirectories(entry.substr(0, split), entry.substr(split + 1), pLastRun, verbose, nFound, nProcessed, nFixed, nFailed);
            }
            else
            {
                split = entry.rfind("\\");
                if (split == entry.npos)
                {
                    split = 0;
                }
                FixFiles(entry.substr(0, split), entry.substr(split), pLastRun, verbose, nFound, nProcessed, nFixed, nFailed);
            }
        }

        // update timestamp
        if (logfilename)
        {
            HANDLE      hFile = CreateFile(logfilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
            GetSystemTimeAsFileTime(&lastRun);
            SetFileTime(hFile, NULL, NULL, &lastRun);
            char log[1024];
            DWORD oCount;

            sprintf(log, "Batches processed: %zu\n\tFiles found: %d\n\tFiles processed: %d\n\tFiles fixed: %d\n\tFiles failed: %d\n", entries.size(), nFound, nProcessed, nFixed, nFailed);
            WriteFile(hFile, log, static_cast<DWORD>(strlen(log)), &oCount, NULL);

            AZStd::chrono::system_clock::time_point endTime = AZStd::chrono::system_clock::now();
            sprintf(log, "Total running time: %.2f secs.\n\tTotal processing time: %.2f secs.\n\tLongest processing time: %.2f secs.\n", (float)AZStd::chrono::milliseconds(endTime - startTime).count() / 1000.f, (float)g_totalFixTimeMs / 1000.f, (float)g_longestFixTimeMs / 1000.f);
            WriteFile(hFile, log, static_cast<DWORD>(strlen(log)), &oCount, NULL);

            CloseHandle(hFile);
        }
    }

    AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    return 0;
}

//-----------------------------------------------------------------------------
// CRCfix
//-----------------------------------------------------------------------------
void CRCfix::SkipToEOL(FILE* infile)
{
    int     c;
    for (c = lastchar; c != EOF && c != '\n'; c = fgetc(infile))
    {
        ;
    }
    lastchar = fgetc(infile);
    linenum++;
}
//-----------------------------------------------------------------------------
char* CRCfix::GetToken(FILE* infile, FILE* outfile)
{
    static char token[512];
    bool    commentline     = false;
    bool    commentblock    = false;
    bool    doublequote     = false;
    bool    singlequote     = false;
    int     i               = 0;
    int     c;

    if (lastchar == EOF)
    {
        return NULL;
    }

    for (c = lastchar; c != EOF; c = fgetc(infile))
    {
        if (!commentline && !commentblock && !doublequote && !singlequote)
        {
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '#' || c == '_')
            {
                token[i++] = c;
                continue;
            }
            else
            {
                if (i)
                {
                    lastchar    = c;
                    token[i]    = 0;
                    i           = 0;
                    return token;
                }
            }
        }

        if (commentline)
        {
            if (c == '\n')
            {
                commentline = false;
            }
        }
        else if (commentblock)
        {
            while (c == '*')
            {
                c = fgetc(infile);
                if (c == '/')
                {
                    commentblock = false;
                }
                fputc('*', outfile);
            }
        }

        if (!commentline && !commentblock)
        {
            if (c == '"' && !singlequote)
            {
                doublequote = !doublequote;
            }
            else if (c == '\'' && !doublequote)
            {
                singlequote = !singlequote;
            }
            else if (!singlequote && !doublequote)
            {
                if (c == '/')
                {
                    c = fgetc(infile);
                    if (c == '/')
                    {
                        commentline = true;
                    }
                    else if (c == '*')
                    {
                        commentblock = true;
                    }
                    if (c == '\'')
                    {
                        singlequote = true;
                    }
                    else if (c == '"')
                    {
                        doublequote = true;
                    }
                    fputc('/', outfile);
                }
            }
            else if (c == '\\')
            {
                fputc(c, outfile);
                c = fgetc(infile);
            }
        }
        fputc(c, outfile);
        if (c == '\n')
        {
            linenum++;
        }
    }
    lastchar = c;
    token[i] = 0;
    return i ? token : NULL;
}
//-----------------------------------------------------------------------------
void CRCfix::GetPreviousCRC(char* token, FILE* infile)
{
    int     c;
    while ((c = fgetc(infile)) != ')')
    {
        *token++ = c;
    }
    *token = 0;
}
//-----------------------------------------------------------------------------
int CRCfix::Fix(Filename srce)
{
    AZStd::chrono::system_clock::time_point startTime = AZStd::chrono::system_clock::now();

    bool        changed = false;
    Filename    dest(srce);
    dest.SetExt("xxx");

    linenum = 0;

    FILE* infile = fopen(srce.GetFullPath(), "r");
    FILE* outfile = fopen(dest.GetFullPath(), "w");

    if (!infile || !outfile)
    {
        if (infile)
        {
            fclose(infile);
            infile = nullptr;
        }

        if (outfile)
        {
            fclose(outfile);
            outfile = nullptr;
        }

        int dt = static_cast<int>(AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - startTime).count());
        g_totalFixTimeMs += dt;
        if (dt > g_longestFixTimeMs)
        {
            g_longestFixTimeMs = dt;
        }

        return -1;
    }

    lastchar    = fgetc(infile);

    while (char* token = GetToken(infile, outfile))
    {
        bool    got = false;

        if (strcmp(token, "AZ_CRC") == 0 && lastchar == '(')
        {
            size_t  i   = strlen(token);
            token[i++]  = lastchar;
            int     c   = fgetc(infile);

            if (c == '"')
            {
                size_t j = i + 1;

                do
                {
                    token[i++] = c;
                    c = fgetc(infile);
                } while (c != '"');

                token[i++] = c;
                c = fgetc(infile);

                int oldcrc = 0, newcrc;

                if (c == ',')
                {
                    GetPreviousCRC(token + i, infile);
                    sscanf(token + i, "%i", &oldcrc);
                    c = ')';
                }

                if (c == ')')
                {
                    token[i]    = 0;
                    c           = fgetc(infile);
                    got         = true;
                    newcrc      = AZ::Crc32(token + j, i - j - 1, true);
                    fprintf(outfile, "%s, 0x%08x)", token, newcrc);
                    if (newcrc != oldcrc)
                    {
                        changed = true;
                    }
                }
            }
            lastchar    = c;
            token[i]    = 0;
        }
        if (!got)
        {
            fwrite(token, 1, strlen(token), outfile);
        }
    }
    fclose(infile);
    fclose(outfile);

    if (changed)
    {
        Filename    backup(srce);
        backup.SetExt("crcfix_old");

        if (backup.Exists())
        {
            backup.SetWritable();
            [[maybe_unused]] bool deleted = backup.Delete();
            AZ_Assert(deleted, "failed to delete");
        }

        if (!srce.Copy(backup.GetFullPath()))
        {
            AZ_TracePrintf("CrcFix", "Failed to copy %s to %s\n", srce, backup);

            int dt = static_cast<int>(AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - startTime).count());
            g_totalFixTimeMs += dt;
            if (dt > g_longestFixTimeMs)
            {
                g_longestFixTimeMs = dt;
            }

            return -1;
        }

        if (!dest.Rename(srce.GetFullPath()))
        {
            AZ_TracePrintf("CrcFix", "Failed to rename %s to %s\n", dest, srce);

            int dt = static_cast<int>(AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - startTime).count());
            g_totalFixTimeMs += dt;
            if (dt > g_longestFixTimeMs)
            {
                g_longestFixTimeMs = dt;
            }

            return -1;
        }

        if (!backup.Delete())
        {
            AZ_TracePrintf("CrcFix", "Failed to delete %s\n", backup);
        }
    }
    else
    {
        dest.Delete();
    }


    int dt = static_cast<int>(AZStd::chrono::milliseconds(AZStd::chrono::system_clock::now() - startTime).count());
    g_totalFixTimeMs += dt;
    if (dt > g_longestFixTimeMs)
    {
        g_longestFixTimeMs = dt;
    }

    return changed ? 1 : 0;
}
//-----------------------------------------------------------------------------
