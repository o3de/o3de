/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "TestImpactWin32_Handle.h"

#include <AzCore/PlatformIncl.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace TestImpact
{
    //! RAII wrapper around OS pipes.
    //! Used to connect the standard output and standard error of the child process to a sink accessible to the
    //! parent process to allow the parent process to read the output(s) of the child process.
    class Pipe
    {
    public:
        Pipe(SECURITY_ATTRIBUTES& sa, HANDLE& stdChannel);
        Pipe(Pipe&& other) = delete;
        Pipe(Pipe& other) = delete;
        Pipe& operator=(Pipe& other) = delete;
        Pipe& operator=(Pipe&& other) = delete;

        //! Releases the child end of the pipe (not needed once parent has their end).
        void ReleaseChild();

        //! Empties the contents of the pipe into the internal buffer.
        void EmptyPipe();

        //! Empties the contents of the pipe into a string, clearing the internal buffer.
        AZStd::string GetContentsAndClearInternalBuffer();

    private:
        // Parent and child process ends of pipe
        ObjectHandle m_parent;
        ObjectHandle m_child;

        // Buffer for emptying pipe upon child processes exit
        AZStd::vector<char> m_buffer;
    };
} // namespace TestImpact
