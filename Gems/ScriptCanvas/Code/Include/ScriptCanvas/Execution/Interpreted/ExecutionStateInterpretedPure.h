/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "ExecutionStateInterpreted.h"

namespace ScriptCanvas
{
    class ExecutionStateInterpretedPure
        : public ExecutionStateInterpreted
    {
    public:
        AZ_RTTI(ExecutionStateInterpretedPure, "{EF702F22-F727-476A-A66A-A7F44687C194}", ExecutionStateInterpreted);
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpretedPure, AZ::SystemAllocator);

        ExecutionStateInterpretedPure(ExecutionStateConfig& config);

        void Execute() override;

        void Initialize() override;

        bool IsPure() const override;

        void StopExecution() override;
    };

    class ExecutionStateInterpretedPureOnGraphStart
        : public ExecutionStateInterpretedPure
    {
    public:
        AZ_RTTI(ExecutionStateInterpretedPureOnGraphStart, "{D4CA9731-31CE-4B27-A91F-6E71E1DE8B7D}", ExecutionStateInterpretedPure);
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpretedPureOnGraphStart, AZ::SystemAllocator);

        ExecutionStateInterpretedPureOnGraphStart(ExecutionStateConfig& config);

        void Execute() override;
    };
} 
