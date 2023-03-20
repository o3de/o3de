/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/Collision/CollisionLayers.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <AzFramework/Physics/CollisionBus.h>

namespace AzPhysics
{
    AZ_CLASS_ALLOCATOR_IMPL(CollisionLayer, AZ::SystemAllocator);
    AZ_CLASS_ALLOCATOR_IMPL(CollisionLayers, AZ::SystemAllocator);

    const CollisionLayer CollisionLayer::Default = 0;

    void CollisionLayer::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionLayer>()
                ->Version(1)
                ->Field("Index", &CollisionLayer::m_index)
                ;
        }
    }

    CollisionLayer::CollisionLayer(AZ::u8 index) 
        : m_index(index) 
    {
        AZ_Assert(m_index < CollisionLayers::MaxCollisionLayers, "Index is too large. Valid values are 0-%d"
            , CollisionLayers::MaxCollisionLayers - 1);
    }

    CollisionLayer::CollisionLayer(const AZStd::string& layerName)
    {
        CollisionLayer layer;
        Physics::CollisionRequestBus::BroadcastResult(layer, &Physics::CollisionRequests::GetCollisionLayerByName, layerName);
        m_index = layer.m_index;
    }

    AZ::u8 CollisionLayer::GetIndex() const
    {
        return m_index;
    }

    void CollisionLayer::SetIndex(AZ::u8 index)
    { 
        AZ_Assert(m_index < CollisionLayers::MaxCollisionLayers, "Index is too large. Valid values are 0-%d"
            , CollisionLayers::MaxCollisionLayers - 1);
        m_index = index;
    }

    AZ::u64 CollisionLayer::GetMask() const
    { 
        return 1ULL << m_index; 
    }

    bool CollisionLayer::operator==(const CollisionLayer& other) const
    {
        return m_index == other.m_index;
    }

    bool CollisionLayer::operator!=(const CollisionLayer& other) const
    {
        return !(*this == other);
    }

    void CollisionLayers::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CollisionLayers>()
                ->Version(1)
                ->Field("LayerNames", &CollisionLayers::m_names)
                ;

            if (auto* editContext = serializeContext->GetEditContext())
            {
                editContext->Class<CollisionLayers>("Collision Layers", "List of defined collision layers")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &CollisionLayers::m_names, "Layers", "Names of each collision layer")
                    ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
                    ;
            }
        }
    }

    CollisionLayer CollisionLayers::GetLayer(const AZStd::string& layerName) const
    {
        CollisionLayer layer = CollisionLayer::Default;
        TryGetLayer(layerName, layer);
        return layer;
    }

    bool CollisionLayers::TryGetLayer(const AZStd::string& layerName, CollisionLayer& layer) const
    {
        if (layerName.empty())
        {
            return false;
        }

        for (AZ::u8 i = 0; i < m_names.size(); ++i)
        {
            if (m_names[i] == layerName)
            {
                layer = CollisionLayer(i);
                return true;
            }
        }
        AZ_Warning("CollisionLayers", false, "Could not find collision layer:%s. Does it exist in the physx configuration window?", layerName.c_str());
        return false;
    }

    const AZStd::string& CollisionLayers::GetName(CollisionLayer layer) const
    {
        return m_names[layer.GetIndex()];
    }

    const AZStd::array<AZStd::string, CollisionLayers::MaxCollisionLayers>& CollisionLayers::GetNames() const
    {
        return m_names;
    }

    void CollisionLayers::SetName(CollisionLayer layer, const AZStd::string& layerName)
    {
        m_names[layer.GetIndex()] = layerName;
    }

    void CollisionLayers::SetName(AZ::u64 layerIndex, const AZStd::string& layerName)
    {
        if (layerIndex >= m_names.size())
        {
            AZ_Warning("PhysX Collision Layers", false, "Trying to set layer name of layer with invalid index: %d", layerIndex);
            return;
        }
        m_names[layerIndex] = layerName;
    }

    bool CollisionLayers::operator==(const CollisionLayers& other) const
    {
        return m_names == other.m_names;
    }

    bool CollisionLayers::operator!=(const CollisionLayers& other) const
    {
        return !(*this == other);
    }
}

