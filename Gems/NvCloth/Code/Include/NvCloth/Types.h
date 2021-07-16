/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/std/hash.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector4.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Vector2.h>

namespace NvCloth
{
    //! Format of particles used by the cloth system.
    //! x,y,z elements represent the position and w the inverse mass of the particle.
    //! An inverse mass value of 0 means the particle will be static (not affected by simulation).
    using SimParticleFormat = AZ::Vector4;

    //! Type for indices in the cloth system.
    using SimIndexType = AZ::u32;

    //! Type for UV coordinates in the cloth system.
    using SimUVType = AZ::Vector2;

    //! Wrapper class to provide type safe id.
    //! @note Make use of the template tag technique (phantom types)
    //!       to provide type safe variants of all ids.
    template<typename Tag>
    class GenericId
    {
    public:
        GenericId() = default;
        explicit GenericId(const AZ::u64 value)
            : m_value(value)
        {
        }

        bool IsValid() const
        {
            return m_value > 0;
        }

        AZ::u64 GetValue() const
        {
            return m_value;
        }

        bool operator==(const GenericId& id) const
        {
            return m_value == id.m_value;
        }

        bool operator!=(const GenericId& id) const
        {
            return !operator==(id);
        }

        bool operator<(const GenericId& id) const
        {
            return m_value < id.m_value;
        }

    private:
        AZ::u64 m_value = 0;
    };

    //! Identifies a cloth inside the system.
    using ClothId = GenericId<struct ClothIdTag>;

    //! Identifies a fabric inside the system.
    using FabricId = GenericId<struct FabricIdTag>;

    //! Name of the default solver that cloth system always creates.
    static const char* const DefaultSolverName = "DefaultClothSolver";

    //! Structure with all the data of a fabric.
    //!
    //! The fabric is a template from which cloths are created from, it contains all the necessary
    //! information (triangles, particles, movement constraints, etc.) to create a cloth.
    //! Use IFabricCooker interface in order to generate fabric cooked data.
    struct FabricCookedData
    {
        AZ_TYPE_INFO(FabricCookedData, "{3C92D56C-BFC1-40F0-AF26-9A872535C552}");

        //! Fabric unique identifier.
        FabricId m_id;

        //! List of particles (positions and inverse masses) used to cook the fabric.
        AZStd::vector<SimParticleFormat> m_particles;

        //! List of triangles' indices used to cook the fabric.
        AZStd::vector<SimIndexType> m_indices;

        //! Gravity value used to cook the fabric.
        AZ::Vector3 m_gravity;

        //! Whether geodesic distance was used to cook the fabric data.
        //! NvCloth will use vertex distance or geodesic distance (using triangle adjacencies)
        //! when calculating tether constraints.
        //! Using geodesic distance is more expensive during the cooking process, but it results
        //! in a more realistic cloth behavior when applying tether constraints.
        bool m_useGeodesicTether;

        //! Mirrored structure with processed data as nv::cloth::CookedData in NvCloth library.
        struct InternalCookedData
        {
            uint32_t m_numParticles;
            AZStd::vector<uint32_t> m_phaseIndices;
            AZStd::vector<int32_t> m_phaseTypes;
            AZStd::vector<uint32_t> m_sets;
            AZStd::vector<float> m_restValues;
            AZStd::vector<float> m_stiffnessValues;
            AZStd::vector<uint32_t> m_indices;
            AZStd::vector<uint32_t> m_anchors;
            AZStd::vector<float> m_tetherLengths;
            AZStd::vector<uint32_t> m_triangles;
        };
        InternalCookedData m_internalData;
    };
} // namespace NvCloth

namespace AZStd
{
    //! Enables NvCloth::GenericId<Tag> to be keys in hashed data structures.
    template<typename Tag>
    struct hash<NvCloth::GenericId<Tag>>
    {
        typedef NvCloth::GenericId<Tag> argument_type;
        typedef AZStd::size_t result_type;
        AZ_FORCE_INLINE size_t operator()(const NvCloth::GenericId<Tag>& id) const
        {
            AZStd::hash<AZ::u64> hasher;
            return hasher(id.GetValue());
        }
    };
} // namespace AZStd
