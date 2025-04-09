/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/string/fixed_string.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>

// this header contains the interface FileIOBase, a base class to derive
// from in order to implement the low level file operations that the engine uses
// all file access by the engine should through a FileIOBase interface-derived class
// which is provided in the global environment at "EngineFileIO"
// To access it use the GetInstance/SetInstance functions in FileIOBase.

namespace AZ
{
    namespace IO
    {
        // Utility functions

        /// return true if name of file matches glob filter.
        /// glob filters are MS-DOS (or windows) findNextFile style filters
        /// like "*.bat" or "blah??.pak" or "test*.exe" and such.
        bool NameMatchesFilter(AZStd::string_view name, AZStd::string_view filter);

        //! Converts the operating-specific values returned by AZ::IO::FileIO API
        //! to independent units representing the milliseconds since 1/1/1970 0:00 UTC
        AZ::u64 FileTimeToMSecsSincePosixEpoch(AZ::u64 fileTime);

        using HandleType = AZ::u32;
        static const HandleType InvalidHandle = 0;

        enum class SeekType : AZ::u32
        {
            SeekFromStart,
            SeekFromCurrent,
            SeekFromEnd
        };

        SeekType GetSeekTypeFromFSeekMode(int mode);
        int GetFSeekModeFromSeekType(SeekType type);

        enum class ResultCode : AZ::u32
        {
            Success = 0,
            Error = 1,
            Error_HandleInvalid = 2
                // More results (errors) here
        };

        // a function which returns a result code and supports operator bool explicitly
        class Result
        {
        public:
            Result(ResultCode resultCode)
                : m_resultCode(resultCode)
            {
            }

            ResultCode GetResultCode() const
            {
                return *this;
            }

            operator bool() const
            {
                return m_resultCode == ResultCode::Success;
            }

            operator ResultCode() const
            {
                return m_resultCode;
            }
        private:
            ResultCode m_resultCode;
        };

        /// The base class for file IO stack classes
        class FileIOBase
        {
        public:
            virtual ~FileIOBase()
            {
            }

            /// GetInstance()
            /// A utility function to get the one global instance of File IO to use when talking to engine.
            /// returns an instance which is aware of concepts such as pak and loose files and can be used to interact with them.
            /// Note that the PAK system itself should not call GetInstance but instead should use the direct underlying IO system
            /// using GetDirectInstance instead since otherwise an infinite loop will be caused.
            static FileIOBase* GetInstance();
            static void SetInstance(FileIOBase* instance);

            /// Direct IO is the underlying IO system and does not route through pack files.  This should only be used by the
            /// pack system itself, or systems with similar needs that should be "blind" to packages and only see the real file system.
            static FileIOBase* GetDirectInstance();
            static void SetDirectInstance(FileIOBase* instance);

            // Standard operations
            virtual Result Open(const char* filePath, OpenMode mode, HandleType& fileHandle) = 0;
            virtual Result Close(HandleType fileHandle) = 0;
            virtual Result Tell(HandleType fileHandle, AZ::u64& offset) = 0;
            virtual Result Seek(HandleType fileHandle, AZ::s64 offset, SeekType type) = 0;
            virtual Result Read(HandleType fileHandle, void* buffer, AZ::u64 size, bool failOnFewerThanSizeBytesRead = false, AZ::u64* bytesRead = nullptr) = 0;
            virtual Result Write(HandleType fileHandle, const void* buffer, AZ::u64 size, AZ::u64* bytesWritten = nullptr) = 0;
            virtual Result Flush(HandleType fileHandle) = 0;
            virtual bool Eof(HandleType fileHandle) = 0;

            /// the only requirement on the ModTime functions is that it must be comparable with subsequent calls to the same function.
            /// there is no specific requirement that they be cross-platform or adhere to any standard, but they should still be comparable
            /// across sessions.
            virtual AZ::u64 ModificationTime(HandleType fileHandle) = 0;
            virtual AZ::u64 ModificationTime(const char* filePath) = 0;

            /// Get the size of the file.  Returns Success if we report size.
            virtual Result Size(const char* filePath, AZ::u64& size) = 0;
            virtual Result Size(HandleType fileHandle, AZ::u64& size) = 0;

            /// no fail, returns false if it does not exist
            virtual bool Exists(const char* filePath) = 0;

            /// no fail, returns false if its not a directory or does not exist
            virtual bool IsDirectory(const char* filePath) = 0;

            /// no fail, returns false if its read only or does not exist
            virtual bool IsReadOnly(const char* filePath) = 0;

            /// create a path, recursively
            virtual Result CreatePath(const char* filePath) = 0;

            /// DestroyPath - Destroys the entire path and all of its contents (all files, all subdirectories, etc)
            /// succeeds if the directory was successfully destroys
            /// succeeds if the directory didn't exist in the first place
            /// fails if the path could not be destroyed (for example, if a file could not be erased for any reason)
            /// also fails if filePath refers to a file name which exists instead of a directory name.  It must be a directory name.
            virtual Result DestroyPath(const char* filePath) = 0;

            /// Remove - erases a single file
            /// succeeds if the file didn't exist to begin with
            /// succeeds if the file did exist and we successfully erased it.
            /// fails if the file cannot be erased
            /// also fails if the specified filePath is actually a directory that exists (instead of a file)
            virtual Result Remove(const char* filePath) = 0;

            /// does not overwrite the destination if it exists.
            /// note that attributes are not required to be copied
            /// the modtime of the destination path must be equal or greater than the source path
            /// but doesn't have to be equal (some operating systems forbid modifying modtimes due to security constraints)
            virtual Result Copy(const char* sourceFilePath, const char* destinationFilePath) = 0;

            /// renames a file or directory name
            /// also fails if the file or directory cannot be found
            /// also succeeds if originalFilePath == newFilePath
            virtual Result Rename(const char* originalFilePath, const char* newFilePath) = 0;

            /// FindFiles -
            /// return all files and directories matching that filter (dos-style filters) at that file path
            /// does not recurse.
            /// does not return the . or .. directories
            /// not all filters are supported.
            /// You must include <AzCore/std/functional.h> if you use this
            /// note: the callback will contain the full concatenated path (filePath + slash + fileName)
            ///       not just the individual file name found.
            /// note: if the file path of the found file corresponds to a registered ALIAS, the longest matching alias will be returned
            ///       so expect return values like @products@/textures/mytexture.dds instead of a full path.  This is so that fileIO works over remote connections.
            /// note: if rootPath is specified the implementation has the option of substituting it for the current directory
            ///      as would be the case on a file server.
            typedef AZStd::function<bool(const char*)> FindFilesCallbackType;
            virtual Result FindFiles(const char* filePath, const char* filter, FindFilesCallbackType callback) = 0;

            // Alias system

            /// SetAlias - Adds an alias to the path resolution system, e.g. @user@, @products@, etc.
            virtual void SetAlias(const char* alias, const char* path) = 0;
            /// ClearAlias - Removes an alias from the path resolution system
            virtual void ClearAlias(const char* alias) = 0;
            /// GetAlias - Returns the destination path for a given alias, or nullptr if the alias does not exist
            virtual const char* GetAlias(const char* alias) const = 0;

            /// SetDeprecateAlias - Adds a deprecated alias with path resolution which points to a new alias
            /// When the DeprecatedAlias is used an Error is logged and the alias is resolved to the path
            /// specified by the new alais
            virtual void SetDeprecatedAlias(AZStd::string_view oldAlias, AZStd::string_view newAlias) = 0;

            /// Shorten the given path if it contains an alias.  it will always pick the longest alias match.
            /// note that it re-uses the buffer, since the data can only get smaller and we don't want to internally allocate memory if we
            /// can avoid it.
            /// If for some reason the @alias@ is longer than the path it represents and the path cannot fit in the buffer
            /// a nullopt is returned, otherwise it returns the length of the result
            virtual AZStd::optional<AZ::u64> ConvertToAlias(char* inOutBuffer, AZ::u64 bufferLength) const = 0;

            //! ConvertToAlias - Converts the start of the path to alias form. i.e @alias@/... if possible
            //! i.e if @test@ = C:\\foo\\bar and path = C:\\foo\\bar\baz
            //! the converted alias will be @test@\baz.
            //! if no alias matches the start of the path, the path is returned as a AZ::IO::FixedPath
            //! If the path cannot fit in AZ::IO::FixedMaxPath false is returned
            virtual bool ConvertToAlias(AZ::IO::FixedMaxPath& convertedPath, const AZ::IO::PathView& path) const = 0;
            AZStd::optional<AZ::IO::FixedMaxPath> ConvertToAlias(const AZ::IO::PathView& path) const;

            //! ResolvePath - Replaces any aliases in path with their values and stores the result in resolvedPath,
            //! also ensures that the path is absolute
            //! NOTE: If the path does not start with an alias then the resolved value of the @products@ is used
            //!       which has the effect of making the path relative to the @products@/ folder
            //! returns true if path was resolved, false otherwise
            //! note that all of the above file-finding and opening functions automatically resolve the path before operating
            //! so you should not need to call this except in very exceptional circumstances where you absolutely need to
            //! hit a physical file and don't want to use SystemFile
            virtual bool ResolvePath(const char* path, char* resolvedPath, AZ::u64 resolvedPathSize) const = 0;

            //! ResolvePath - Replaces any @ aliases in the supplied path with their the resolved alias values
            //! Converts the path to an absolute path, and replaces path separators with unix slashes
            //! The resolved path is returned in as a FixedMaxPath, unless it is longer than
            //! MaxPathLength in which case a null optional is returned
            virtual bool ResolvePath(AZ::IO::FixedMaxPath& resolvedPath, const AZ::IO::PathView& path) const = 0;
            AZStd::optional<AZ::IO::FixedMaxPath> ResolvePath(const AZ::IO::PathView& path) const;

            //! ReplaceAliases - If the path starts with an @...@ alias it is substituted with the alias value
            //! otherwise the path is copied as is to the resolvedAlias path value
            //! returns true if the resulting path can fit within AZ::IO::FixedMaxPath buffer
            virtual bool ReplaceAlias(AZ::IO::FixedMaxPath& replacedAliasPath, const AZ::IO::PathView& path) const = 0;

            /// Divulge the filename used to originally open that handle.
            virtual bool GetFilename(HandleType fileHandle, char* filename, AZ::u64 filenameSize) const = 0;

            virtual bool IsRemoteIOEnabled()
            {
                return false;
            }
        };

        /**
         * Stream implementation for reading/writing to/from a FileIO handle.
         * This may be used alongside ObjectStream, or in async asset tasks.
         */
        class FileIOStream
            : public GenericStream
        {
        public:
            AZ_CLASS_ALLOCATOR(FileIOStream, SystemAllocator);
            FileIOStream();
            FileIOStream(HandleType fileHandle, AZ::IO::OpenMode mode, bool ownsHandle);
            FileIOStream(const char* path, AZ::IO::OpenMode mode, bool errorOnFailure = false);
            ~FileIOStream() override;

            bool Open(const char* path, AZ::IO::OpenMode mode);
            bool ReOpen() override;
            void Close() override;
            HandleType GetHandle() const;
            const char* GetFilename() const override;
            OpenMode GetModeFlags() const override;

            bool        IsOpen() const override;
            bool        CanSeek() const override;
            bool        CanRead() const override;
            bool        CanWrite() const override;
            void        Seek(OffsetType bytes, SeekMode mode) override;
            SizeType    Read(SizeType bytes, void* oBuffer) override;
            SizeType    Write(SizeType bytes, const void* iBuffer) override;
            SizeType    GetCurPos() const override;
            SizeType    GetLength() const override;

            virtual void Flush();

        private:
            HandleType m_handle;    ///< Open file handle.
            AZStd::string m_filename; ///< Stores filename for reopen support
            OpenMode m_mode; ///< Stores open mode flag to be used when reopening
            bool m_ownsHandle;      ///< Set if the FileIOStream instance created the handle (and should therefore auto-close it on destruction).
            bool m_errorOnFailure{ false };
        };
    } // namespace IO
} // namespace AZ
