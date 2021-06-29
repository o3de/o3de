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

#include <AzCore/Component/Component.h>
#include <Source/JointComponent.h>

namespace PhysX
{
    class FixedJointComponent
        : public JointComponent
    {
    public:
        AZ_COMPONENT(FixedJointComponent, "{02E6C633-8F44-4CEE-AE94-DCB06DE36422}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        FixedJointComponent() = default;
        explicit FixedJointComponent(const GenericJointConfiguration& config);
        FixedJointComponent(const GenericJointConfiguration& config, 
            const GenericJointLimitsConfiguration& limitConfig);
        ~FixedJointComponent() = default;

    protected:
        // JointComponent
        void InitNativeJoint() override;
    };
} // namespace PhysX
