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
#ifndef GM_REPLICA_PROCESS_POLICY_H
#define GM_REPLICA_PROCESS_POLICY_H

#include <GridMate/Replica/Tasks/ReplicaTask.h>

namespace GridMate
{
    /**
    *  Dummy replica tasks process policy that does nothing
    */
    class NullProcessPolicy
    {
    public:
        void BeginFrame(ReplicaTask::RunContext& ctx) { (void)ctx; }
        void EndFrame(ReplicaTask::RunContext& ctx) { (void)ctx; }
        bool ShouldProcess(ReplicaTask::RunContext& ctx, ReplicaTask& task) { (void)ctx; (void)task; return true; }
    };

    /**
    *  Process policy that limits tasks processing by bandwidth usage per peer
    *  Task will still be processed if any of the target peers is not bandwidth limited
    *  or if upstream peer is not limited. Meaning, the actual send rate might be
    *  slightly bigger than the limit set on replica manager.
    */
    class SendLimitProcessPolicy
    {
    public:
        void BeginFrame(ReplicaTask::RunContext& ctx);
        void EndFrame(ReplicaTask::RunContext& ctx) { (void)ctx; }
        bool ShouldProcess(ReplicaTask::RunContext& ctx, ReplicaTask& task);

    private:
        AZStd::chrono::system_clock::time_point m_lastCheckTime;
    };
} // namespace GridMate

#endif // GM_REPLICA_PROCESS_POLICY_H
