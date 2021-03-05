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

namespace LmbrCentral
{
    namespace SplineAttributeUtil
    {
        template<typename AttributeType>
        AttributeType CreateElement()
        {
            return AttributeType();
        }

        template<>
        inline AZ::Vector3 CreateElement<AZ::Vector3>()
        {
            return AZ::Vector3::CreateZero();
        }

        template<>
        inline AZ::Transform CreateElement<AZ::Transform>()
        {
            return AZ::Transform::CreateIdentity();
        }

        template<>
        inline AZ::Color CreateElement<AZ::Color>()
        {
            return AZ::Color::CreateOne();
        }
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::Reflect(AZ::SerializeContext& context)
    {
        context.Class<SplineAttribute<AttributeType>>()
            ->Field("Elements", &SplineAttribute<AttributeType>::m_elements);

        if (AZ::EditContext* editContext = context.GetEditContext())
        {
            editContext->Class<SplineAttribute<AttributeType>>("SplineAttribute", "Attribute of a spline")
                // The dynamic edit data provider allows us to have different UI edit controls for each instance of a SplineAttribute.
                ->SetDynamicEditDataProvider(&SplineAttribute<AttributeType>::GetElementDynamicEditData)
                ->DataElement(0, &SplineAttribute<AttributeType>::m_elements, "Elements", "Elements in the attribute")
                ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                ;
        }
    }

    /// Dynamic edit data provider function.  We use this to dynamically override the edit context for each element in the SplineAttribute.
    /// This enables components to set component-specific ranges and UI controls.
    /// handlerPtr: pointer to the object whose edit data registered the handler (i.e. the class instance pointer)
    /// elementPtr: pointer to the sub-member of handlePtr that we are querying edit data for (i.e. the member variable)
    /// elementType: uuid of the specific class type of the elementPtr
    /// The function can either return a pointer to the ElementData to use, or nullptr to use the default one.
    template<typename AttributeType>
    const AZ::Edit::ElementData* SplineAttribute<AttributeType>::GetElementDynamicEditData(const void* handlerPtr, const void* elementPtr, const AZ::Uuid& elementType)
    {
        const SplineAttribute<AttributeType>* classInstance = reinterpret_cast<const SplineAttribute<AttributeType>*>(handlerPtr);
        if (classInstance)
        {
            // If our elementPtr isn't m_elements, we assume it must be one of the actual elements.
            // WARNING:  If the members of SplineAttribute ever gets modified, this check will need to change to encompass the
            // new set of members as well.
            if (elementPtr != &(classInstance->m_elements))
            {
                // Secondary check - make sure the type matches the element type in m_elements.
                if (elementType == AZ::AzTypeInfo<AttributeType>::Uuid())
                {
                    // Yes, so return our overridden edit data context if it was set (or the default if it wasn't).
                    if (classInstance->m_elementEditData.m_name != nullptr)
                    {
                        return &(classInstance->m_elementEditData);
                    }
                }

            }
        }

        // This wasn't one of the attribute elements, so don't override the edit context.
        return nullptr;
    }

    template<typename AttributeType>
    SplineAttribute<AttributeType>::SplineAttribute(size_t size)
    {
        m_elements.resize(size, SplineAttributeUtil::CreateElement<AttributeType>());
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::Activate(AZ::EntityId entityId)
    {
        m_entityId = entityId;
        SplineComponentNotificationBus::Handler::BusConnect(entityId);
        AZ::SplinePtr spline;
        SplineComponentRequestBus::EventResult(spline, entityId, &SplineComponentRequests::GetSpline);
        m_elements.resize(spline->GetVertexCount(), SplineAttributeUtil::CreateElement<AttributeType>());
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::Deactivate()
    {
        SplineComponentNotificationBus::Handler::BusDisconnect();
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::SetElement(size_t index, AttributeType value)
    {
        m_elements[index] = value;
    }

    template<typename AttributeType>
    AttributeType SplineAttribute<AttributeType>::GetElement(size_t index) const
    {
        return m_elements[index];
    }

    template<typename AttributeType>
    AttributeType SplineAttribute<AttributeType>::GetElementInterpolated(size_t index, float fraction, Interpolator interpolator) const
    {
        if (m_elements.size() > 0)
        {
            size_t indexWrapped = index % m_elements.size();
            size_t nextIndexWrapped = (index + 1) % m_elements.size();

            return interpolator(m_elements[indexWrapped], m_elements[nextIndexWrapped], fraction);
        }

        return SplineAttributeUtil::CreateElement<AttributeType>();
    }

    template<typename AttributeType>
    AttributeType SplineAttribute<AttributeType>::GetElementInterpolated(const AZ::SplineAddress& address, Interpolator interpolator) const
    {
        return GetElementInterpolated(address.m_segmentIndex, address.m_segmentFraction, interpolator);
    }

    template<typename AttributeType>
    size_t SplineAttribute<AttributeType>::Size() const
    {
        return m_elements.size();
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::OnVertexAdded(size_t index)
    {
        m_elements.insert(m_elements.begin() + index, SplineAttributeUtil::CreateElement<AttributeType>());
        SplineAttributeNotificationBus::Event(m_entityId, &SplineAttributeNotifications::OnAttributeAdded, index);
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::OnVertexRemoved(size_t index)
    {
        m_elements.erase(m_elements.begin() + index);
        SplineAttributeNotificationBus::Event(m_entityId, &SplineAttributeNotifications::OnAttributeRemoved, index);
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::OnVerticesSet(const AZStd::vector<AZ::Vector3>& vertices)
    {
        m_elements.resize(vertices.size(), SplineAttributeUtil::CreateElement<AttributeType>());
        SplineAttributeNotificationBus::Event(m_entityId, &SplineAttributeNotifications::OnAttributesSet, vertices.size());
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::OnVerticesCleared()
    {
        m_elements.clear();
        SplineAttributeNotificationBus::Event(m_entityId, &SplineAttributeNotifications::OnAttributesCleared);
    }

    template<typename AttributeType>
    const AZ::Edit::ElementData& SplineAttribute<AttributeType>::GetElementEditData() const
    {
        return m_elementEditData;
    }

    template<typename AttributeType>
    void SplineAttribute<AttributeType>::SetElementEditData(const AZ::Edit::ElementData &elementData)
    {
        m_elementEditData = elementData;
    }
} // namespace LmbrCentral