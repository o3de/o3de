/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
    class BehaviorContext;
    class EditContext;
    class SerializeContext;
} // namespace AZ

// this just provides a convenient template to avoid the necessary boilerplate when you derive from AWSScriptBehaviorBase
#define AWS_SCRIPT_BEHAVIOR_DEFINITION(className, guidString) \
    AZ_TYPE_INFO(className, guidString) \
    AZ_CLASS_ALLOCATOR(className, AZ::SystemAllocator, 0) \
    void ReflectSerialization(AZ::SerializeContext* serializeContext) override; \
    void ReflectBehaviors(AZ::BehaviorContext* behaviorContext) override; \
    void ReflectEditParameters(AZ::EditContext* editContext) override; \
    className(); \

namespace AWSCore
{
    //! An interface for AWS ScriptCanvas Behaviors to inherit from 
    class AWSScriptBehaviorBase
    {
    public:
        virtual ~AWSScriptBehaviorBase() = default;

        virtual void ReflectSerialization(AZ::SerializeContext* reflectContext) = 0;
        virtual void ReflectBehaviors(AZ::BehaviorContext* behaviorContext) = 0;
        virtual void ReflectEditParameters(AZ::EditContext* editContext) = 0;

        virtual void Init() {}
        virtual void Activate() {}
        virtual void Deactivate() {}
    };
} // namespace AWSCore
