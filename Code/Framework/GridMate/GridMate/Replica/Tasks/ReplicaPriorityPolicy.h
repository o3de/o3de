/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_REPLICA_PRIORITY_POLICY_H
#define GM_REPLICA_PRIORITY_POLICY_H

#include <GridMate/Replica/Tasks/ReplicaTask.h>
#include <GridMate/Replica/Replica.h>

namespace GridMate
{
    /**
    *  Dummy priotization policy that is not prioritizing tasks in any way
    */
    class NullPriorityPolicy
    {
    public:
        struct Compare
        {
            AZ_FORCE_INLINE bool operator()(const ReplicaTask* left, const ReplicaTask* right) const
            {
                (void)left;
                (void)right;
                return false;
            }
        };

        static void UpdatePriority(ReplicaTask& task)
        {
            (void)task;
        }
    };

    /**
    *  Prioritizing tasks by creation time, ignoring all other priorities, including user defined
    *  Arranges tasks in ascending order, meaning the earler replica is created - the higher priority it has
    */
    class CreateTimePriorityPolicy
    {
    public:
        struct Compare
        {
            AZ_FORCE_INLINE bool operator()(const ReplicaTask* left, const ReplicaTask* right) const
            {
                ReplicaPtr repLeft = left->GetReplica();
                ReplicaPtr repRight = right->GetReplica();

                if (repLeft->GetCreateTime() == repRight->GetCreateTime()) // if create time is the same -> tiebreaking on repId. Assuming repId are sequentially growing
                {
                    return repLeft->GetRepId() > repRight->GetRepId();
                }

                return (repLeft->GetCreateTime()) > (repRight->GetCreateTime());
            }
        };

        static void UpdatePriority(ReplicaTask& task)
        {
            (void)task;
        }
    };

    /**
    *  Priority policy used for Marshaling
    *  This priority policy calculates priorities based on Replica's CreateTime, and user defined priority
    *  Arranges tasks in descending order, meaning 0x00.. - is the lowest priority, 0xff.. - is the highest
    */
    class SendPriorityPolicy
    {
    public:
        struct Compare
        {
            AZ_FORCE_INLINE bool operator()(const ReplicaTask* left, const ReplicaTask* right) const
            {
                if (left->GetPriority() == right->GetPriority()) // if priority is the same -> tiebreaking on repId. Assuming repId are sequentially growing
                {
                    ReplicaPtr repLeft = left->GetReplica();
                    ReplicaPtr repRight = right->GetReplica();

                    return repLeft->GetRepId() > repRight->GetRepId();
                }

                return left->GetPriority() < right->GetPriority();
            }
        };

        static void UpdatePriority(ReplicaTask& task)
        {
            // building priority
            AZ::u64 pri = 0;
            ReplicaPtr rep = task.GetReplica();

            // aging
            ReplicaTask::PriorityType addAge = task.GetAge() * ReplicaTask::k_ageScale;
            addAge *= addAge;

            static_assert(sizeof(AZ::u32) == sizeof(rep->GetCreateTime()), "CreateTime cannot fit in 32 bits!");

            AZ::u32 timeDesc = ~rep->GetCreateTime();
            pri |= timeDesc; // put creation time in lower bits
            pri |= (static_cast<AZ::u64>(rep->GetPriority() + addAge) << sizeof(timeDesc) * CHAR_BIT); // put custom aged priority into bits 32-48

            task.SetPriority(pri);
        }
    };
} // namespace GridMate

#endif // GM_REPLICA_PRIORITY_POLICY_H
