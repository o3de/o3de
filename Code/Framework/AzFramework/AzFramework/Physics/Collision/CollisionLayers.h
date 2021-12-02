/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/RTTI/TypeInfoSimple.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/std/containers/array.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzPhysics
{
    //! This class represents which layer a collider exists on.
    //! A collider can only exist on a single layer, defined by the index.
    //! There is a maximum of 64 layers.
    class CollisionLayer
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(CollisionLayer, "{5AA459C8-2D92-46D2-9154-ED49EE4FE70E}");
        static void Reflect(AZ::ReflectContext* context);

        static const CollisionLayer Default; //!< Default collision layer, 0.

        //! Construct a layer with the given index.
        //! @param index The index of the layer. Must be between 0 - CollisionLayers::MaxCollisionLayers. Default CollisionLayer::Default.
        CollisionLayer(AZ::u8 index = Default.GetIndex());

        //! Construct a layer with the given name.
        //! This will lookup the layer name to retrieve the index. If not found will set the index to CollisionLayer::Default.
        //! @param layername The name of the layer.
        CollisionLayer(const AZStd::string& layerName);

        //! Get the index of this layer.
        //! Index will be between 0 - CollisionLayers::MaxCollisionLayers
        //! @return The layers index.
        AZ::u8 GetIndex() const;

        //! Set the index of this layer.
        //! @param index The index to set. Must be between 0 - CollisionLayers::MaxCollisionLayers
        void SetIndex(AZ::u8 index);

        //! Get the Layer index represented as a bitmask.
        //! @return A bitmask with the layer index bit toggled on.
        AZ::u64 GetMask() const;

        bool operator==(const CollisionLayer& other) const;
        bool operator!=(const CollisionLayer& other) const;
    private:
        AZ::u8 m_index;
    };

    //! Collision layers defined for the project.
    class CollisionLayers
    {
    public:
        static const AZ::u8 MaxCollisionLayers = 64;
        AZ_CLASS_ALLOCATOR_DECL;
        AZ_TYPE_INFO(CollisionLayers, "{68E7CB59-29BC-4825-AE99-182D6421EE65}");
        static void Reflect(AZ::ReflectContext* context);

        CollisionLayers() = default;

        //! Get the requested layer.
        //! @param name The name of the layer to retrieve.
        //! @return The request layer if found, otherwise CollisionLayer::Defualt.
        CollisionLayer GetLayer(const AZStd::string& name) const;

        //! Get the requested layer.
        //! @param name The name of the layer to retrieve.
        //! @param layer [OUT] The request layer if found, otherwise layer is left untouched.
        //! @return Returns true if the layer was found, otherwise false.
        bool TryGetLayer(const AZStd::string& name, CollisionLayer& layer) const;

        //! Get the name of the requested layer.
        //! @return Returns the name of the requested layer
        const AZStd::string& GetName(CollisionLayer layer) const;

        //! Get the names of all the layers.
        //! @return Returns an array of all the layers names.
        const AZStd::array<AZStd::string, MaxCollisionLayers>& GetNames() const;

        //! Set the name of the requested layer.
        //! @param layer The requested layer to modify.
        //! @param layerName The name of the layer.
        void SetName(CollisionLayer layer, const AZStd::string& layerName);

        //! Set the name of the requested layer by index.
        //! Will verify layerIndex is within bounds.
        //! @param layerIndex The requested layer index.
        //! @param layerName The name of the layer.
        void SetName(AZ::u64 layerIndex, const AZStd::string& layerName);

        bool operator==(const CollisionLayers& other) const;
        bool operator!=(const CollisionLayers& other) const;
    private:
        AZStd::array<AZStd::string, MaxCollisionLayers> m_names;
    };
}
