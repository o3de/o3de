/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/EditContext.h>

#include <LmbrCentral/Shape/SplineComponentBus.h>

namespace LmbrCentral
{
    /// Messages broadcast when spline attributes are changed
    class SplineAttributeNotifications
        : public AZ::ComponentBus
    {
    public:
        /// Call when a new attribute is added
        virtual void OnAttributeAdded(size_t index) = 0;

        /// Called when an attribute is removed
        virtual void OnAttributeRemoved(size_t index) = 0;

        /// Called when a a new set of attributes is set
        virtual void OnAttributesSet(size_t size) = 0;

        /// Called when all attributes are cleared
        virtual void OnAttributesCleared() = 0;
    };

    using SplineAttributeNotificationBus = AZ::EBus<SplineAttributeNotifications>;

    /// @brief Attributes are used to attach auxilary data to each vertex in a spline.
    /// @description The elements of the attribute are of homogenous type. New elements can not be added or removed
    /// manually as the number of elements always matches the number of vertices in the spline.
    template<typename AttributeType>
    class SplineAttribute
        : public SplineComponentNotificationBus::Handler
    {
    public:
        AZ_RTTI(SplineAttribute<AttributeType>, "{A8E73C3D-65DD-43D8-A25E-2E35674B4B78}");
        AZ_CLASS_ALLOCATOR(SplineAttribute<AttributeType>, AZ::SystemAllocator);
        static void Reflect(AZ::SerializeContext& context);

        SplineAttribute() = default;
        explicit SplineAttribute(size_t size);

        void Activate(AZ::EntityId entityId);
        void Deactivate();

        /// Interpolation function.
        using Interpolator = AZStd::function<AttributeType(AttributeType, AttributeType, float)>;

        /// Sets an element in the spline attribute.
        /// @param index The index of the element to set.
        /// @param value The value of the element to set.
        void SetElement(size_t index, AttributeType value);

        /// Gets an element in the attribute.
        /// @param index The index of the element to get.
        /// @return the value of the point at the index specified.
        AttributeType GetElement(size_t index) const;

        /// Evaluates an interpolated value between two elements in the attribute.
        /// @param index The beginning index.
        /// @param fraction The percentage between this index and the next.
        /// @param interpolator A function to interpolate between the elements.
        /// @return the interpolated value.
        AttributeType GetElementInterpolated(size_t index, float fraction, Interpolator interpolator) const;

        /// Evaluates an interpolated value between two elements in the attribute.
        /// @param address The address along the spline.
        /// @param interpolator A function to interpolate between the elements.
        /// @return the interpolated value.
        AttributeType GetElementInterpolated(const AZ::SplineAddress& address, Interpolator interpolator) const;

        /// Returns the number of elements in the attribute.
        /// @return The number of elements in the attribute.
        size_t Size() const;

        /// Returns the custom edit context data that we're using for each element in the attribute
        /// @return The custom edit context data that we're using for each element in the attribute.
        const AZ::Edit::ElementData& GetElementEditData() const;


        /// Sets custom edit context data to use for each element in the attribute.
        /// This can be used to set sliders, ranges, etc on the SplineAttribute that are custom to a specific component.
        /// @param elementData The ElementData for us to copy and use for this instance of the SplineAttribute.
        void SetElementEditData(const AZ::Edit::ElementData &elementData);


    protected:
        // SplineComponentNotificationBus
        void OnVertexAdded(size_t index) override;
        void OnVertexRemoved(size_t index) override;
        void OnVerticesSet(const AZStd::vector<AZ::Vector3>& vertices) override;
        void OnVerticesCleared() override;

        static const AZ::Edit::ElementData* GetElementDynamicEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType);

        AZ::EntityId m_entityId;
        AZStd::vector<AttributeType> m_elements; ///< The attribute data elements.
        AZ::Edit::ElementData m_elementEditData; ///< Custom editContext description to apply to each element
    };

    namespace SplineAttributeUtil
    {
        template<typename AttributeType>
        AttributeType CreateElement();
    }
} // namespace LmbrCentral

#include "SplineAttribute.inl"
