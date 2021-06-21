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
    class HingeJointComponent
        : public JointComponent
    {
    public:
        AZ_COMPONENT(HingeJointComponent, "{A5CA0031-72E4-4908-B764-EDECD3091882}", JointComponent);

        static void Reflect(AZ::ReflectContext* context);

        HingeJointComponent() = default;
        HingeJointComponent(
            const JointComponentConfiguration& configuration, 
            const JointGenericProperties& genericProperties,
            const JointLimitProperties& limitProperties);
        ~HingeJointComponent() = default;

    protected:
        // JointComponent
        void InitNativeJoint() override;
    };
} // namespace PhysX
