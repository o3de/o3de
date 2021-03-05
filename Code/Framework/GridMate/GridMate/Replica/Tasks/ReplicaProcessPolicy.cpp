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
#include <GridMate/Replica/Tasks/ReplicaProcessPolicy.h>
#include <GridMate/Replica/ReplicaMgr.h>

#include <AzCore/Math/MathUtils.h>

namespace GridMate
{
    static const float k_secToMilli = 1000.f;
    static const float k_initialDt = 0.1f; // dt on the first frame

    void SendLimitProcessPolicy::BeginFrame(ReplicaTask::RunContext& ctx)
    {
        ReplicaManager* rm = ctx.m_replicaManager;
        AZStd::chrono::system_clock::time_point now(AZStd::chrono::system_clock::now());
        float dt = AZStd::chrono::milliseconds(now - m_lastCheckTime).count() / k_secToMilli;
        AZ_Assert(dt >= 0.0f, "Frame duration < 0 seconds.");

        if (m_lastCheckTime.time_since_epoch() == AZStd::chrono::milliseconds::zero()) // clamping first frame
        {
            dt = k_initialDt;
        }

        m_lastCheckTime = now;

        {
            AZStd::lock_guard<AZStd::recursive_mutex> lock(rm->m_mutexRemotePeers);

            for (ReplicaPeer* peer : rm->m_remotePeers)
            {
                if (peer->GetConnectionId() == InvalidConnectionID)
                {
                    continue;
                }

                // Updating averages
                peer->m_dataSentLastSecond.Update(dt, peer->m_sentBytes);
                peer->m_avgSendRateBurst += (peer->m_dataSentLastSecond.GetSum() - peer->m_avgSendRateBurst) * (dt / rm->GetSendLimitBurstRange());

                if (dt >= 1.0f)
                {
                    peer->m_sendBytesAllowed = rm->GetSendLimit();      //Frame processing took >= one second, so reset send limit
                }
                else
                {
                    //Calculate carryOver
                    int carryOver = peer->m_sendBytesAllowed - peer->m_sentBytes;
                    // Calculate Window with carry over from last burst; max at BytesPerSecond
                    int allow = AZStd::GetMin(static_cast<unsigned int>(carryOver + (rm->GetSendLimit()*dt)), rm->GetSendLimit());
                    peer->m_sendBytesAllowed = AZStd::GetMax(allow, 0);
                    AZ_Assert(peer->m_sendBytesAllowed <= 100000000, "peer->m_sendBytesAllowed > 100,000,000");
                    //if(rm->GetSendLimit() > 0)
                    //{
                    //    AZ_Printf("GridMate", "allow %d carryOv %d dt %f sendLimit %d\n", allow, carryOver, dt, rm->GetSendLimit(), rm->GetSendLimit());
                    //}
                }

                peer->m_sentBytes = 0;      //Reset
            }
        }
    }

    bool SendLimitProcessPolicy::ShouldProcess(ReplicaTask::RunContext& ctx, ReplicaTask& task)
    {
        ReplicaManager* rm = ctx.m_replicaManager;
        if (!rm->GetSendLimit()) // no limiter set
        {
            return true;
        }

        const ReplicaPtr& replica = task.GetReplica();

        if ( ! rm->k_enableBackPressure && replica->GetPriority() == k_replicaPriorityRealTime) //if back-pressure disabled, don't limit real-time traffic
        {
            return true;
        }

        bool shouldProcess = false;
        for (ReplicaTarget& target : replica->m_targets)
        {
            shouldProcess |= (target.GetPeer()->m_sentBytes < target.GetPeer()->m_sendBytesAllowed);
        }

        if (replica->m_upstreamHop && replica->m_upstreamHop != &rm->m_self)
        {
            shouldProcess |= (replica->m_upstreamHop->m_sentBytes < replica->m_upstreamHop->m_sendBytesAllowed);
        }

        return shouldProcess;
    }
}