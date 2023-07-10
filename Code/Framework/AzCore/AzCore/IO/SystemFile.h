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
#include <AzCore/std/chrono/chrono.h>
#include <AzCore/std/containers/span.h>
#include <AzCore/std/functional.h>
#include <AzCore/std/parallel/atomic.h>
#include <AzCore/std/parallel/thread.h>
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
                SF_SKIP_CLOSE_ON_DESTRUCTION = (1 << 8), ///< SystemFile destructor will not invoke Close() on an open handle
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
            //! Constructor which invokes Open() using the file name and mode
            SystemFile(const char* fileName, int mode, int platformFlags = 0);
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
            const char* Name() const { return m_fileName.c_str(); }
            bool IsOpen() const;

            /// Return native handle to the file.
            const FileHandleType& NativeHandle() const { return m_handle; }

            //////////////////////////////////////////////////////////////////////////
            //////////////////////////////////////////////////////////////////////////
            // Utility functions
            /// Check if a file or directory exists.
            static bool Exists(const char* path);
            /// Check if path is a directory
            static bool IsDirectory(const char* path);
            /// FindFiles
            using FindFileCB = AZStd::function<bool /* true to continue to enumerate otherwise false */ (const char* /* fileName*/, bool /* true if file, false if folder*/)>;
            static void FindFiles(const char* filter, FindFileCB cb);
            /// Get the time the file was last modified.
            static AZ::u64 ModificationTime(const char* fileName);
            /// Return a length of a file. 0 if files has 0 length or doesn't exits.
            static SizeType Length(const char* fileName);
            /// Read content from a file. If byteSize is 0 it reads the entire file.
            static SizeType Read(const char* fileName, void* buffer, SizeType byteSize = 0, SizeType byteOffset = 0);
            /// Delete a file, returns true if file was actually deleted.
            static bool Delete(const char* fileName);
            /// Rename a file, returns true if the file was successfully renamed. If overwrite is true, rename even if target exists.
            static bool Rename(const char* sourceFileName, const char* targetFileName, bool overwrite = false);
            /// Returns true if a file is writable, false if it doesn't exist or is read only
            static bool IsWritable(const char* sourceFileName);
            /// Returns true if able to modify readonly attribute. Can fail if file doesn't exist.
            static bool SetWritable(const char* sourceFileName, bool writable);
            /// Recursively creates a directory hierarchy
            static bool CreateDir(const char* dirName);
            /// Delete a directory
            static bool DeleteDir(const char* dirName);
            /// Retrieves platform specific Null device path (ex: "/dev/null")
            static const char* GetNullFilename();

            //! Opens stdin for read
            //! The destructor of SystemFile will not close the handle
            //! This allows stdin to continue to remain open even if the instance scope ends
            //! NOTE: However the handle can be explicitly closed using the Close() function
            //! Invoking that function will close the stdin handle
            static SystemFile GetStdin();
            //! Opens stdout for write
            //! The destructor of SystemFile will not close the handle
            //! This allows stdout to continue to remain open even if the instance scope ends
            //! NOTE: However the handle can be explicitly closed using the Close() function
            //! Invoking that function will close the stdout handle
            static SystemFile GetStdout();
            //! Opens stderr for write
            //! The destructor of SystemFile will not close the handle
            //! This allows stderr to continue to remain open even if the instance scope ends
            //! NOTE: However the handle can be explicitly closed using the Close() function
            //! Invoking that function will close the stderr handle
            static SystemFile GetStderr();

        private:
            static void CreatePath(const char * fileName);

            bool PlatformOpen(int mode, int platformFlags);
            void PlatformClose();

            FileHandleType m_handle;
            AZ::IO::FixedMaxPathString m_fileName;
            //! If true the destructor will invoke Close if the Handle is open
            bool m_closeOnDestruction{ true };
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

        /**
         * Utility class for capturing the output of file descriptor redirection using with RAII behavior.
         * NOTE: This class creates a new thread when Start() is called to pump the read end of the pipe
         * when it is filled with data.
         * The thread is terminated in Stop
         * Example:
         *
         *   printf("Test"); // prints to stdout
         *   AZ::IO::FileDescriptorCapturer redirectStdoutToFile(1);
         *   redirectStdoutToFile.Start();
         *   printf("Test"); // capture stdout as part of pipe
         *   bool testWasOutput{};
         *   auto StdoutVisitor [&testWasOutput](AZStd::span<const AZStd::byte> capturedBytes)
         *   {
         *       AZStd::string_view capturedString(reinterpret_cast<const char*>(capturedBytes.data()), captureBytes.size());
         *       testWasOutput = capturedString.contains("Test");
         *   };
         *   redirectStdout.Stop(StdoutVisitor); // Invokes visitor 0 or more times with captured data
         *   EXPECT_TRUE(testWasOutput);
         */
        class FileDescriptorCapturer
        {
        public:

            // 64 KiB for the default pipe size
            inline static constexpr int DefaultPipeSize = (1 << 16);

            FileDescriptorCapturer(int sourceDescriptor);
            ~FileDescriptorCapturer();

            //! Redirects file descriptor to a visitor callback
            //! Internally a pipe is used to send output to the visitor
            //!
            //! NOTE: This function will be called on a different thread than the one used to invoke Start()
            //! The caller is responsible for ensuring thread safety with the callback function
            using OutputRedirectVisitor = AZStd::function<void(AZStd::span<AZStd::byte>)>;

            //! Starts capture of file descriptor
            //! @param redirectCallback callback to invoke when data is available for read
            //! @param waitTimeout Timeout to wait for the descriptor to be signaled with data
            //!        default to 1 seconds
            //!        NOTE: This is not a timeout of the capture operation, but determines how frequently
            //!        a polling operation retries.
            //!        Using AZStd::chrono::milliseconds::max() results in an infinite timeout
            //! @param pipeSize used internally for pipe used to send captured data to the redirect callback
            //!        redirectCallback will not receieve more data than pipeSize in a single invocation
            void Start(OutputRedirectVisitor redirectCallback,
                AZStd::chrono::milliseconds waitTimeout = AZStd::chrono::seconds(1),
                int pipeSize = DefaultPipeSize);

            //! Stops capture of file descriptor and reset it back to it's previous value
            void Stop();

            // Writes to the original source descriptor, bypassing the capture
            int WriteBypassingCapture(const void* data, unsigned int size);

        private:
            //! Reads all the data from the pipe and sends it to the visitor
            //! returns true if the flush operation has succeeded
            bool Flush();

            // Any RedirectState value above DisconnectePipe
            // will cause the flush thread to exit
            enum class RedirectState : AZ::u8
            {
                Active = 0,
                ClosingPipeWriteSide = 64,
                DisconnectedPipe = 128,
                Idle = 255,
            };
            void Reset();
            int m_sourceDescriptor = -1;
            int m_dupSourceDescriptor = -1;
            inline static constexpr int ReadEnd = 0;
            inline static constexpr int WriteEnd = 1;
            intptr_t m_pipe[2]{ -1, -1 };
            //! atomic that is used to allow
            //! the Start method to be thread safe in relation to the Reset/Stop method.
            AZStd::atomic<RedirectState> m_redirectState{ RedirectState::Idle };

            //! Opaque data structure used for storing event or queue data used for
            //! event based signaling when the read end of the descriptor has available data
            intptr_t m_pipeData = 0;

            //! Thread responsible for flushing the descriptor to the callback function when full
            AZStd::thread m_flushThread;

            //! Callback which is pumped on the Flush Thread whenever pipe data is available for reading
            OutputRedirectVisitor m_redirectCallback;
        };
    }
}

namespace AZ::IO::Posix
{
    // Returns file descriptor from C FILE* struct
    int Fileno(FILE* stream);
}
