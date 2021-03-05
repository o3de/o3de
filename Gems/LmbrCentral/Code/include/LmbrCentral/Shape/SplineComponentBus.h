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
#include <AzCore/Math/Spline.h>
#include <AzCore/Math/VertexContainerInterface.h>

namespace LmbrCentral
{
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
        virtual void ChangeSplineType(AZ::u64 splineType) = 0;
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