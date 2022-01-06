/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <cstdarg>
#include <stdio.h>
#include <AzCore/IO/FileIO.h>
#include "FileOperations.h"
#include <AzFramework/StringFunc/StringFunc.h>

namespace AZ
{
    namespace IO
    {
        int64_t Print(HandleType fileHandle, const char* format, ...)
        {
            va_list arglist;
            va_start(arglist, format);
            int64_t result = PrintV(fileHandle, format, arglist);
            va_end(arglist);
            return result;
        }

        int64_t PrintV(HandleType fileHandle, const char* format, va_list arglist)
        {
            const int bufferSize = 1024;
            char buffer[bufferSize];

            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::PrintV: No FileIO instance");

            int printCount = azvsnprintf(buffer, bufferSize, format, arglist);

            if (printCount > 0)
            {
                AZ_Assert(printCount < bufferSize, "AZ::IO::PrintV: required buffer size for vsnprintf is larger than working buffer");
                if (!fileIO->Write(fileHandle, buffer, printCount))
                {
                    return -1;
                }
                return printCount;
            }

            return printCount;
        }

        Result Move(const char* sourceFilePath, const char* destinationFilePath)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::Move: No FileIO instance");

            if (!fileIO->Copy(sourceFilePath, destinationFilePath))
            {
                return ResultCode::Error;
            }

            if (!fileIO->Remove(sourceFilePath))
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }



        AZ::IO::Result SmartMove(const char* sourceFilePath, const char* destinationFilePath)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::SmartMove: No FileIO instance");
            if (!fileIO->Exists(sourceFilePath))
            {
                AZ_Warning("AZ::IO::SmartMove", false, "Source file does not exist (%s)", sourceFilePath);
                return ResultCode::Error;
            }
            AZStd::string tmpDestFile;
            bool destFileMoved = false;
            if (fileIO->Exists(destinationFilePath))
            {
                if (CreateTempFileName(destinationFilePath, tmpDestFile))
                {
                    if (!fileIO->Rename(destinationFilePath, tmpDestFile.c_str()))
                    {
                        //if the rename fails try deleting the destination file
                        if (!fileIO->Remove(destinationFilePath))
                        {
                            AZ_Warning("AZ::IO::SmartMove", false, "Unable to move/delete the destination file (%s)", destinationFilePath);
                            return ResultCode::Error;
                        }
                    }
                    else
                    {
                        destFileMoved = true;
                    }
                }
                else
                {
                    if (!fileIO->Remove(destinationFilePath))
                    {
                        AZ_Warning("AZ::IO::SmartMove", false, "Unable to remove the destination file (%s)", destinationFilePath);
                        return ResultCode::Error;
                    }
                }

                //Now try renaming the source file with the destination file
                if (!fileIO->Rename(sourceFilePath, destinationFilePath))
                {
                    // if the move fails, try copying instead
                    if (!fileIO->Copy(sourceFilePath, destinationFilePath))
                    {
                        AZ_Warning("AZ::IO::SmartMove", false, "Unable to move/copy the source file (%s)", sourceFilePath);
                        if (destFileMoved)
                        {
                            // if we were unable to move/copy the source file to the dest file,
                            // we will try to revert back the destination file from the temp file.
                            if (!fileIO->Rename(tmpDestFile.c_str(), destinationFilePath))
                            {
                                AZ_Warning("AZ::IO::SmartMove", false, "Unable to rename back the destination file (%s)", destinationFilePath);
                            }
                        }

                        return ResultCode::Error;
                    }
                    // removing the source file if copy succeeds
                    if (!fileIO->Remove(sourceFilePath))
                    {
                        AZ_Warning("AZ::IO::SmartMove", false, "Unable to delete the source file (%s)", sourceFilePath);
                    }
                }

                if (destFileMoved)
                {
                    // Remove the temp file the destination file was moved to
                    if (!fileIO->Remove(tmpDestFile.c_str()))
                    {
                        AZ_Warning("AZ::IO::SmartMove", false, "Unable to move/delete the destination file (%s)", destinationFilePath);
                        return ResultCode::Error;
                    }
                }

                return ResultCode::Success;

            }
            else
            {
                return Move(sourceFilePath, destinationFilePath);
            }

        }

        bool CreateTempFileName(const char* file, AZStd::string& tempFile)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::CreateTempFileName: No FileIO instance");
            const int s_MaxCreateTempFileTries = 16;
            AZStd::string fullPath, fileName;
            tempFile.clear();

            if (!AzFramework::StringFunc::Path::GetFullPath(file, fullPath))
            {
                AZ_Warning("AZ::IO::CreateTempFileName", false, " Filepath needs to be an absolute path: '%s'", file);
                return false;
            }

            if (!AzFramework::StringFunc::Path::GetFullFileName(file, fileName))
            {
                AZ_Warning("AZ::IO::CreateTempFileName", false, " Filepath needs to be an absolute path: '%s'", file);
                return false;
            }

            for (int idx = 0; idx < s_MaxCreateTempFileTries; idx++)
            {
                AzFramework::StringFunc::Path::ConstructFull(fullPath.c_str(), AZStd::string::format("$tmp%d_%s", rand(), fileName.c_str()).c_str(), tempFile, true);
                //Ensure that the file does not exist
                if (!fileIO->Exists(tempFile.c_str()))
                {
                    return true;
                }
            }
            AZ_Warning("AZ::IO::CreateTempFileName", false, "Unable to create temp file for (%s). Please check the folder and clear any temp files.", file);
            tempFile.clear();
            return false;
        }

        int GetC(HandleType fileHandle)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::GetC: No FileIO instance");
            char character;
            if (!fileIO->Read(fileHandle, &character, 1, true))
            {
                return EOF;
            }
            return static_cast<int>(character);
        }

        int UnGetC(int character, HandleType fileHandle)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::UnGetC: No FileIO instance");

            // EOF does nothing
            if (character == EOF)
            {
                return EOF;
            }
            char characterToRestore = static_cast<char>(character);
            // Just move back a byte and write it in
            // Still not identical behavior since ungetc normally gets lost if you seek, but it works for our case
            if (!fileIO->Seek(fileHandle, -1, SeekType::SeekFromCurrent))
            {
                return EOF;
            }

            if (!fileIO->Write(fileHandle, &characterToRestore, sizeof(characterToRestore)))
            {
                return EOF;
            }
            return character;
        }

        char* FGetS(char* buffer, uint64_t bufferSize, HandleType fileHandle)
        {
            AZ_Assert(buffer && bufferSize, "AZ::IO::FGetS: Invalid buffer supplied");

            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FGetS: No FileIO instance");

            memset(buffer, 0, bufferSize);

            AZ::u64 bytesRead = 0;
            fileIO->Read(fileHandle, buffer, bufferSize - 1, false, &bytesRead);
            if (!bytesRead)
            {
                return nullptr;
            }

            char* currentPosition = buffer;
            int64_t len = 0;

            bool done = false;
            int64_t slashRPosition = -1;
            do
            {
                if (*currentPosition != '\r' && *currentPosition != '\n' && static_cast<uint64_t>(len) < bytesRead - 1)
                {
                    len++;
                    currentPosition++;
                }
                else
                {
                    done = true;
                    if (*currentPosition == '\r')
                    {
                        slashRPosition = len;
                    }
                }
            } while (!done);

            // null terminate string
            buffer[len] = '\0';

            //////////////////////////////////////////
            //seek back to the end of the string
            AZ::s64 seekback = bytesRead - len - 1;

            // handle CR/LF for file coming from different platforms
            if (slashRPosition > -1 && bytesRead > static_cast<uint64_t>(slashRPosition) && buffer[slashRPosition + 1] == '\n')
            {
                seekback--;
            }
            fileIO->Seek(fileHandle, -seekback, AZ::IO::SeekType::SeekFromCurrent);
            ///////////////////////////////////////////

            return buffer;
        }

        int FPutS(const char* buffer, HandleType fileHandle)
        {
            FileIOBase* fileIO = AZ::IO::FileIOBase::GetInstance();
            AZ_Assert(fileIO, "AZ::IO::FPutS: No FileIO instance");

            uint64_t writeAmount = strlen(buffer);
            uint64_t bytesWritten = 0;
            if (!fileIO->Write(fileHandle, buffer, writeAmount))
            {
                return EOF;
            }
            return static_cast<int>(bytesWritten);
        }
    } // namespace IO
} // namespace AZ
