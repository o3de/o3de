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
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace LmbrCentral
{
    /// Common functionality and data for the SplineComponent.
    class SplineCommon
    {
    public:
        AZ_CLASS_ALLOCATOR(SplineCommon, AZ::SystemAllocator);
        AZ_RTTI(SplineCommon, "{91A31D7E-F63A-4AA8-BC50-909B37F0AD8B}");

        SplineCommon();
        virtual ~SplineCommon() = default;

        static void Reflect(AZ::ReflectContext* context);

        void ChangeSplineType(SplineType splineType);

        /// Override callbacks to be used when spline changes/is modified.
        void SetCallbacks(
            const AZ::IndexFunction& OnAddVertex, const AZ::IndexFunction& OnRemoveVertex,
            const AZ::IndexFunction& OnUpdateVertex, const AZ::VoidFunction& OnSetVertices,
            const AZ::VoidFunction& OnClearVertices, const AZ::VoidFunction& OnChangeType,
            const AZ::BoolFunction& OnOpenClose);

        AZ::SplinePtr m_spline; ///< Reference to the underlying spline data.

    private:
        AZ::u32 OnChangeSplineType();

        SplineType m_splineType = SplineType::LINEAR; ///< The currently set spline type (default to Linear).

        AZ::IndexFunction m_onAddVertex = nullptr;
        AZ::IndexFunction m_onRemoveVertex = nullptr;
        AZ::IndexFunction m_onUpdateVertex = nullptr;
        AZ::VoidFunction m_onSetVertices = nullptr;
        AZ::VoidFunction m_onClearVertices = nullptr;
        AZ::VoidFunction m_onChangeType = nullptr;
        AZ::BoolFunction m_onOpenCloseChange = nullptr;
    };

    /// Component interface to core spline implementation.
    class SplineComponent
        : public AZ::Component
        , private SplineComponentRequestBus::Handler
        , private AZ::TransformNotificationBus::Handler
    {
    public:
        friend class EditorSplineComponent;

        AZ_COMPONENT(SplineComponent, "{F0905297-1E24-4044-BFDA-BDE3583F1E57}");

        // AZ::Component
        void Activate() override;
        void Deactivate() override;

        // SplineComponentRequestBus
        AZ::SplinePtr GetSpline() override;
        void ChangeSplineType(SplineType splineType) override;
        void SetClosed(bool closed) override;

        // SplineComponentRequestBus/VertexContainerInterface
        bool GetVertex(size_t index, AZ::Vector3& vertex) const override;
        void AddVertex(const AZ::Vector3& vertex) override;
        bool UpdateVertex(size_t index, const AZ::Vector3& vertex) override;
        bool InsertVertex(size_t index, const AZ::Vector3& vertex) override;
        bool RemoveVertex(size_t index) override;
        void SetVertices(const AZStd::vector<AZ::Vector3>& vertices) override;
        void ClearVertices() override;
        size_t Size() const override;
        bool Empty() const override;

        // TransformNotificationBus
        void OnTransformChanged(const AZ::Transform& local, const AZ::Transform& world) override;

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SplineService"));
            provided.push_back(AZ_CRC_CE("VariableVertexContainerService"));
            provided.push_back(AZ_CRC_CE("FixedVertexContainerService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SplineService"));
            incompatible.push_back(AZ_CRC_CE("VariableVertexContainerService"));
            incompatible.push_back(AZ_CRC_CE("FixedVertexContainerService"));
            incompatible.push_back(AZ_CRC_CE("NonUniformScaleService"));
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("TransformService"));
        }

        static void Reflect(AZ::ReflectContext* context);

    private:
        SplineCommon m_splineCommon; ///< Stores common spline functionality and properties.
        AZ::Transform m_currentTransform; ///< Caches the current transform for the entity on which this component lives.
    };

} // namespace LmbrCentral
