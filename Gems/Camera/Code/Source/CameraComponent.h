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
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/ComponentAdapter.h>

#include <AzFramework/Components/CameraBus.h>
#include <IViewSystem.h>
#include <ISystem.h>
#include <Cry_Camera.h>
#include <CameraComponentController.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewProviderBus.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>

namespace Camera
{
    //////////////////////////////////////////////////////////////////////////
    /// The CameraComponent holds all of the data necessary for a camera.
    /// Get and set data through the CameraRequestBus or TransformBus
    //////////////////////////////////////////////////////////////////////////
    using CameraComponentBase = AzFramework::Components::ComponentAdapter<CameraComponentController, CameraComponentConfig>;
    class CameraComponent
        : public CameraComponentBase
    {
    public:
        AZ_COMPONENT(CameraComponent, CameraComponentTypeId, AZ::Component);
        CameraComponent() = default;
        CameraComponent(const CameraComponentConfig& properties);
        virtual ~CameraComponent() = default;

        // AZ::Component interface
        void Activate() override;
        void Deactivate() override;

        static void Reflect(AZ::ReflectContext* reflection);
    };
} // Camera
