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
    class BallJointComponent
        : public JointComponent
    {
    public:
        AZ_COMPONENT(BallJointComponent, "{914036AC-195E-4517-B58E-D29E42A560B9}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        BallJointComponent() = default;
        BallJointComponent(const GenericJointConfiguration& config
            , const GenericJointLimitsConfiguration& swingLimit);
        ~BallJointComponent() = default;

    protected:
        // JointComponent
        void InitNativeJoint() override;

    private:
        void InitSwingLimits();
    };
} // namespace PhysX
