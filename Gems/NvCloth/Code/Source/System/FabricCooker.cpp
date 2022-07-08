/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Debug/Profiler.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/set.h>

#include <System/FabricCooker.h>

// NvCloth library includes
#include <NvCloth/Range.h>
#include <NvClothExt/ClothFabricCooker.h>

namespace NvCloth
{
    namespace Internal
    {
        FabricId ComputeFabricId(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            const AZ::Vector3& fabricGravity,
            bool useGeodesicTether)
        {
            AZ::Crc32 upperCrc32(particles.data(), sizeof(SimParticleFormat)*particles.size());
            upperCrc32.Add(&fabricGravity, sizeof(fabricGravity));

            AZ::Crc32 lowerCrc32(indices.data(), sizeof(SimIndexType)*indices.size());
            lowerCrc32.Add(&useGeodesicTether, sizeof(useGeodesicTether));

            const AZ::u32 upper = static_cast<AZ::u32>(upperCrc32);
            const AZ::u32 lower = static_cast<AZ::u32>(lowerCrc32);

            const AZ::u64 id =
                static_cast<AZ::u64>(lower) |
                (static_cast<AZ::u64>(upper) << 32);

            return FabricId(id);
        }

        nv::cloth::BoundedData ToNvBoundedData(const void* data, size_t stride, size_t count)
        {
            nv::cloth::BoundedData boundedData;
            boundedData.data = data;
            boundedData.stride = static_cast<physx::PxU32>(stride);
            boundedData.count = static_cast<physx::PxU32>(count);
            return boundedData;
        }

        template <typename T>
        static void CopyNvRange(const nv::cloth::Range<const T>& nvRange, AZStd::vector<T>& azVector)
        {
            azVector.resize(nvRange.size());
            AZStd::copy(nvRange.begin(), nvRange.end(), azVector.begin());
        }

        void CopyCookedData(FabricCookedData::InternalCookedData& azCookedData, const nv::cloth::CookedData& nvCookedData)
        {
            azCookedData.m_numParticles = nvCookedData.mNumParticles;
            // All these are fast copies
            CopyNvRange(nvCookedData.mPhaseIndices, azCookedData.m_phaseIndices);
            CopyNvRange(nvCookedData.mPhaseTypes, azCookedData.m_phaseTypes);
            CopyNvRange(nvCookedData.mSets, azCookedData.m_sets);
            CopyNvRange(nvCookedData.mRestvalues, azCookedData.m_restValues);
            CopyNvRange(nvCookedData.mStiffnessValues, azCookedData.m_stiffnessValues);
            CopyNvRange(nvCookedData.mIndices, azCookedData.m_indices);
            CopyNvRange(nvCookedData.mAnchors, azCookedData.m_anchors);
            CopyNvRange(nvCookedData.mTetherLengths, azCookedData.m_tetherLengths);
            CopyNvRange(nvCookedData.mTriangles, azCookedData.m_triangles);
        }

        AZStd::optional<FabricCookedData> Cook(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            const AZ::Vector3& fabricGravity,
            bool useGeodesicTether)
        {
            // Check if all the particles are static (inverse masses are all 0)
            const bool fullyStaticFabric = AZStd::all_of(particles.cbegin(), particles.cend(),
                [](const SimParticleFormat& particle)
                {
                    return particle.GetW() == 0.0f;
                });


            const int numIndicesPerTriangle = 3;
            const AZStd::vector<float> defaultInvMasses(particles.size(), 1.0f);

            nv::cloth::ClothMeshDesc meshDesc;
            meshDesc.setToDefault();
            meshDesc.points = ToNvBoundedData(particles.data(), sizeof(SimParticleFormat), particles.size());
            if (!fullyStaticFabric)
            {
                const int offsetToW = 3;
                meshDesc.invMasses = ToNvBoundedData(reinterpret_cast<const float*>(particles.data()) + offsetToW, sizeof(SimParticleFormat), particles.size());
            }
            else
            {
                // NvCloth doesn't support cooking a fabric where all its simulation particles are static (inverse masses are all 0.0).
                // In this situation we will cook the fabric with the default inverse masses (all 1.0). At runtime, inverse masses are
                // provided to the cloth when created, and they will override the fabric ones. NvCloth does support the cloth instance
                // to be fully static, but not the fabric.
                meshDesc.invMasses = ToNvBoundedData(defaultInvMasses.data(), sizeof(float), defaultInvMasses.size());
            }
            meshDesc.triangles = ToNvBoundedData(indices.data(), sizeof(SimIndexType) * numIndicesPerTriangle, indices.size() / numIndicesPerTriangle);
            meshDesc.flags = (sizeof(SimIndexType) == 2) ? nv::cloth::MeshFlag::e16_BIT_INDICES : 0;

            AZStd::unique_ptr<nv::cloth::ClothFabricCooker> cooker(NvClothCreateFabricCooker());
            if (!cooker ||
                !cooker->cook(meshDesc, *reinterpret_cast<const physx::PxVec3*>(&fabricGravity), useGeodesicTether))
            {
                return AZStd::nullopt;
            }

            FabricId fabricId = ComputeFabricId(particles, indices, fabricGravity, useGeodesicTether);
            if (!fabricId.IsValid())
            {
                return AZStd::nullopt;
            }

            FabricCookedData fabricData;
            fabricData.m_id = fabricId;
            fabricData.m_particles = particles;
            fabricData.m_indices = indices;
            fabricData.m_gravity = fabricGravity;
            fabricData.m_useGeodesicTether = useGeodesicTether;
            CopyCookedData(fabricData.m_internalData, cooker->getCookedData());

            return AZStd::optional<FabricCookedData>(AZStd::move(fabricData));
        }

        void WeldVertices(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<SimParticleFormat>& weldedParticles,
            AZStd::vector<SimIndexType>& weldedIndices,
            AZStd::vector<int>& remappedVertices,
            float weldingDistance = AZ::Constants::FloatEpsilon)
        {
            // Comparison functor for simulation particles based on the position.
            // Inverse mass is not involved in the comparison.
            struct ParticlesCompareLess
            {
                bool operator()(const SimParticleFormat& lhs, const SimParticleFormat& rhs) const
                {
                    if (!AZ::IsClose(lhs.GetX(), rhs.GetX(), m_weldingDistance))
                    {
                        return lhs.GetX() < rhs.GetX();
                    }
                    else if (!AZ::IsClose(lhs.GetY(), rhs.GetY(), m_weldingDistance))
                    {
                        return lhs.GetY() < rhs.GetY();
                    }
                    else if (!AZ::IsClose(lhs.GetZ(), rhs.GetZ(), m_weldingDistance))
                    {
                        return lhs.GetZ() < rhs.GetZ();
                    }
                    return false;
                }

                float m_weldingDistance = AZ::Constants::FloatEpsilon;
            };

            using ParticleToIndicesMap = AZStd::map<SimParticleFormat, AZStd::vector<size_t>, ParticlesCompareLess>;

            ParticleToIndicesMap particleToIndicesMap({ weldingDistance });
            for (size_t originalIndex = 0; originalIndex < particles.size(); ++originalIndex)
            {
                // To weld vertices with the same position we use a map where the key is the particle itself.
                // When inserting the particle to the map it will pick up the particle with the same position.
                auto insertedIt = particleToIndicesMap.insert({ particles[originalIndex], {} }).first;

                insertedIt->second.push_back(originalIndex);

                // Keep the minimum inverse mass value when welding particles.
                // It's OK to modify the W of the key element from the map because it's not involved in the comparison functor.
                insertedIt->first.SetW(
                    AZStd::min(
                        insertedIt->first.GetW(),
                        particles[originalIndex].GetW()));
            }

            // Compose welded particles and remapped vertices.
            int remappedIndex = 0;
            const int invalidIndex = -1;
            weldedParticles.resize_no_construct(particleToIndicesMap.size());
            remappedVertices.resize(particles.size(), invalidIndex);
            for (const auto& particleToIndicesPair : particleToIndicesMap)
            {
                weldedParticles[remappedIndex] = particleToIndicesPair.first;

                for (const size_t& originalIndex : particleToIndicesPair.second)
                {
                    remappedVertices[originalIndex] = remappedIndex;
                }

                ++remappedIndex;
            }

            // Compose welded indices.
            weldedIndices.resize_no_construct(indices.size());
            for (size_t i = 0; i < indices.size(); ++i)
            {
                const int remappedVertexIndex = remappedVertices[indices[i]];
                AZ_Assert(remappedVertexIndex >= 0, "Vertex Index %u has an invalid remapping", indices[i]);
                weldedIndices[i] = static_cast<SimIndexType>(remappedVertexIndex);
            }
        }

        void RemoveStaticTriangles(
            const AZStd::vector<SimParticleFormat>& particles,
            const AZStd::vector<SimIndexType>& indices,
            AZStd::vector<SimParticleFormat>& simplifiedParticles,
            AZStd::vector<SimIndexType>& simplifiedIndices,
            AZStd::vector<int>& remappedVertices)
        {
            using ParticleIndexSet = AZStd::set<size_t>;
            using TriangleIndices = AZStd::array<SimIndexType, 3>;

            ParticleIndexSet particleIndexSet;

            const size_t numTriangles = indices.size() / 3;
            size_t simplifiedNumTriangles = 0;

            auto isTriangleStatic = [&particles](const TriangleIndices& triangleIndices)
            {
                return (particles[triangleIndices[0]].GetW() == 0.0f)
                    && (particles[triangleIndices[1]].GetW() == 0.0f)
                    && (particles[triangleIndices[2]].GetW() == 0.0f);
            };

            // Collect all the vertices that belongs to non-static triangles
            for (size_t triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
            {
                const TriangleIndices triangleIndices =
                {{
                    indices[triangleIndex * 3 + 0],
                    indices[triangleIndex * 3 + 1],
                    indices[triangleIndex * 3 + 2]
                }};

                if (isTriangleStatic(triangleIndices))
                {
                    continue;
                }

                for (const auto& vertexIndex : triangleIndices)
                {
                    particleIndexSet.insert({ vertexIndex });
                }
                ++simplifiedNumTriangles;
            }

            // Compose simplified particles and remapped vertices.
            int remappedIndex = 0;
            const int invalidIndex = -1;
            simplifiedParticles.resize_no_construct(particleIndexSet.size());
            remappedVertices.resize(particles.size(), invalidIndex);
            for (const auto& particleIndex : particleIndexSet)
            {
                simplifiedParticles[remappedIndex] = particles[particleIndex];

                remappedVertices[particleIndex] = remappedIndex;

                ++remappedIndex;
            }

            // Compose simplified indices.
            size_t simplifiedIndex = 0;
            simplifiedIndices.resize_no_construct(simplifiedNumTriangles * 3);
            for (size_t triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
            {
                const TriangleIndices triangleIndices =
                {{
                    indices[triangleIndex * 3 + 0],
                    indices[triangleIndex * 3 + 1],
                    indices[triangleIndex * 3 + 2]
                }};

                if (isTriangleStatic(triangleIndices))
                {
                    continue;
                }

                for (const auto& vertexIndex : triangleIndices)
                {
                    const int remappedVertexIndex = remappedVertices[vertexIndex];
                    AZ_Assert(remappedVertexIndex >= 0, "Vertex Index %u has an invalid remapping", vertexIndex);
                    simplifiedIndices[simplifiedIndex++] = static_cast<SimIndexType>(remappedVertexIndex);
                }
            }
            AZ_Assert(simplifiedIndex == simplifiedIndices.size(),
                "Number of indices after removing static particles is %zu, but it's expected %zu.", simplifiedIndex, simplifiedIndices.size());
        }
    }

    AZStd::optional<FabricCookedData> FabricCooker::CookFabric(
        const AZStd::vector<SimParticleFormat>& particles,
        const AZStd::vector<SimIndexType>& indices,
        const AZ::Vector3& fabricGravity,
        bool useGeodesicTether)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        return Internal::Cook(particles, indices, fabricGravity, useGeodesicTether);
    }

    void FabricCooker::SimplifyMesh(
        const AZStd::vector<SimParticleFormat>& particles,
        const AZStd::vector<SimIndexType>& indices,
        AZStd::vector<SimParticleFormat>& simplifiedParticles,
        AZStd::vector<SimIndexType>& simplifiedIndices,
        AZStd::vector<int>& remappedVertices,
        bool removeStaticTriangles)
    {
        AZ_PROFILE_FUNCTION(Cloth);

        // Weld vertices together
        AZStd::vector<SimParticleFormat> weldedParticles;
        AZStd::vector<SimIndexType> weldedIndices;
        AZStd::vector<int> weldedRemappedVertices;
        Internal::WeldVertices(
            particles, indices,
            weldedParticles, weldedIndices,
            weldedRemappedVertices);

        if (!removeStaticTriangles)
        {
            simplifiedParticles = AZStd::move(weldedParticles);
            simplifiedIndices = AZStd::move(weldedIndices);
            remappedVertices = AZStd::move(weldedRemappedVertices);
            return;
        }

        // Remove static particles
        AZStd::vector<int> simplifiedRemappedVertices;
        Internal::RemoveStaticTriangles(
            weldedParticles, weldedIndices,
            simplifiedParticles, simplifiedIndices,
            simplifiedRemappedVertices);

        // Compose final remapped vertices
        remappedVertices.resize_no_construct(particles.size());
        for (size_t i = 0; i < particles.size(); ++i)
        {
            const int weldedRemappedIndex = weldedRemappedVertices[i];
            remappedVertices[i] = simplifiedRemappedVertices[weldedRemappedIndex];
        }
    }
} // namespace NvCloth
