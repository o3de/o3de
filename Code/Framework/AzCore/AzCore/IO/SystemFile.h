/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/base.h>

#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/IO/SystemFile_Platform.h>
#include <AzCore/std/function/function_fwd.h>
#include <AzCore/std/string/fixed_string.h>

// Establish a consistent size that works across platforms. It's actually larger than this
// on platforms we support, but this is a good least common denominator
#define AZ_MAX_PATH_LEN 1024

namespace AZ
{
    namespace IO
    {
        /**
         * Platform independent wrapper for system file.
         */
        class SystemFile
        {
        public:
            enum OpenMode
            {
                SF_OPEN_READ_ONLY   = (1 << 0),
                SF_OPEN_READ_WRITE  = (1 << 1),
                SF_OPEN_WRITE_ONLY  = (1 << 2),
                SF_OPEN_APPEND      = (1 << 3),   ///< All the writes will occur in the end of the file.
                SF_OPEN_CREATE_NEW  = (1 << 4),   ///< Create a file only if new, otherwise an error is returned.
                SF_OPEN_CREATE      = (1 << 5),   ///< Create a file, if file exists it will overwrite it's content.
                SF_OPEN_TRUNCATE    = (1 << 6),   ///< Opens a file and truncate it's size to zero. If file doesn't exist an error is returned.
                SF_OPEN_CREATE_PATH = (1 << 7),   ///< Also create any intermediate paths that are part of the file path. Must be used in conjunction with SF_OPEN_CREATE or SF_OPEN_CREATE_NEW.
            };

            enum SeekMode
            {
                SF_SEEK_BEGIN = 0,
                SF_SEEK_CURRENT,
                SF_SEEK_END,
            };

            using SizeType = AZ::IO::Internal::SizeType;
            using SeekSizeType = AZ::IO::Internal::SeekSizeType;
            using FileHandleType = AZ::IO::Internal::FileHandleType;

            SystemFile();
            ~SystemFile();

            SystemFile(SystemFile&&);
            SystemFile& operator=(SystemFile&&);

            /**
             * Opens a file.
             * \param fileName full file name including path
             * \param mode combination of OpenMode flags
             * \param platformFlags platform flags that will be | "or" with the mapping of the OpenMode flags.
             * \return true if the operation was successful otherwise false.
             */
            bool Open(const char* fileName, int mode, int platformFlags = 0);
            // ReOpen a file that has been open before (the name was stored before)
            bool ReOpen(int mode, int platformFlags = 0);
            /// Closes a file, if file already close it has no effect.
            void Close();
            /// Seek in current file.
            void Seek(SeekSizeType offset, SeekMode mode);
            /// Get the cursor position in the current file.
            SizeType Tell() const;
            /// Is the cursor at the end of the file?
            bool Eof() const;
            /// Get the time the file was last modified.
            AZ::u64 ModificationTime();
            /// Read data from a file synchronous. Return number of bytes actually read in the buffer.
            SizeType Read(SizeType byteSize, void* buffer);
            /// Writes data to a file synchronous. Return number of bytes actually written to the file.
            SizeType Write(const void* buffer, SizeType byteSize);
            /// Flush the contents of the file buffers to disk.
            void Flush();
            /// Return file length
            SizeType Length() const;
            /// Return disc offset if possible, otherwise 0
            SizeType DiskOffset() const;
            /// Return file name or NULL if file is not open.
            AZ_FORCE_INLINE const char* Name() const    { return m_fileName.c_str(); }
            bool IsOpen() const;

            /// Return native handle to the file.
            AZ_FORCE_INLINE const FileHandleType&   NativeHandle() const    { return m_handle; }

            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // Utility functions
            /// Check if a file or directory exists.
            static bool     Exists(const char* path);
            /// Check if path is a directory
            static bool     IsDirectory(const char* path);
            /// FindFiles
            typedef AZStd::function<bool /* true to continue to enumerate otherwise false */ (const char* /* fileName*/, bool /* true if file, false if folder*/)>  FindFileCB;
            static void     FindFiles(const char* filter, FindFileCB cb);
            /// Get the time the file was last modified.
            static AZ::u64  ModificationTime(const char* fileName);
            /// Return a length of a file. 0 if files has 0 length or doesn't exits.
            static SizeType Length(const char* fileName);
            /// Read content from a file. If byteSize is 0 it reads the entire file.
            static SizeType Read(const char* fileName, void* buffer, SizeType byteSize = 0, SizeType byteOffset = 0);
            /// Delete a file, returns true if file was actually deleted.
            static bool     Delete(const char* fileName);
            /// Rename a file, returns true if the file was successfully renamed. If overwrite is true, rename even if target exists.
            static bool     Rename(const char* sourceFileName, const char* targetFileName, bool overwrite = false);
            /// Returns true if a file is writable, false if it doesn't exist or is read only
            static bool     IsWritable(const char* sourceFileName);
            /// Returns true if able to modify readonly attribute. Can fail if file doesn't exist.
            static bool     SetWritable(const char* sourceFileName, bool writable);
            /// Recursively creates a directory hierarchy
            static bool     CreateDir(const char* dirName);
            /// Delete a directory
            static bool     DeleteDir(const char* dirName);
            /// Retrieves platform specific Null device path (ex: "/dev/null")
            static const char* GetNullFilename();

        private:
            static void CreatePath(const char * fileName);

            bool PlatformOpen(int mode, int platformFlags);
            void PlatformClose();

            FileHandleType m_handle;
            AZ::IO::FixedMaxPathString m_fileName;
        };

        /**
         * Utility class for performing file descriptor redirection with RAII behavior.
         * Example:
         *
         *   printf("Test"); // prints to stdout
         *   AZ::IO::FileRedirector redirectStdoutToFile(1);
         *   redirectStdoutToFile.RedirectTo("myfile.txt");
         *   printf("Test"); // prints to myfile.txt
         *   redirectStdout.Reset();
         *   {
         *       AZ::IO::FileDescriptorRedirector redirectStdoutToNull(1);
         *       redirectStdoutToNull.RedirectTo(AZ::IO::NullFilename);
         *       printf("Test"); // < prints nothing
         *   }
         *   printf("Test"); // < prints to stdout
         */

        class FileDescriptorRedirector
        {
        public:
            enum class Mode
            {
                Create,  // Creates/Replaces the output file
                Append   // Appends at the end of output the file
            };

            FileDescriptorRedirector(int sourceFileDescriptor);
            ~FileDescriptorRedirector();

            void RedirectTo(AZStd::string_view toFileName, Mode mode = Mode::Append);
            void Reset();

            // Writes to the original file descriptor, bypassing the redirection
            void WriteBypassingRedirect(const void* data, unsigned int size);

        private:

            int m_sourceFileDescriptor = -1;
            int m_dupSourceFileDescriptor = -1;
            int m_redirectionFileDescriptor = -1;
        };
    }
}
