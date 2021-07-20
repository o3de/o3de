/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <fcntl.h>
#include <corecrt_io.h>
#include <sys/stat.h>

#include <AzCore/std/typetraits/underlying_type.h>

namespace AZ
{
    namespace IO
    {
        namespace Internal
        {
            using SizeType = AZ::u64;
            using SeekSizeType = AZ::s64;
            using FileHandleType = void*;
        }

        namespace PosixInternal
        {
            enum class OpenFlags : int
            {
                Append       = _O_APPEND,     // Moves the file pointer to the end of the file before every write operation.
                Create       = _O_CREAT,      // Creates a file and opens it for writing. Has no effect if the file specified by filename exists. PermissionMode is required.
                Temporary    = _O_TEMPORARY,  // Applies only when used with CREAT. Creates a file as temporary; the file is deleted when the last file descriptor is closed. PermissionMode equired when CREAT is specified.
                Exclusive    = _O_EXCL,       // Applies only when used with CREAT. Returns an error value if a file specified by filename exists.
                Truncate     = _O_TRUNC,      // Opens a file and truncates it to zero length; the file must have write permission. Cannot be specified with RDONLY.
                                              // Note: The TRUNC flag destroys the contents of the specified file.

                ReadOnly     = _O_RDONLY,     // Opens a file for reading only. Cannot be specified with RDWR or WRONLY.
                WriteOnly    = _O_WRONLY,     // Opens a file for writing only. Cannot be specified with RDONLY or RDWR.
                ReadWrite    = _O_RDWR,       // Opens a file for both reading and writing. Cannot be specified with RDONLY or WRONLY.

                // Windows-specific
                NoInherit    = _O_NOINHERIT,  // Prevents creation of a shared file descriptor.
                Random       = _O_RANDOM,     // Specifies that caching is optimized for, but not restricted to, random access from disk.
                Sequential   = _O_SEQUENTIAL, // Specifies that caching is optimized for, but not restricted to, sequential access from disk.
                Binary       = _O_BINARY,     // Opens the file in binary(untranslated) mode. (See fopen for a description of binary mode.)
                Text         = _O_TEXT,       // Opens a file in text(translated) mode. (For more information, see Text and Binary Mode File I/O and fopen.)
                U16Text      = _O_U16TEXT,    // Opens a file in Unicode UTF-16 mode.
                U8Text       = _O_U8TEXT,     // Opens a file in Unicode UTF-8 mode.
                WText        = _O_WTEXT,      // Opens a file in Unicode mode.

                // Unix-specific
                Async        = 0,             // (Not applicable in windows) Enable signal-driven I/O: generate a signal (SIGIO by default, but this can be changed via fcntl(2)) when input or output becomes possible on this file descriptor. This feature is available only for terminals, pseudoterminals, sockets, and (since Linux 2.6) pipes and FIFOs.
                Direct       = 0,             // (Not applicable in windows) Try to minimize cache effects of the I/O to and from this file. In general this will degrade performance, but it is useful in special situations, such as when applications do their own caching.
                Directory    = 0,             // (Not applicable in windows) If pathname is not a directory, cause the open to fail.
                NoFollow     = 0,             // (Not applicable in windows) If the trailing component (i.e., basename) of pathname is a symbolic link, then the open fails, with the error ELOOP. Symbolic links in earlier components of the pathname will still be followed.
                NoAccessTime = 0,             // (Not applicable in windows) Do not update the file last access time.
                Path         = 0,             // (Not applicable in windows) Obtain a file descriptor that can be used for two purposes: to indicate a location in the filesystem tree and to perform operations that act purely at the file descriptor level.
            };
            AZ_DEFINE_ENUM_BITWISE_OPERATORS(OpenFlags);

            enum class PermissionModeFlags : int
            {
                None = 0,
                Read = _S_IREAD,
                Write = _S_IWRITE
            };
            AZ_DEFINE_ENUM_BITWISE_OPERATORS(PermissionModeFlags);

            inline int Open(const char* const fileName, OpenFlags openFlags, PermissionModeFlags newFilePermissions = PermissionModeFlags::None)
            {
                int fileDescriptor = -1;
                int shareAccess = _SH_DENYNO; // < Deny nothing, allowing read/write sharing
                errno_t result = _sopen_s(&fileDescriptor, fileName, static_cast<int>(openFlags), shareAccess, static_cast<int>(newFilePermissions));
                if (result == 0)
                {
                    return fileDescriptor;
                }
                else
                {
                    return -1;
                }
            }

            inline int Close(int fileDescriptor)
            {
                return _close(fileDescriptor);
            }

            inline int Read(int fileDescriptor, void* data, unsigned int size)
            {
                return _read(fileDescriptor, data, size);
            }

            inline int Write(int fileDescriptor, const void* data, unsigned int size)
            {
                return _write(fileDescriptor, data, size);
            }

            inline int Dup(int fileDescriptor)
            {
                return _dup(fileDescriptor);
            }

            inline int Dup2(int fileDescriptorSource, int fileDescriptorDestination)
            {
                return _dup2(fileDescriptorSource, fileDescriptorDestination);
            }
        } // namespace AZ::IO::PosixInternal
    } // namespace AZ::IO
} // namespace AZ
