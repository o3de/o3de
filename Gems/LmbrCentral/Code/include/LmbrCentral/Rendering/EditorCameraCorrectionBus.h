/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
