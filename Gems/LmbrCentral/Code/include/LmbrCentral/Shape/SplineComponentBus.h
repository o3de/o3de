/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/VertexContainerInterface.h>

namespace LmbrCentral
{
    enum class SplineType
    {
        LINEAR = 0,
        BEZIER,
        CATMULL_ROM
    };

    /// Services provided by the Spline Component.
    class SplineComponentRequests
        : public AZ::VariableVertices<AZ::Vector3>
    {
    public:
        /// Returns a reference to the underlying spline.
        virtual AZ::SplinePtr GetSpline() = 0;
        /// Change the type of interpolation used by the spline.
        /// @param splineType Refers to the RTTI Hash of the underlying Spline type
        /// (example: AZ::LinearSpline::RTTI_Type().GetHash()).
        virtual void ChangeSplineType(SplineType splineType) = 0;
        /// Set whether the spline should form a closed loop or not.
        virtual void SetClosed(bool closed) = 0;

    protected:
        ~SplineComponentRequests() = default;
    };

    /// Bus to service the Spline component event group.
    using SplineComponentRequestBus = AZ::EBus<SplineComponentRequests, AZ::ComponentBus>;

    /// Listener for spline changes.
    class SplineComponentNotification
        : public AZ::ComponentBus
        , public AZ::VertexContainerNotificationInterface<AZ::Vector3>
    {
    public:
        /// Called when the spline has changed.
        virtual void OnSplineChanged() {}

        /// Called when the Open/Close property is changed.
        virtual void OnOpenCloseChanged(bool /*closed*/) {}

        /// Called when a new vertex is added to spline.
        void OnVertexAdded(size_t /*index*/) override {}

        /// Called when a vertex is removed from spline.
        void OnVertexRemoved(size_t /*index*/) override {}

        /// Called when a vertex is updated.
        void OnVertexUpdated(size_t /*index*/) override {}

        /// Called when all vertices on the spline are set.
        void OnVerticesSet(const AZStd::vector<AZ::Vector3>& /*vertices*/) override {}

        /// Called when all vertices from the spline are removed.
        void OnVerticesCleared() override {}

    protected:
        ~SplineComponentNotification() = default;
    };

    /// Bus to service the spline component notification group.
    using SplineComponentNotificationBus = AZ::EBus<SplineComponentNotification>;
} // namespace LmbrCentral
