/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Process/ProcessCommon_fwd.h>
#include <AzFramework/AzFramework_Traits_Platform.h>

#if AZ_TRAIT_AZFRAMEWORK_PROCESSLAUNCH_DEFAULT
#include <Default/AzFramework/Process/ProcessCommon_Default.h>
#else
#include <AzFramework/Process/ProcessCommon.h>
#endif

namespace AzFramework
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

        // Waits for stdout or stderr to be ready for reading.  Note that its non-const
        // because it can detect if the outputs break and update them to be "broken".
        virtual void WaitForReadyOutputs(OutputStatus& outputStatus) = 0;

    protected:
        AZ_DISABLE_COPY(ProcessCommunicator);
        
        void ReadFromOutputs(ProcessOutput& processOutput, OutputStatus& status);

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
        virtual bool CreatePipesForProcess(AzFramework::ProcessData* processData) = 0;
    };

    //! Platform-specific class, default does nothing, but you can derive from it
    //! and supply it in your platform-specific implementation.  
    class StdInOutProcessCommunicatorData
    {
        public:
            StdInOutProcessCommunicatorData() = default;
            virtual ~StdInOutProcessCommunicatorData() = default;
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
        // AzFramework::ProcessCommunicator overrides
        bool IsValid() const override;
        AZ::u32 ReadError(void* readBuffer, AZ::u32 bufferSize) override;
        AZ::u32 PeekError() override;
        AZ::u32 ReadOutput(void* readBuffer, AZ::u32 bufferSize) override;
        AZ::u32 PeekOutput() override;
        AZ::u32 WriteInput(const void* writeBuffer, AZ::u32 bytesToWrite) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::StdProcessCommunicator overrides
        bool CreatePipesForProcess(ProcessData* processData) override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        void CreateHandles();
        void CloseAllHandles();

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::ProcessCommunicator overrides
        void WaitForReadyOutputs(OutputStatus& outputStatus) override;
        //////////////////////////////////////////////////////////////////////////

        AZStd::unique_ptr<CommunicatorHandleImpl> m_stdInWrite;
        AZStd::unique_ptr<CommunicatorHandleImpl> m_stdOutRead;
        AZStd::unique_ptr<CommunicatorHandleImpl> m_stdErrRead;

        //! OPTIONAL - platform-specific classes can plug in additional arbitrary platform
        //! specific data in here.  or leave it nullptr.
        AZStd::unique_ptr<StdInOutProcessCommunicatorData> m_communicatorData;
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
        // AzFramework::ProcessCommunicatorForChildProcess overrides
        bool IsValid() const override;
        AZ::u32 WriteError(const void* writeBuffer, AZ::u32 bytesToWrite) override;
        AZ::u32 WriteOutput(const void* writeBuffer, AZ::u32 bytesToWrite) override;
        AZ::u32 PeekInput() override;
        AZ::u32 ReadInput(void* buffer, AZ::u32 bufferSize) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::StdProcessCommunicatorForChildProcess overrides
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
} // namespace AzFramework
