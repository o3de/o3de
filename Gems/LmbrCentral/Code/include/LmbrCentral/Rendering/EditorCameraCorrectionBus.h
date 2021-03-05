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

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Matrix3x3.h>

namespace LmbrCentral
{
    // EditorCameraCorrectionRequests allows a component to add a pre multiply transform to the editor camera so that
    // the component appears to be the orientation that the editor camera expects (looking down the y-axis).
    // For example, projector lights project along the x-axis, so the editor light component uses this bus so that 
    // the "Look through entity" camera is positioned to look along the x-axis of the entity.
    class EditorCameraCorrectionRequests
        : public AZ::ComponentBus
    {
    public:
        virtual AZ::Matrix3x3 GetTransformCorrection() { return AZ::Matrix3x3::CreateIdentity(); }
        AZ::Matrix3x3 GetInverseTransformCorrection() { return GetTransformCorrection().GetInverseFast(); }

    protected:
        ~EditorCameraCorrectionRequests() = default;
    };
    
    using EditorCameraCorrectionRequestBus = AZ::EBus<EditorCameraCorrectionRequests>;
}