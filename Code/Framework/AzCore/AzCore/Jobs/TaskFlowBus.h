/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace tf
{
    class Executor;
}

namespace AZ
{
    /**
     * Interface to query for the AZ global executor
     */
    class TaskFlowExecutorInterface
    {
    public:
        AZ_RTTI(TaskFlowExecutorInterface, "{8DC58A99-A9C2-42E8-B27D-778849341E8D}");

        TaskFlowExecutorInterface() = default;
        virtual ~TaskFlowExecutorInterface() = default;
        virtual tf::Executor* GetExecutor() = 0;

    private:
        TaskFlowExecutorInterface(TaskFlowExecutorInterface&) = delete;
        TaskFlowExecutorInterface& operator=(TaskFlowExecutorInterface&) = delete;
    };

}
