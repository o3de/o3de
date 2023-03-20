/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Process/TestImpactProcessInfo.h>

#include <AzCore/std/optional.h>

namespace TestImpact
{
    //! Abstraction of platform-specific process.
    class Process
    {
    public:
        explicit Process(const ProcessInfo& processInfo);
        virtual ~Process() = default;

        //! Terminates the process with the specified return code.
        virtual void Terminate(ReturnCode returnCode) = 0;

        //! Block the calling thread until the process exits.
        virtual void BlockUntilExit() = 0;

        //! Returns whether or not the process is still running.
        virtual bool IsRunning() const = 0;

        //! Returns the process info associated with this process.
        const ProcessInfo& GetProcessInfo() const;

        //! Returns the return code of the exited process.
        //! Will be empty if the process is still running or was not successfully launched.
        AZStd::optional<ReturnCode> GetReturnCode() const;

        //! Flushes the internal buffer and returns the process's buffered standard output.
        //! Subsequent calls will keep returning data so long as the process is producing output.
        //! Will return nullopt if no output routing or no output produced.
        virtual AZStd::optional<AZStd::string> ConsumeStdOut() = 0;

        //! Flushes the internal buffer and returns the process's buffered standard error.
        //! Subsequent calls will keep returning data so long as the process is producing errors.
        //! Will return nullopt if no error routing or no errors produced.
        virtual AZStd::optional<AZStd::string> ConsumeStdErr() = 0;

    protected:
        //! The information used to launch the process.
        ProcessInfo m_processInfo;

        //! The return code of a successfully launched process (otherwise is empty)
        AZStd::optional<ReturnCode> m_returnCode;
    };
} // namespace TestImpact
