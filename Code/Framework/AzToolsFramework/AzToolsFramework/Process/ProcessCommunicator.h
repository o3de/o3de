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

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzToolsFramework/Process/ProcessCommon_fwd.h>

#if defined(AZ_PLATFORM_WINDOWS)
    #include <AzToolsFramework/Process/internal/ProcessCommon_Win.h>
#elif AZ_TRAIT_OS_PLATFORM_APPLE
    #include <AzToolsFramework/Process/internal/ProcessCommon_OSX.h>
#elif defined(AZ_PLATFORM_LINUX)
    #include <AzToolsFramework/Process/internal/ProcessCommon_Linux.h>
#endif

namespace AzToolsFramework
{
    class ProcessOutput
    {
    public:
        AZStd::string outputResult;
        AZStd::string errorResult;

        void Clear();
        bool HasOutput() const;
        bool HasError() const;
    };

    class ProcessCommunicator
    {
    public:

        struct OutputStatus
        {
            bool outputDeviceReady = false;
            bool errorsDeviceReady = false;
            bool shouldReadOutput = false;
            bool shouldReadErrors = false;
        };

        ProcessCommunicator() = default;
        virtual ~ProcessCommunicator() = default;

        // Check if communicator is in a valid state
        virtual bool IsValid() const = 0;

        // Read error data into a given buffer size (returns amount of data read)
        // Blocking call (until child process writes data)
        virtual AZ::u32 ReadError(void* readBuffer, AZ::u32 bufferSize) = 0;

        // Peek if error data is ready to be read (returns amount of data available to read)
        // Non-blocking call
        virtual AZ::u32 PeekError() = 0;

        // Read output data into a given buffer size (returns amount of data read)
        // Blocking call (until child process writes data)
        virtual AZ::u32 ReadOutput(void* readBuffer, AZ::u32 bufferSize) = 0;

        // Peek if output data is ready to be read (returns amount of data available to read)
        // Non-blocking call
        virtual AZ::u32 PeekOutput() = 0;

        // Write input data to child process (returns amount of data sent)
        // Blocking call (until child process reads data)
        virtual AZ::u32 WriteInput(const void* writeBuffer, AZ::u32 bytesToWrite) = 0;

        // Waits for errors to be ready to read
        // Blocking call (until child process writes errors)
        AZ::u32 BlockUntilErrorAvailable(AZStd::string& readBuffer);

        // Waits for output to be ready to read
        // Blocking call (until child process writes output)
        AZ::u32 BlockUntilOutputAvailable(AZStd::string& readBuffer);

        // Reads into process output until the communicator's output handles are no longer valid
        void ReadIntoProcessOutput(ProcessOutput& processOutput);

    protected:
        AZ_DISABLE_COPY(ProcessCommunicator);
        
        // Waits for output or error to be ready for reading
        virtual void WaitForReadyOutputs(OutputStatus& outputStatus) const = 0;

        void ReadFromOutputs(ProcessOutput& processOutput,
            OutputStatus& status, char* buffer, AZ::u32 bufferSize);

    private:
        static const size_t s_readBufferSize = 16 * 1024;
    };

    class ProcessCommunicatorForChildProcess
    {
    public:

        ProcessCommunicatorForChildProcess() = default;
        virtual ~ProcessCommunicatorForChildProcess() = default;

        // Check if communicator is in a valid state
        virtual bool IsValid() const = 0;

        // Write error data to parent process (returns amount of data sent)
        // Blocking call (until parent process reads data)
        virtual AZ::u32 WriteError(const void* writeBuffer, AZ::u32 bytesToWrite) = 0;

        // Write output data to parent process (returns amount of data sent)
        // Blocking call (until parent process reads data)
        virtual AZ::u32 WriteOutput(const void* writeBuffer, AZ::u32 bytesToWrite) = 0;

        // Peek if input data is ready to be read (returns amount of data available to read)
        // Non-blocking call
        virtual AZ::u32 PeekInput() = 0;

        // Read input data into a given buffer size (returns amount of data read)
        // Blocking call (until parent process writes data)
        virtual AZ::u32 ReadInput(void* readBuffer, AZ::u32 bufferSize) = 0;

        // Waits for input to be ready to read
        // Blocking call (until parent process writes errors)
        AZ::u32 BlockUntilInputAvailable(AZStd::string& readBuffer);

    protected:
        AZ_DISABLE_COPY(ProcessCommunicatorForChildProcess);
    };

    using StdProcessCommunicatorHandle = AZStd::unique_ptr<CommunicatorHandleImpl>;

    class StdInOutCommunication
    {
    public: 
        virtual ~StdInOutCommunication() = default;

    protected:
        AZ::u32 PeekHandle(StdProcessCommunicatorHandle& handle);
        AZ::u32 ReadDataFromHandle(StdProcessCommunicatorHandle& handle, void* readBuffer, AZ::u32 bufferSize);
        AZ::u32 WriteDataToHandle(StdProcessCommunicatorHandle& handle, const void* writeBuffer, AZ::u32 bytesToWrite);
    };

    class StdProcessCommunicator
        : public ProcessCommunicator
    {
    public:
        virtual bool CreatePipesForProcess(AzToolsFramework::ProcessData* processData) = 0;
    };

    /**
    * Communicator to talk to processes via std::in and std::out
    *
    * to do this, it must provide handles for the child process to 
    * inherit before process creation
    */
    class StdInOutProcessCommunicator
        : public StdProcessCommunicator
        , public StdInOutCommunication
    {
    public:
        StdInOutProcessCommunicator();
        ~StdInOutProcessCommunicator();

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::ProcessCommunicator overrides
        bool IsValid() const override;
        AZ::u32 ReadError(void* readBuffer, AZ::u32 bufferSize) override;
        AZ::u32 PeekError() override;
        AZ::u32 ReadOutput(void* readBuffer, AZ::u32 bufferSize) override;
        AZ::u32 PeekOutput() override;
        AZ::u32 WriteInput(const void* writeBuffer, AZ::u32 bytesToWrite) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::StdProcessCommunicator overrides
        bool CreatePipesForProcess(ProcessData* processData) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        void CreateHandles();
        void CloseAllHandles();

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::ProcessCommunicator overrides
        void WaitForReadyOutputs(OutputStatus& outputStatus) const override;
        //////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<CommunicatorHandleImpl> m_stdInWrite;
        AZStd::unique_ptr<CommunicatorHandleImpl> m_stdOutRead;
        AZStd::unique_ptr<CommunicatorHandleImpl> m_stdErrRead;
        bool m_initialized = false;
    };

    class StdProcessCommunicatorForChildProcess
        : public ProcessCommunicatorForChildProcess
    {
    public:
        virtual bool AttachToExistingPipes() = 0;
    };

    class StdInOutProcessCommunicatorForChildProcess
        : public StdProcessCommunicatorForChildProcess
        , public StdInOutCommunication
    {
    public:
        StdInOutProcessCommunicatorForChildProcess();
        ~StdInOutProcessCommunicatorForChildProcess();

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::ProcessCommunicatorForChildProcess overrides
        bool IsValid() const override;
        AZ::u32 WriteError(const void* writeBuffer, AZ::u32 bytesToWrite) override;
        AZ::u32 WriteOutput(const void* writeBuffer, AZ::u32 bytesToWrite) override;
        AZ::u32 PeekInput() override;
        AZ::u32 ReadInput(void* buffer, AZ::u32 bufferSize) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzToolsFramework::StdProcessCommunicatorForChildProcess overrides
        bool AttachToExistingPipes() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        void CreateHandles();
        void CloseAllHandles();

        StdProcessCommunicatorHandle m_stdInRead;
        StdProcessCommunicatorHandle m_stdOutWrite;
        StdProcessCommunicatorHandle m_stdErrWrite;
        bool m_initialized = false;
    };
} // namespace AzToolsFramework
