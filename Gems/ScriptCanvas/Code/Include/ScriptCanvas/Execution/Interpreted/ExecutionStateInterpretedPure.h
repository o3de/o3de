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

#include "ExecutionStateInterpreted.h"

namespace ScriptCanvas
{
    class ExecutionStateInterpretedPure
        : public ExecutionStateInterpreted
    {
    public:
        AZ_RTTI(ExecutionStateInterpretedPure, "{EF702F22-F727-476A-A66A-A7F44687C194}", ExecutionStateInterpreted);
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpretedPure, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        ExecutionStateInterpretedPure(const ExecutionStateConfig& config);

        void Execute() override;

        void Initialize() override;

        void StopExecution() override;
    };

    class ExecutionStateInterpretedPureOnGraphStart
        : public ExecutionStateInterpretedPure
    {
    public:
        AZ_RTTI(ExecutionStateInterpretedPureOnGraphStart, "{D4CA9731-31CE-4B27-A91F-6E71E1DE8B7D}", ExecutionStateInterpretedPure);
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpretedPureOnGraphStart, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        ExecutionStateInterpretedPureOnGraphStart(const ExecutionStateConfig& config);

        void Execute() override;
    };
} 
