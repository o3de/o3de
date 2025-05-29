/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/RTTI.h>

#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/FeatureProcessor.h>

namespace AZ
{
    namespace RPI
    {
        // forward declares
        class Scene;
        class AuxGeomDraw;
        using AuxGeomDrawPtr = AZStd::shared_ptr<AuxGeomDraw>;

        //! Interface of AuxGeom system, which is used for drawing Auxiliary Geometry, both for debug and things like editor manipulators.
        class ATOM_RPI_PUBLIC_API AuxGeomFeatureProcessorInterface
            : public FeatureProcessor
        {
        public:
            AZ_RTTI(AZ::RPI::AuxGeomFeatureProcessorInterface, "{2750EE44-5AE6-4379-BA3B-EDCD1507C997}", AZ::RPI::FeatureProcessor);

            AuxGeomFeatureProcessorInterface() = default;
            virtual ~AuxGeomFeatureProcessorInterface() = default;

            // Note that you have to delete these for safety reasons, you will trip a static_assert if you do not
            AZ_DISABLE_COPY_MOVE(AuxGeomFeatureProcessorInterface);

            //! Use this static helper to get the AuxGeom immediate draw interface for a given scene.
            //! Example usage:
            //! 
            //! auto defaultScene = AZ::RPI::RPISystemInterface::Get()->GetDefaultScene();
            //! if (auto auxGeom = AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(defaultScene))
            //! {
            //!     auxGeom->DrawPoint(drawArgs);
            //!     auxGeom->DrawLine(drawArgs);
            //! }
            static AuxGeomDrawPtr GetDrawQueueForScene(const ScenePtr scenePtr);
            static AuxGeomDrawPtr GetDrawQueueForScene(const Scene* scene);

            //! Get an AuxGeomDraw interface for drawing AuxGeom in the scene the FP is attached to.
            //! There is an inline static shortcut GetDrawQueueForScene above that avoids having to
            //! do the two steps of first getting the AuxGeomFeatureProcessorInterface pointer and then calling
            //! this function.
            virtual AuxGeomDrawPtr GetDrawQueue() = 0;

            //! Get the draw interface for drawing AuxGeom in immediate mode for the given view.
            //! Per view draw interfaces support 2d drawing.
            virtual AuxGeomDrawPtr GetDrawQueueForView(const View* view) = 0;

            //! Get existing or create a new AuxGeomDrawQueue object to store AuxGeom requests for this view.
            virtual AuxGeomDrawPtr GetOrCreateDrawQueueForView(const View* view) = 0;

            //! Feature processor releases the AuxGeomDrawQueue for the supplied view. DrawQueue is deleted when references fall to zero.
            virtual void ReleaseDrawQueueForView(const View* view) = 0;
        };
    } // namespace RPI
} // namespace AZ
