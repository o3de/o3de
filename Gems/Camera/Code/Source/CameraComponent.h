/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once
#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/ComponentAdapter.h>

#include <AzFramework/Components/CameraBus.h>
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
