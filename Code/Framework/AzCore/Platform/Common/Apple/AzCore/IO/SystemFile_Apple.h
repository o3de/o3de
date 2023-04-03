/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <sys/syslimits.h>
#include <fcntl.h>
#include <unistd.h>
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
            using FileHandleType = int;
        }

        namespace PosixInternal
        {
            enum class OpenFlags : int
            {
                Append       = O_APPEND,     // Moves the file pointer to the end of the file before every write operation.
                Create       = O_CREAT,      // Creates a file and opens it for writing. Has no effect if the file specified by filename exists. PermissionMode is required.
#ifdef O_TMPFILE
                Temporary    = O_TMPFILE,    // Applies only when used with CREAT. Creates a file as temporary; the file is deleted when the last file descriptor is closed. PermissionMode equired when CREAT is specified.
#else
                Temporary    = 0,            // (Not applicable for this platform) Applies only when used with CREAT. Creates a file as temporary; the file is deleted when the last file descriptor is closed. PermissionMode equired when CREAT is specified.
#endif
                Exclusive    = O_EXCL,       // Applies only when used with CREAT. Returns an error value if a file specified by filename exists.
                Truncate     = O_TRUNC,      // Opens a file and truncates it to zero length; the file must have write permission. Cannot be specified with RDONLY.
                                             // Note: The TRUNC flag destroys the contents of the specified file.

                ReadOnly     = O_RDONLY,     // Opens a file for reading only. Cannot be specified with RDWR or WRONLY.
                WriteOnly    = O_WRONLY,     // Opens a file for writing only. Cannot be specified with RDONLY or RDWR.
                ReadWrite    = O_RDWR,       // Opens a file for both reading and writing. Cannot be specified with RDONLY or WRONLY.

                // Windows-specific
                NoInherit    = 0,            // (Not applicable in unix) Prevents creation of a shared file descriptor.
                Random       = 0,            // (Not applicable in unix) Specifies that caching is optimized for, but not restricted to, random access from disk.
                Sequential   = 0,            // (Not applicable in unix) Specifies that caching is optimized for, but not restricted to, sequential access from disk.
                Binary       = 0,            // (Not applicable in unix) Opens the file in binary(untranslated) mode. (See fopen for a description of binary mode.)
                Text         = 0,            // (Not applicable in unix) Opens a file in text(translated) mode. (For more information, see Text and Binary Mode File I/O and fopen.)
                U16Text      = 0,            // (Not applicable in unix) Opens a file in Unicode UTF-16 mode.
                U8Text       = 0,            // (Not applicable in unix) Opens a file in Unicode UTF-8 mode.
                WText        = 0,            // (Not applicable in unix) Opens a file in Unicode mode.

                // Unix-specific
                Async        = O_ASYNC,      // Enable signal-driven I/O: generate a signal (SIGIO by default, but this can be changed via fcntl(2)) when input or output becomes possible on this file descriptor. This feature is available only for terminals, pseudoterminals, sockets, and (since Linux 2.6) pipes and FIFOs.
                Direct       = 0,            // Try to minimize cache effects of the I/O to and from this file. In general this will degrade performance, but it is useful in special situations, such as when applications do their own caching.
                Directory    = O_DIRECTORY,  // If pathname is not a directory, cause the open to fail.
                NoFollow     = O_NOFOLLOW,   // If the trailing component (i.e., basename) of pathname is a symbolic link, then the open fails, with the error ELOOP. Symbolic links in earlier components of the pathname will still be followed.
#ifdef O_NOATIME
                NoAccessTime = O_NOATIME,    // Do not update the file last access time.
#else
                NoAccessTime = 0,            // (Not applicable for this platform) Do not update the file last access time.
#endif

#ifdef O_PATH
                Path         = O_PATH,       // Obtain a file descriptor that can be used for two purposes: to indicate a location in the filesystem tree and to perform operations that act purely at the file descriptor level.
#else
                Path         = 0,            // (Not applicable for this platform) Obtain a file descriptor that can be used for two purposes: to indicate a location in the filesystem tree and to perform operations that act purely at the file descriptor level.
#endif
                NonBlock     = O_NONBLOCK,   // Opens a pipe in non-blocking mode. If the process tries to perform incompatible access on a file region with an incompatible mandatory lock when this flag is set, then system call fails and returns EAGAIN.
            };
            AZ_DEFINE_ENUM_BITWISE_OPERATORS(OpenFlags);

            enum class PermissionModeFlags : int
            {
                None = 0,
                Read = S_IRUSR,
                Write = S_IWUSR
            };
            AZ_DEFINE_ENUM_BITWISE_OPERATORS(PermissionModeFlags);

            inline int Open(const char* const fileName, OpenFlags openFlags,
                PermissionModeFlags newFilePermissions = PermissionModeFlags::None)
            {
                return open(fileName, static_cast<int>(openFlags), static_cast<int>(newFilePermissions));
            }

            inline int Close(int fileDescriptor)
            {
                return close(fileDescriptor);
            }

            inline int Read(int fileDescriptor, void* data, size_t size)
            {
                return static_cast<int>(read(fileDescriptor, data, size));
            }

            inline int Write(int fileDescriptor, const void* data, size_t size)
            {
                return static_cast<int>(write(fileDescriptor, data, size));
            }

            int Dup(int fileDescriptor);
            int Dup2(int fileDescriptorSource, int fileDescriptorDestination);

            int Pipe(int(&pipeFileDescriptors)[2], int pipeSize, OpenFlags flags);
        } // namespace AZ::IO::PosixInternal
    } // namespace AZ::IO
} // namespace AZ
