/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzFramework/IO/LocalFileIO.h>
#include <sys/stat.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/IOUtils.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Casting/lossy_cast.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <cctype>

namespace AZ
{
    namespace IO
    {
        const HandleType LocalHandleStartValue = 1000000; //start the local file io handles at 1 million

        LocalFileIO::LocalFileIO()
        {
            m_nextHandle = LocalHandleStartValue;
        }

        LocalFileIO::~LocalFileIO()
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            while (!m_openFiles.empty())
            {
                const auto& handlePair = *m_openFiles.begin();
                Close(handlePair.first);
            }

            AZ_Assert(m_openFiles.empty(), "Trying to shutdown filing system with files still open");
        }

        Result LocalFileIO::Open(const char* filePath, OpenMode mode, HandleType& fileHandle)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            AZ::IO::UpdateOpenModeForReading(mode);

            // Generate open modes for SystemFile
            int systemFileMode = TranslateOpenModeToSystemFileMode(resolvedPath, mode);
            bool write = AnyFlag(mode & (OpenMode::ModeWrite | OpenMode::ModeUpdate | OpenMode::ModeAppend));
            if (write)
            {
                CheckInvalidWrite(resolvedPath);
            }

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);

                fileHandle = GetNextHandle();

                // Construct a new SystemFile in the map (SystemFiles don't copy/move very well).
                auto newPair = m_openFiles.emplace(fileHandle);
                // Check for successful insert
                if (!newPair.second)
                {
                    fileHandle = InvalidHandle;
                    return ResultCode::Error;
                }
                // Attempt to open the newly created file
                if (newPair.first->second.Open(resolvedPath, systemFileMode, 0))
                {
                    return ResultCode::Success;
                }
                else
                {
                    // Remove file, it's not actually open
                    m_openFiles.erase(fileHandle);
                    // On failure, ensure the fileHandle returned is invalid
                    //  some code does not check return but handle value (equivalent to checking for nullptr FILE*)
                    fileHandle = InvalidHandle;
                    return ResultCode::Error;
                }
            }
        }

        Result LocalFileIO::Close(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            filePointer->Close();

            {
                AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
                m_openFiles.erase(fileHandle);
            }
            return ResultCode::Success;
        }

        Result LocalFileIO::Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead, AZ::u64* bytesRead)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SizeType readResult = filePointer->Read(size, buffer);

            if (bytesRead)
            {
                *bytesRead = aznumeric_cast<AZ::u64>(readResult);
            }

            if (static_cast<AZ::u64>(readResult) != size)
            {
                if (failOnFewerThanSizeBytesRead)
                {
                    return ResultCode::Error;
                }

                // Reading less than desired is valid if ferror is not set
                AZ_Assert(Eof(fileHandle), "End of file unexpectedly reached before all data was read");
            }

            return ResultCode::Success;
        }

        Result LocalFileIO::Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SizeType writeResult = filePointer->Write(buffer, size);

            if (bytesWritten)
            {
                *bytesWritten = writeResult;
            }

            if (static_cast<AZ::u64>(writeResult) != size)
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

        Result LocalFileIO::Flush(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            filePointer->Flush();

            return ResultCode::Success;
        }

        Result LocalFileIO::Tell(HandleType fileHandle, AZ::u64& offset)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SizeType resultValue = filePointer->Tell();
            if (resultValue == -1)
            {
                return ResultCode::Error;
            }

            offset = static_cast<AZ::u64>(resultValue);
            return ResultCode::Success;
        }

        Result LocalFileIO::Seek(HandleType fileHandle, AZ::s64 offset, SeekType type)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            SystemFile::SeekMode mode = SystemFile::SF_SEEK_BEGIN;
            if (type == SeekType::SeekFromCurrent)
            {
                mode = SystemFile::SF_SEEK_CURRENT;
            }
            else if (type == SeekType::SeekFromEnd)
            {
                mode = SystemFile::SF_SEEK_END;
            }

            filePointer->Seek(offset, mode);

            return ResultCode::Success;
        }

        Result LocalFileIO::Size(HandleType fileHandle, AZ::u64& size)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return ResultCode::Error_HandleInvalid;
            }

            size = filePointer->Length();
            return ResultCode::Success;
        }

        Result LocalFileIO::Size(const char* filePath, AZ::u64& size)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            size = SystemFile::Length(resolvedPath);
            if (!size)
            {
                return SystemFile::Exists(resolvedPath) ? ResultCode::Success : ResultCode::Error;
            }

            return ResultCode::Success;
        }

        bool LocalFileIO::IsReadOnly(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            return !SystemFile::IsWritable(resolvedPath);
        }

        bool LocalFileIO::Eof(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return false;
            }

            return filePointer->Eof();
        }

        AZ::u64 LocalFileIO::ModificationTime(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            return SystemFile::ModificationTime(resolvedPath);
        }

        AZ::u64 LocalFileIO::ModificationTime(HandleType fileHandle)
        {
            auto filePointer = GetFilePointerFromHandle(fileHandle);
            if (!filePointer)
            {
                return 0;
            }

            return filePointer->ModificationTime();
        }

        bool LocalFileIO::Exists(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            return SystemFile::Exists(resolvedPath);
        }

        void LocalFileIO::CheckInvalidWrite(const char* path)
        {
            (void)path;

#if defined(AZ_ENABLE_TRACING)
            const char* assetsAlias = GetAlias("@assets@");

            if (((path) && (assetsAlias) && (azstrnicmp(path, assetsAlias, strlen(assetsAlias)) == 0)))
            {
                AZ_Error("FileIO", false, "You may not alter data inside the asset cache.  Please check the call stack and consider writing into the source asset folder instead.\n"
                    "Attempted write location: %s", path);
            }
#endif
        }

        Result LocalFileIO::Remove(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];

            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            CheckInvalidWrite(resolvedPath);

            if (IsDirectory(resolvedPath))
            {
                return ResultCode::Error;
            }

            return SystemFile::Delete(resolvedPath) ? ResultCode::Success : ResultCode::Error;
        }

        Result LocalFileIO::Rename(const char* originalFilePath, const char* newFilePath)
        {
            char resolvedOldPath[AZ_MAX_PATH_LEN];
            char resolvedNewPath[AZ_MAX_PATH_LEN];

            ResolvePath(originalFilePath, resolvedOldPath, AZ_MAX_PATH_LEN);
            ResolvePath(newFilePath, resolvedNewPath, AZ_MAX_PATH_LEN);

            CheckInvalidWrite(resolvedNewPath);

            if (!SystemFile::Rename(resolvedOldPath, resolvedNewPath))
            {
                return ResultCode::Error;
            }

            return ResultCode::Success;
        }

        SystemFile* LocalFileIO::GetFilePointerFromHandle(HandleType fileHandle)
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            auto openFileIterator = m_openFiles.find(fileHandle);
            if (openFileIterator != m_openFiles.end())
            {
                SystemFile& file = openFileIterator->second;
                return &file;
            }
            return nullptr;
        }

        HandleType LocalFileIO::GetNextHandle()
        {
            return m_nextHandle++;
        }

        static ResultCode DestroyPath_Recurse(LocalFileIO* fileIO, const char* filePath)
        {
            // this is a deltree command.  It needs to eat everything.  Even files.
            ResultCode res = ResultCode::Success;

            fileIO->FindFiles(filePath, "*", [&](const char* iterPath) -> bool
                {
                    // depth first recurse into directories!

                    // note:  findFiles returns full path names.

                    if (fileIO->IsDirectory(iterPath))
                    {
                        // recurse.
                        if (DestroyPath_Recurse(fileIO, iterPath) != ResultCode::Success)
                        {
                            res = ResultCode::Error;
                            return false; // stop the find files.
                        }
                    }
                    else
                    {
                        // if its a file, remove it
                        if (fileIO->Remove(iterPath) != ResultCode::Success)
                        {
                            res = ResultCode::Error;
                            return false; // stop the find files.
                        }
                    }
                    return true; // continue the find files
                });

            if (res != ResultCode::Success)
            {
                return res;
            }

            // now that we've finished recursing, rmdir on the folder itself
            return AZ::IO::SystemFile::DeleteDir(filePath) ? ResultCode::Success : ResultCode::Error;
        }

        Result LocalFileIO::DestroyPath(const char* filePath)
        {
            char resolvedPath[AZ_MAX_PATH_LEN];
            ResolvePath(filePath, resolvedPath, AZ_MAX_PATH_LEN);

            bool pathExists = Exists(resolvedPath);
            if (!pathExists)
            {
                return ResultCode::Success;
            }

            if (pathExists && (!IsDirectory(resolvedPath)))
            {
                return ResultCode::Error;
            }

            return DestroyPath_Recurse(this, resolvedPath);
        }

        static void ToUnixSlashes(char* path, AZ::u64 size)
        {
            auto PrevAndCurrentCharIsPathSeparator = [](const char prev, const char next) -> bool
            {
                constexpr AZStd::string_view pathSeparator = "/";
                const bool prevIsPathSeparator = pathSeparator.find_first_of(prev) != AZStd::string_view::npos;
                const bool nextIsPathSeparator = pathSeparator.find_first_of(next) != AZStd::string_view::npos;
                return prevIsPathSeparator && nextIsPathSeparator;
            };

            size_t copyOffset = 0;
            for (size_t i = 0; i < size && path[i] != '\0'; i++)
            {
                if (path[i] == '\\')
                {
                    path[i] = '/';
                }

                // Replace runs of path separators with one path separator
#if AZ_TRAIT_USE_WINDOWS_FILE_API
                // Network file systems for Windows based APIs start with consecutive path separators
                // so skip over the first character in this case
                constexpr size_t duplicateSeparatorStartOffet = 1;
#else
                constexpr size_t duplicateSeparatorStartOffet = 0;
#endif
                if (i > duplicateSeparatorStartOffet)
                {
                    if (PrevAndCurrentCharIsPathSeparator(path[i - 1], path[i]))
                    {
                        continue;
                    }
                }

                // If the copy offset is different than the iteration index of the path, then copy over it over
                if (copyOffset != i)
                {
                    path[copyOffset] = path[i];
                }

                ++copyOffset;
            }

            // Null-terminate the path again in case duplicate slashes were collapsed
            path[copyOffset] = '\0';

        }

        bool LocalFileIO::ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const
        {
            if (resolvedPath == nullptr || resolvedPathSize == 0)
            {
                return false;
            }

            resolvedPath[0] = '\0';

            if (path == nullptr)
            {
                return false;
            }

            if (IsAbsolutePath(path))
            {
                size_t pathLen = strlen(path);
                if (pathLen + 1 < resolvedPathSize)
                {
                    azstrncpy(resolvedPath, resolvedPathSize, path, pathLen + 1);

                    //see if the absolute path uses @assets@ or @root@, if it does lowercase the relative part
                    [[maybe_unused]] bool lowercasePath = LowerIfBeginsWith(resolvedPath, resolvedPathSize, GetAlias("@assets@"))
                        || LowerIfBeginsWith(resolvedPath, resolvedPathSize, GetAlias("@root@"))
                        || LowerIfBeginsWith(resolvedPath, resolvedPathSize, GetAlias("@projectplatformcache@"));

                    ToUnixSlashes(resolvedPath, resolvedPathSize);
                    return true;
                }
                else
                {
                    return false;
                }
            }

            char rootedPathBuffer[AZ_MAX_PATH_LEN] = {0};
            const char* rootedPath = path;
            // if the path does not begin with an alias, then it is assumed to begin with @assets@
            if (path[0] != '@')
            {
                if (GetAlias("@assets@"))
                {
                    const int rootLength = 9;// strlen("@assets@/")
                    azstrncpy(rootedPathBuffer, AZ_MAX_PATH_LEN, "@assets@/", rootLength);
                    size_t pathLen = strlen(path);
                    size_t rootedPathBufferlength = rootLength + pathLen + 1;// +1 for null terminator
                    if (rootedPathBufferlength > resolvedPathSize)
                    {
                        AZ_Assert(rootedPathBufferlength < resolvedPathSize, "Constructed path length is wrong:%s", rootedPathBuffer);//path constructed is wrong
                        size_t remainingSize = resolvedPathSize - rootLength - 1;// - 1 for null terminator
                        azstrncpy(rootedPathBuffer + rootLength, AZ_MAX_PATH_LEN, path, remainingSize);
                        rootedPathBuffer[resolvedPathSize - 1] = '\0';
                    }
                    else
                    {
                        azstrncpy(rootedPathBuffer + rootLength, AZ_MAX_PATH_LEN - rootLength, path, pathLen + 1);
                    }
                }
                else
                {
                    ConvertToAbsolutePath(path, rootedPathBuffer, AZ_MAX_PATH_LEN);
                }
                rootedPath = rootedPathBuffer;
            }

            if (ResolveAliases(rootedPath, resolvedPath, resolvedPathSize))
            {
                ToUnixSlashes(resolvedPath, resolvedPathSize);
                return true;
            }

            return false;
        }

        bool LocalFileIO::ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const
        {
            if (AZ::IO::FixedMaxPathString fixedPath{ path.Native() };
                ResolvePath(fixedPath.c_str(), resolvedPath.Native().data(), resolvedPath.Native().capacity()))
            {
                // Update the null-terminator offset
                resolvedPath.Native().resize_no_construct(AZStd::char_traits<char>::length(resolvedPath.Native().data()));
                return true;
            }

            return false;
        }

        void LocalFileIO::SetAlias(const char* key, const char* path)
        {
            char fullPath[AZ_MAX_PATH_LEN];
            ConvertToAbsolutePath(path, fullPath, AZ_MAX_PATH_LEN);

            const auto it = AZStd::find_if(m_aliases.begin(), m_aliases.end(), [key](const AliasType& alias)
            {
                return alias.first.compare(key) == 0;
            });

            if (it != m_aliases.end())
            {
                it->second = fullPath;
            }
            else
            {
                m_aliases.emplace_back(key, fullPath);
            }
        }

        const char* LocalFileIO::GetAlias(const char* key) const
        {
            const auto it = AZStd::find_if(m_aliases.begin(), m_aliases.end(), [key](const AliasType& alias)
                    {
                        return alias.first.compare(key) == 0;
                    });

            if (it != m_aliases.end())
            {
                return it->second.c_str();
            }
            return nullptr;
        }

        void LocalFileIO::ClearAlias(const char* key)
        {
            m_aliases.erase(AZStd::remove_if(m_aliases.begin(), m_aliases.end(), [key](const AliasType& alias)
                    {
                        return alias.first.compare(key) == 0;
                    }), m_aliases.end());
        }

        AZStd::optional<AZ::u64> LocalFileIO::ConvertToAliasBuffer(char* outBuffer, AZ::u64 outBufferLength, AZStd::string_view inBuffer) const
        {
            size_t longestMatch = 0;
            size_t bufStringLength = inBuffer.size();
            AZStd::string_view longestAlias;

            for (const auto& [alias, resolvedAlias] : m_aliases)
            {
                // here we are making sure that the buffer being passed in has enough space to include the alias in it.
                // we are trying to find the LONGEST match, meaning of the following two examples, the second should 'win'
                // File:  g:/O3DE/dev/files/morefiles/blah.xml
                // Alias1 links to 'g:/O3DE/dev/'
                // Alias2 links to 'g:/O3DE/dev/files/morefiles'
                // so returning Alias2 is preferred as it is more specific, even though alias1 includes it.
                // note that its not possible for this to be matched if the string is shorter than the length of the alias itself so we skip
                // strings that are shorter than the alias's mapped path without checking.
                if ((longestMatch == 0) || (resolvedAlias.size() > longestMatch) && (resolvedAlias.size() <= bufStringLength))
                {
                    // custom strcmp that ignores slash directions
                    constexpr AZStd::string_view pathSeparators{ "/\\" };
                    bool allMatch = AZStd::equal(resolvedAlias.begin(), resolvedAlias.end(), inBuffer.begin(),
                        [&pathSeparators](const char lhs, const char rhs)
                    {
                        const bool lhsIsSeparator = pathSeparators.find_first_of(lhs) != AZStd::string_view::npos;
                        const bool rhsIsSeparator = pathSeparators.find_first_of(lhs) != AZStd::string_view::npos;
                        return (lhsIsSeparator && rhsIsSeparator) || tolower(lhs) == tolower(rhs);
                    });

                    if (allMatch)
                    {
                        // Either the resolvedAlias path must match the path exactly or the path must have a path separator character
                        // right after the resolved alias
                        if (const size_t matchLen = resolvedAlias.size();
                            matchLen == bufStringLength || (pathSeparators.find_first_of(inBuffer[matchLen]) != AZStd::string_view::npos))
                        {
                            longestMatch = matchLen;
                            longestAlias = alias;
                        }
                    }
                }
            }
            if (!longestAlias.empty())
            {
                // rearrange the buffer to have
                // [alias][old path]
                size_t aliasSize = longestAlias.size();
                size_t charsToAbsorb = longestMatch;
                size_t remainingData = bufStringLength - charsToAbsorb;
                size_t finalStringSize = aliasSize + remainingData;
                if (finalStringSize >= outBufferLength)
                {
                    AZ_Error("FileIO", false, "Alias %.*s cannot fit in output buffer. The output buffer is too small (max len %lu, actual len %zu)",
                        aznumeric_cast<int>(longestAlias.size()), longestAlias.data(), outBufferLength, finalStringSize);
                    // avoid buffer overflow, return original untouched
                    return AZStd::nullopt;
                }

                // move the remaining piece of the string:
                memmove(outBuffer + aliasSize, inBuffer.data() + charsToAbsorb, remainingData);
                // copy the alias
                memcpy(outBuffer, longestAlias.data(), aliasSize);
                /// add a null
                outBuffer[finalStringSize] = 0;
                return finalStringSize;
            }

            // If the input and output buffer are different, copy over the input buffer to the output buffer
            if (outBuffer != inBuffer.data())
            {
                if (inBuffer.size() >= outBufferLength)
                {
                    AZ_Error("FileIO", false, R"(Path "%.*s" cannot fit in output buffer. The output buffer is too small (max len %llu, actual len %zu))",
                        aznumeric_cast<int>(inBuffer.size()), inBuffer.data(), outBufferLength, inBuffer.size());
                    return AZStd::nullopt;
                }
                size_t finalStringSize = inBuffer.copy(outBuffer, outBufferLength);
                outBuffer[finalStringSize] = 0;
            }
            return bufStringLength;
        }

        AZStd::optional<AZ::u64> LocalFileIO::ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const
        {
            return ConvertToAliasBuffer(inOutBuffer, bufferLength, inOutBuffer);
        }

        bool LocalFileIO::ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& pathView) const
        {
            AZStd::optional<AZ::u64> convertedPathSize =
                ConvertToAliasBuffer(convertedPath.Native().data(), convertedPath.Native().capacity(), pathView.Native());
            if (convertedPathSize)
            {
                // Force update of AZStd::fixed_string m_size member to set correct null-terminator offset
                convertedPath.Native().resize_no_construct(*convertedPathSize);
                return true;
            }

            return false;
        }

        bool LocalFileIO::ResolveAliases(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const
        {
            AZ_Assert(path != resolvedPath && resolvedPathSize > strlen(path), "Resolved path is incorrect");
            AZ_Assert(path && path[0] != '%', "%% is deprecated, @ is the only valid alias token");

            // we assert above, but we also need to properly handle the case when the resolvedPath buffer size
            // is too small to copy the source into.
            size_t pathLen = strlen(path) + 1; // account for null
            if (path == resolvedPath || (resolvedPathSize < pathLen))
            {
                return false;
            }

            azstrncpy(resolvedPath, resolvedPathSize, path, pathLen);
            for (const auto& alias : m_aliases)
            {
                const char* key = alias.first.c_str();
                size_t keyLen = alias.first.length();
                if (azstrnicmp(resolvedPath, key, keyLen) == 0) // we only support aliases at the front of the path
                {
                    [[maybe_unused]] bool lowercasePath = LowerIfBeginsWith(resolvedPath, resolvedPathSize, "@assets@")
                        || LowerIfBeginsWith(resolvedPath, resolvedPathSize, "@root@")
                        || LowerIfBeginsWith(resolvedPath, resolvedPathSize, "@projectplatformcache@");

                    const char* dest = alias.second.c_str();
                    size_t destLen = alias.second.length();
                    char* afterKey = resolvedPath + keyLen;
                    size_t afterKeyLen = pathLen - keyLen;
                    // must ensure that we are replacing the entire folder name, not a partial (e.g. @GAME01@/ vs @GAME0@/)
                    if (*afterKey == '/' || *afterKey == '\\' || *afterKey == 0)
                    {
                        if (afterKeyLen + destLen + 1 < resolvedPathSize)//if after replacing the alias the length is greater than the max path size than skip
                        {
                            // scoot the right hand side of the replacement over to make room
                            memmove(resolvedPath + destLen, afterKey, afterKeyLen + 1); // make sure null is copied
                            memcpy(resolvedPath, dest, destLen); // insert replacement
                            pathLen -= keyLen;
                            pathLen += destLen;

                            AZStd::replace(resolvedPath, resolvedPath + resolvedPathSize, '\\', '/');
                            return true;
                        }
                    }
                }
            }

            // warn on failing to resolve an alias
            AZ_Warning(
                "LocalFileIO::ResolveAlias", path && path[0] != '@',
                "Failed to resolve an alias: %s", path ? path : "(null)");

            return false;
        }

        bool LocalFileIO::ReplaceAlias(AZ::IO::FixedMaxPath& replacedAliasPath, const AZ::IO::PathView& path) const
        {
            if (path.empty())
            {
                replacedAliasPath = path;
                return true;
            }

            AZStd::string_view pathStrView = path.Native();
            for (const auto& [aliasKey, aliasValue] : m_aliases)
            {
                if (AZ::StringFunc::StartsWith(pathStrView, aliasKey))
                {
                    // Reduce of the size result result path by the size of the and add the resolved alias size
                    AZStd::string_view postAliasView = pathStrView.substr(aliasKey.size());
                    size_t requiredFixedMaxPathSize = postAliasView.size();
                    requiredFixedMaxPathSize += aliasValue.size();

                    // The replaced alias path is greater than 1024 characters, return false
                    if (requiredFixedMaxPathSize > replacedAliasPath.Native().max_size())
                    {
                        return false;
                    }
                    replacedAliasPath.Native() = AZ::IO::FixedMaxPathString(AZStd::string_view{ aliasValue });
                    replacedAliasPath.Native() += postAliasView;
                    return true;
                }
            }

            if (pathStrView.size() > replacedAliasPath.Native().max_size())
            {
                return false;
            }

            replacedAliasPath = path;
            return true;
        }

        bool LocalFileIO::GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const
        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(m_openFileGuard);
            auto fileIt = m_openFiles.find(fileHandle);
            if (fileIt != m_openFiles.end())
            {
                azstrncpy(filename, filenameSize, fileIt->second.Name(), filenameSize);
                return true;
            }
            return false;
        }

        bool LocalFileIO::LowerIfBeginsWith(char* inOutBuffer, AZ::u64 bufferLen, const char* alias) const
        {
            if (alias)
            {
                AZ::u64 aliasLen = azlossy_caster(strlen(alias));
                if (azstrnicmp(inOutBuffer, alias, aliasLen) == 0)
                {
                    for (AZ::u64 i = aliasLen; i < bufferLen && inOutBuffer[i] != '\0'; ++i)
                    {
                        inOutBuffer[i] = static_cast<char>(std::tolower(static_cast<int>(inOutBuffer[i])));
                    }

                    return true;
                }
            }

            return false;
        }

        AZ::OSString LocalFileIO::RemoveTrailingSlash(const AZ::OSString& pathStr)
        {
            if (pathStr.empty() || (pathStr[pathStr.length() - 1] != '/' && pathStr[pathStr.length() - 1] != '\\'))
            {
                return pathStr;
            }

            return pathStr.substr(0, pathStr.length() - 1);
        }

        AZ::OSString LocalFileIO::CheckForTrailingSlash(const AZ::OSString& pathStr)
        {
            if (pathStr.empty() || pathStr[pathStr.length() - 1] == '/')
            {
                return pathStr;
            }

            if (pathStr[pathStr.length() - 1] == '\\')
            {
                return pathStr.substr(0, pathStr.length() - 1) + "/";
            }

            return pathStr + "/";
        }
    } // namespace IO
} // namespace AZ
