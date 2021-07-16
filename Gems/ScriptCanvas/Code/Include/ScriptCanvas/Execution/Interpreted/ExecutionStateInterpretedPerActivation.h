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
    class ExecutionStateInterpretedPerActivation
        : public ExecutionStateInterpreted
    {
    public:
        AZ_RTTI(ExecutionStateInterpretedPerActivation, "{79BFC45F-2487-456A-9599-3D43CFEABD14}", ExecutionStateInterpreted);
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpretedPerActivation, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);
        
        ExecutionStateInterpretedPerActivation(const ExecutionStateConfig& config);
            
        ~ExecutionStateInterpretedPerActivation() override;

        void Execute() override;

        void Initialize() override;

        void StopExecution() override;

    protected:
        bool m_deactivationRequired = false;
    };

    class ExecutionStateInterpretedPerActivationOnGraphStart
        : public ExecutionStateInterpretedPerActivation
    {
    public:
        AZ_RTTI(ExecutionStateInterpretedPerActivationOnGraphStart, "{039AA0BF-C179-4F9C-A7CD-248F24453C4B}", ExecutionStateInterpretedPerActivation);
        AZ_CLASS_ALLOCATOR(ExecutionStateInterpretedPerActivation, AZ::SystemAllocator, 0);

        static void Reflect(AZ::ReflectContext* reflectContext);

        ExecutionStateInterpretedPerActivationOnGraphStart(const ExecutionStateConfig& config);

        void Execute() override;

    private:
        bool m_deactivationRequired = false;
    };
} 
