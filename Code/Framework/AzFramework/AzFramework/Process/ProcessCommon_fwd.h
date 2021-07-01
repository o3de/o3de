/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AzFramework
{
    // Type of communication between parent and child processes
    enum ProcessCommunicationType
    {
        COMMUNICATOR_TYPE_STDINOUT,
        COMMUNICATOR_TYPE_NONE,
        //COMMUNICATOR_TYPE_IPC
    };

    enum ProcessPriority
    {
        // we don't support raising priority
        PROCESSPRIORITY_NORMAL,
        PROCESSPRIORITY_BELOWNORMAL,    // below other normal priorities
        PROCESSPRIORITY_IDLE,           // lowest possible priority
    };

    struct ProcessData;
    class ProcessOutput;
    class ProcessCommunicator;
    class ProcessCommunicatorForChildProcess;
    class StdProcessCommunicator;
    class StdProcessCommunicatorForChildProcess;
    class CommunicatorHandleImpl;
} // namespace AzFramework
