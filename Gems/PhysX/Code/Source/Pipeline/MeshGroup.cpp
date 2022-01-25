/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/Rules/MaterialRule.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>

#include <Source/Pipeline/MeshGroup.h>
#include <Source/Pipeline/MeshExporter.h>
#include <Source/Material.h>

#include <PxPhysicsAPI.h>

namespace PhysX
{
    namespace Pipeline
    {
        TriangleMeshAssetParams::TriangleMeshAssetParams()
        {
            physx::PxCookingParams defaultCookingParams = physx::PxCookingParams(physx::PxTolerancesScale());
            const AZ::u32 defaultMeshPreprocessParams = static_cast<AZ::u32>(defaultCookingParams.meshPreprocessParams);

            m_mergeMeshes = true;
            m_weldVertices = (defaultMeshPreprocessParams & physx::PxMeshPreprocessingFlag::eWELD_VERTICES) != 0;
            m_disableCleanMesh = (defaultMeshPreprocessParams & physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH) != 0;
            m_force32BitIndices = (defaultMeshPreprocessParams & physx::PxMeshPreprocessingFlag::eFORCE_32BIT_INDICES) != 0;
            m_suppressTriangleMeshRemapTable = defaultCookingParams.suppressTriangleMeshRemapTable;
            m_buildTriangleAdjacencies = defaultCookingParams.buildTriangleAdjacencies;
            m_meshWeldTolerance = defaultCookingParams.meshWeldTolerance;

            defaultCookingParams.midphaseDesc.setToDefault(physx::PxMeshMidPhase::eBVH34);
            m_numTrisPerLeaf = defaultCookingParams.midphaseDesc.mBVH34Desc.numPrimsPerLeaf;
        }

        void TriangleMeshAssetParams::Reflect(AZ::ReflectContext* context)
        {
            if (
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                serializeContext
            )
            {
                serializeContext->Class<TriangleMeshAssetParams>()->Version(1)
                    ->Field("MergeMeshes", &TriangleMeshAssetParams::m_mergeMeshes)
                    ->Field("WeldVertices", &TriangleMeshAssetParams::m_weldVertices)
                    ->Field("DisableCleanMesh", &TriangleMeshAssetParams::m_disableCleanMesh)
                    ->Field("Force32BitIndices", &TriangleMeshAssetParams::m_force32BitIndices)
                    ->Field("SuppressTriangleMeshRemapTable", &TriangleMeshAssetParams::m_suppressTriangleMeshRemapTable)
                    ->Field("BuildTriangleAdjacencies", &TriangleMeshAssetParams::m_buildTriangleAdjacencies)
                    ->Field("MeshWeldTolerance", &TriangleMeshAssetParams::m_meshWeldTolerance)
                    ->Field("NumTrisPerLeaf", &TriangleMeshAssetParams::m_numTrisPerLeaf);

                if (
                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    editContext
                )
                {
                    editContext->Class<TriangleMeshAssetParams>("Triangle Mesh Asset Parameters",
                        "Configure the parameters controlling the exported triangle mesh asset.")

                        ->DataElement(AZ_CRC("MergeMeshes", 0x118c4a63), &TriangleMeshAssetParams::m_mergeMeshes, "Merge Meshes",
                            "<span>When set, all selected nodes will be merged into a single collision mesh. Otherwise "
                            "they will be exported as separate shapes. Typically it is more efficient to have a single "
                            "mesh, however if you have game code handling specific shapes differently, you want to "
                            "avoid merging them together.</span>")

                        ->DataElement(AZ_CRC("WeldVertices", 0xe4e0c33c), &TriangleMeshAssetParams::m_weldVertices, "Weld Vertices",
                            "<span>When set, mesh welding is performed. Clean mesh must be enabled.</span>")

                        ->DataElement(AZ_CRC("DisableCleanMesh", 0xc720ef8e), &TriangleMeshAssetParams::m_disableCleanMesh, "Disable Clean Mesh",
                            "<span>When set, mesh cleaning is disabled. This makes cooking faster. When clean mesh is "
                            "not performed, mesh welding is also not performed.</span>")

                        ->DataElement(AZ_CRC("Force32BitIndices", 0x640dfd70), &TriangleMeshAssetParams::m_force32BitIndices, "Force 32-Bit Indices",
                            "<span>When set, 32-bit indices will always be created regardless of triangle count."
                            "</span>")

                        ->DataElement(AZ_CRC("SuppressTriangleMeshRemapTable", 0x8b818a60), &TriangleMeshAssetParams::m_suppressTriangleMeshRemapTable, "Suppress Triangle Mesh Remap Table",
                            "<span>When true, the face remap table is not created. This saves a significant amount of "
                            "memory, but the SDK will not be able to provide the remap information for internal mesh "
                            "triangles returned by collisions, sweeps or raycasts hits.</span>")

                        ->DataElement(AZ_CRC("BuildTriangleAdjacencies", 0xbb5a9b49), &TriangleMeshAssetParams::m_buildTriangleAdjacencies, "Build Triangle Adjacencies",
                            "<span>When true, the triangle adjacency information is created. You can get the adjacency "
                            "triangles for a given triangle from getTriangle.</span>")

                        ->DataElement(AZ_CRC("MeshWeldTolerance", 0x37df452d), &TriangleMeshAssetParams::m_meshWeldTolerance, "Mesh Weld Tolerance",
                            "<span>Mesh weld tolerance. If mesh welding is enabled, this controls the distance at "
                            "which vertices are welded. If mesh welding is not enabled, this value defines the "
                            "acceptance distance for mesh validation. Provided no two vertices are within this "
                            "distance, the mesh is considered to be clean. If not, a warning will be emitted. Having a "
                            "clean, welded mesh is required to achieve the best possible performance. The default "
                            "vertex welding uses a snap-to-grid approach. This approach effectively truncates each "
                            "vertex to integer values using Mesh Weld Tolerance. Once these snapped vertices are "
                            "produced, all vertices that snap to a given vertex on the grid are remapped to reference "
                            "a single vertex. Following this, all triangles indices are remapped to reference this "
                            "subset of clean vertices. It should be noted that the vertices that we do not alter the "
                            "position of the vertices; the snap-to-grid is only performed to identify nearby vertices. "
                            "The mesh validation approach also uses the same snap-to-grid approach to identify nearby "
                            "vertices. If more than one vertex snaps to a given grid coordinate, we ensure that the "
                            "distance between the vertices is at least Mesh Weld Tolerance. If this is not the case, a "
                            "warning is emitted.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 3)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 3)

                        ->DataElement(AZ_CRC("NumTrisPerLeaf", 0x391bf6d1), &TriangleMeshAssetParams::m_numTrisPerLeaf, "Number of Triangles Per Leaf",
                            "<span>Mesh cooking hint for max triangles per leaf limit. Fewer triangles per leaf "
                            "produces larger meshes with better runtime performance and worse cooking performance. "
                            "More triangles per leaf results in faster cooking speed and smaller mesh sizes, but with "
                            "worse runtime performance.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 4)
                            ->Attribute(AZ::Edit::Attributes::Max, 15);
                }
            }
        }

        bool TriangleMeshAssetParams::GetMergeMeshes() const
        {
            return m_mergeMeshes;
        }

        void TriangleMeshAssetParams::SetMergeMeshes(bool mergeMeshes)
        {
            m_mergeMeshes = mergeMeshes;
        }

        bool TriangleMeshAssetParams::GetWeldVertices() const
        {
            return m_weldVertices;
        }

        void TriangleMeshAssetParams::SetWeldVertices(bool weldVertices)
        {
            m_weldVertices = weldVertices;
        }

        bool TriangleMeshAssetParams::GetDisableCleanMesh() const
        {
            return m_disableCleanMesh;
        }

        bool TriangleMeshAssetParams::GetForce32BitIndices() const
        {
            return m_force32BitIndices;
        }

        bool TriangleMeshAssetParams::GetSuppressTriangleMeshRemapTable() const
        {
            return m_suppressTriangleMeshRemapTable;
        }

        bool TriangleMeshAssetParams::GetBuildTriangleAdjacencies() const
        {
            return m_buildTriangleAdjacencies;
        }

        float TriangleMeshAssetParams::GetMeshWeldTolerance() const
        {
            return m_meshWeldTolerance;
        }

        void TriangleMeshAssetParams::SetMeshWeldTolerance(float weldTolerance)
        {
            m_meshWeldTolerance = weldTolerance;
        }

        AZ::u32 TriangleMeshAssetParams::GetNumTrisPerLeaf() const
        {
            return m_numTrisPerLeaf;
        }


        ConvexAssetParams::ConvexAssetParams()
        {
            const physx::PxCookingParams defaultCookingParams = physx::PxCookingParams(physx::PxTolerancesScale());
            const physx::PxConvexMeshDesc defaultconvexDesc;
            const AZ::u32 defaultConvexFlags = static_cast<AZ::u32>(defaultconvexDesc.flags);

            m_areaTestEpsilon = defaultCookingParams.areaTestEpsilon;
            m_planeTolerance = defaultCookingParams.planeTolerance;
            m_use16bitIndices = (defaultConvexFlags & physx::PxConvexFlag::e16_BIT_INDICES) != 0;
            m_checkZeroAreaTriangles = (defaultConvexFlags & physx::PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES) != 0;
            m_quantizeInput = (defaultConvexFlags & physx::PxConvexFlag::eQUANTIZE_INPUT) != 0;
            m_usePlaneShifting = (defaultConvexFlags & physx::PxConvexFlag::ePLANE_SHIFTING) != 0;
            m_shiftVertices = (defaultConvexFlags & physx::PxConvexFlag::eSHIFT_VERTICES) != 0;
            m_buildGpuData = false;
            m_gaussMapLimit = defaultCookingParams.gaussMapLimit;
        }

        void ConvexAssetParams::Reflect(AZ::ReflectContext* context)
        {
            if (
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                serializeContext
            )
            {
                serializeContext->Class<ConvexAssetParams>()->Version(1)
                    ->Field("AreaTestEpsilon", &ConvexAssetParams::m_areaTestEpsilon)
                    ->Field("PlaneTolerance", &ConvexAssetParams::m_planeTolerance)
                    ->Field("Use16bitIndices", &ConvexAssetParams::m_use16bitIndices)
                    ->Field("CheckZeroAreaTriangles", &ConvexAssetParams::m_checkZeroAreaTriangles)
                    ->Field("QuantizeInput", &ConvexAssetParams::m_quantizeInput)
                    ->Field("UsePlaneShifting", &ConvexAssetParams::m_usePlaneShifting)
                    ->Field("ShiftVertices", &ConvexAssetParams::m_shiftVertices)
                    ->Field("GaussMapLimit", &ConvexAssetParams::m_gaussMapLimit)
                    ->Field("BuildGpuData", &ConvexAssetParams::m_buildGpuData);

                if (
                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    editContext
                )
                {
                    editContext->Class<ConvexAssetParams>("Convex Asset Parameters",
                        "Configure the parameters controlling the exported convex asset.")
                        ->DataElement(AZ_CRC("AreaTestEpsilon", 0x3c6f6877), &ConvexAssetParams::m_areaTestEpsilon, "Area Test Epsilon",
                            "<span>If the area of a triangle of the hull is below this value, the triangle will be "
                            "rejected. This test is done only if Check Zero Area Triangles is used.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 3)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 3)

                        ->DataElement(AZ_CRC("PlaneTolerance", 0xa8640bac), &ConvexAssetParams::m_planeTolerance, "Plane Tolerance",
                            "<span>The value is used during hull construction. When a new point is about to be added "
                            "to the hull it gets dropped when the point is closer to the hull than the planeTolerance. "
                            "The Plane Tolerance is increased according to the hull size. If 0.0f is set all points "
                            "are accepted when the convex hull is created. This may lead to edge cases where the new "
                            "points may be merged into an existing polygon and the polygons plane equation might "
                            "slightly change therefore. This might lead to failures during polygon merging phase in "
                            "the hull computation. It is recommended to use the default value, however if it is "
                            "required that all points needs to be accepted or huge thin convexes are created, it might "
                            "be required to lower the default value.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 100.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)

                        ->DataElement(AZ_CRC("Use16bitIndices", 0xb81adbfa), &ConvexAssetParams::m_use16bitIndices, "Use 16-bit Indices",
                            "<span>Denotes the use of 16-bit vertex indices in Convex triangles or polygons. "
                            "Otherwise, 32-bit indices are assumed.</span>")

                        ->DataElement(AZ_CRC("CheckZeroAreaTriangles", 0xa8b649c4), &ConvexAssetParams::m_checkZeroAreaTriangles, "Check Zero Area Triangles",
                            "<span>Checks and removes almost zero-area triangles during convex hull computation. The "
                            "rejected area size is specified in Area Test Epsilon.</span>")

                        ->DataElement(AZ_CRC("QuantizeInput", 0xe64b9553), &ConvexAssetParams::m_quantizeInput, "Quantize Input",
                            "<span>Quantizes the input vertices using the k-means clustering.</span>")

                        ->DataElement(AZ_CRC("UsePlaneShifting", 0xa10bad2e), &ConvexAssetParams::m_usePlaneShifting, "Use Plane Shifting",
                            "<span>Enables plane shifting vertex limit algorithm. Plane shifting is an alternative "
                            "algorithm for the case when the computed hull has more vertices than the specified vertex "
                            "limit. The default algorithm computes the full hull, and an OBB around the input "
                            "vertices. This OBB is then sliced with the hull planes until the vertex limit is reached. "
                            "The default algorithm requires the vertex limit to be set to at least 8, and typically "
                            "produces results that are much better quality than are produced by plane shifting. When "
                            "plane shifting is enabled, the hull computation stops when vertex limit is reached. The "
                            "hull planes are then shifted to contain all input vertices, and the new plane "
                            "intersection points are then used to generate the final hull with the given vertex limit. "
                            "Plane shifting may produce sharp edges to vertices very far away from the input cloud, and"
                            "does not guarantee that all input vertices are inside the resulting hull. However, it can "
                            "be used with a vertex limit as low as 4.</span>")

                        ->DataElement(AZ_CRC("ShiftVertices", 0x580b6169), &ConvexAssetParams::m_shiftVertices, "Shift Vertices",
                            "<span>Convex hull input vertices are shifted to be around origin to provide better "
                            "computation stability. It is recommended to provide input vertices around the origin, "
                            "otherwise use this flag to improve numerical stability.</span>")

                        ->DataElement(AZ_CRC("GaussMapLimit", 0x409f655e), &ConvexAssetParams::m_gaussMapLimit, "Gauss Map Limit",
                            "<span>Vertex limit beyond which additional acceleration structures are computed for each "
                            "convex mesh. Increase that limit to reduce memory usage. Computing the extra structures "
                            "all the time does not guarantee optimal performance. There is a per-platform break - even "
                            "point below which the extra structures actually hurt performance.</span>")

                        ->DataElement(AZ_CRC("BuildGpuData", 0x0b7b0568), &ConvexAssetParams::m_buildGpuData, "Build GPU Data",
                            "<span>When true, additional information required for GPU-accelerated rigid body "
                            "simulation is created. This can increase memory usage and cooking times for convex meshes "
                            "and triangle meshes. Convex hulls are created with respect to GPU simulation limitations. "
                            "Vertex limit is set to 64 and vertex limit per face is internally set to 32.</span>");
                }
            }
        }

        float ConvexAssetParams::GetAreaTestEpsilon() const
        {
            return m_areaTestEpsilon;
        }

        float ConvexAssetParams::GetPlaneTolerance() const
        {
            return m_planeTolerance;
        }

        bool ConvexAssetParams::GetUse16bitIndices() const
        {
            return m_use16bitIndices;
        }

        bool ConvexAssetParams::GetCheckZeroAreaTriangles() const
        {
            return m_checkZeroAreaTriangles;
        }

        bool ConvexAssetParams::GetQuantizeInput() const
        {
            return m_quantizeInput;
        }

        bool ConvexAssetParams::GetUsePlaneShifting() const
        {
            return m_usePlaneShifting;
        }

        bool ConvexAssetParams::GetShiftVertices() const
        {
            return m_shiftVertices;
        }

        AZ::u32 ConvexAssetParams::GetGaussMapLimit() const
        {
            return m_gaussMapLimit;
        }

        bool ConvexAssetParams::GetBuildGpuData() const
        {
            return m_buildGpuData;
        }


        void PrimitiveAssetParams::Reflect(AZ::ReflectContext* context)
        {
            if (
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                serializeContext
            )
            {
                serializeContext->Class<PrimitiveAssetParams>()->Version(1)
                    ->Field("PrimitiveShapeTarget", &PrimitiveAssetParams::m_primitiveShapeTarget)
                    ->Field("VolumeTermCoefficient", &PrimitiveAssetParams::m_volumeTermCoefficient);

                if (
                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    editContext
                )
                {
                    editContext->Class<PrimitiveAssetParams>("Primitive Asset Parameters",
                        "Configure the parameters controlling the exported primitive asset.")

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &PrimitiveAssetParams::m_primitiveShapeTarget, "Target Shape",
                            "<span>The shape that should be fitted to this mesh. If \"Automatic\" is selected, the "
                            "algorithm will determine which of the shapes fits best.</span>")
                            ->EnumAttribute(PrimitiveShapeTarget::BestFit, "Automatic")
                            ->EnumAttribute(PrimitiveShapeTarget::Sphere, "Sphere")
                            ->EnumAttribute(PrimitiveShapeTarget::Box, "Box")
                            ->EnumAttribute(PrimitiveShapeTarget::Capsule, "Capsule")

                        ->DataElement(AZ_CRC("VolumeTermCoefficient", 0xf471b1e2), &PrimitiveAssetParams::m_volumeTermCoefficient, "Volume Term Coefficient",
                            "<span>This parameter controls how aggressively the primitive fitting algorithm will try "
                            "to minimize the volume of the fitted primitive. A value of 0 (no volume minimization) is "
                            "recommended for most meshes, especially those with moderate to high vertex counts. For "
                            "meshes that have very few vertices, or vertices that are distributed mainly along the "
                            "edges of the shape, the algorithm can sometimes fit sub-optimal primitives that touch the "
                            "edges of the mesh but not the faces. Such primitives can be further optimized by "
                            "increasing the value of this parameter so that the algorithm actively tries to shrink the "
                            "volume of the generated primitive in addition to minimizing its deviation from the mesh. "
                            "A value that is too high may cause the primitive collider shrink too much so that it is "
                            "completely occluded by the mesh.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 0.002f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.00002f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 7)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 7);
                }
            }
        }

        PrimitiveShapeTarget PrimitiveAssetParams::GetPrimitiveShapeTarget() const
        {
            return m_primitiveShapeTarget;
        }

        float PrimitiveAssetParams::GetVolumeTermCoefficient() const
        {
            return m_volumeTermCoefficient;
        }


        void ConvexDecompositionParams::Reflect(AZ::ReflectContext* context)
        {
            if (
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                serializeContext
            )
            {
                serializeContext->Class<ConvexDecompositionParams>()->Version(1)
                    ->Field("MaxConvexHulls", &ConvexDecompositionParams::m_maxConvexHulls)
                    ->Field("MaxNumVerticesPerConvexHull", &ConvexDecompositionParams::m_maxNumVerticesPerConvexHull)
                    ->Field("Concavity", &ConvexDecompositionParams::m_concavity)
                    ->Field("Resolution", &ConvexDecompositionParams::m_resolution)
                    ->Field("Mode", &ConvexDecompositionParams::m_mode)
                    ->Field("Alpha", &ConvexDecompositionParams::m_alpha)
                    ->Field("Beta", &ConvexDecompositionParams::m_beta)
                    ->Field("MinVolumePerConvexHull", &ConvexDecompositionParams::m_minVolumePerConvexHull)
                    ->Field("PlaneDownsampling", &ConvexDecompositionParams::m_planeDownsampling)
                    ->Field("ConvexHullDownsampling", &ConvexDecompositionParams::m_convexHullDownsampling)
                    ->Field("PCA", &ConvexDecompositionParams::m_pca)
                    ->Field("ProjectHullVertices", &ConvexDecompositionParams::m_projectHullVertices);

                if (
                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    editContext
                )
                {
                    editContext->Class<ConvexDecompositionParams>("Decomposition Parameters",
                        "Configure the parameters controlling the approximate convex decomposition algorithm.")

                        ->DataElement(AZ_CRC("MaxConvexHulls", 0x862ea924), &ConvexDecompositionParams::m_maxConvexHulls, "Maximum Hulls",
                            "<span>Controls the maximum number of hulls to generate.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, 1024)

                        ->DataElement(AZ_CRC("MaxNumVerticesPerConvexHull", 0x936f94bd), &ConvexDecompositionParams::m_maxNumVerticesPerConvexHull, "Maximum Vertices Per Hull",
                            "<span>Controls the maximum number of triangles per convex hull.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 4)
                            ->Attribute(AZ::Edit::Attributes::Max, 1024)

                        ->DataElement(AZ_CRC("Concavity", 0x104f75ec), &ConvexDecompositionParams::m_concavity, "Concavity",
                            "<span>Maximum concavity of each approximate convex hull.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)

                        ->DataElement(AZ_CRC("Resolution", 0xfdd30f8a), &ConvexDecompositionParams::m_resolution, "Resolution",
                            "<span>Maximum number of voxels generated during the voxelization stage.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 10000)
                            ->Attribute(AZ::Edit::Attributes::Max, 64000000)
                            ->Attribute(AZ::Edit::Attributes::Step, 10000)

                        ->ClassElement(AZ::Edit::ClassElements::Group, "Advanced")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, false)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &ConvexDecompositionParams::m_mode, "Mode",
                            "<span>Select voxel-based approximate convex decomposition or tetrahedron-based "
                            "approximate convex decomposition.</span>")
                            ->Attribute(AZ::Edit::Attributes::EnumValues, AZStd::vector<AZ::Edit::EnumConstant<AZ::u32>> {
                                AZ::Edit::EnumConstant<AZ::u32>(0, "Voxel-based"),
                                AZ::Edit::EnumConstant<AZ::u32>(1, "Tetrahedron-based")
                            })

                        ->DataElement(AZ_CRC("Alpha", 0xd0e0396a), &ConvexDecompositionParams::m_alpha, "Alpha",
                            "<span>Controls the bias toward clipping along symmetry planes.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)

                        ->DataElement(AZ_CRC("Beta", 0x8f910463), &ConvexDecompositionParams::m_beta, "Beta",
                            "<span>Controls the bias toward clipping along revolution axes.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 1.0f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 4)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 4)

                        ->DataElement(AZ_CRC("MinVolumePerConvexHull", 0x1902aa21), &ConvexDecompositionParams::m_minVolumePerConvexHull, "Minimum Volume Per Hull",
                            "<span>Controls the adaptive sampling of the generated convex hulls.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                            ->Attribute(AZ::Edit::Attributes::Max, 0.01f)
                            ->Attribute(AZ::Edit::Attributes::Step, 0.0001f)
                            ->Attribute(AZ::Edit::Attributes::Decimals, 6)
                            ->Attribute(AZ::Edit::Attributes::DisplayDecimals, 6)

                        ->DataElement(AZ_CRC("PlaneDownsampling", 0xa8d39a9f), &ConvexDecompositionParams::m_planeDownsampling, "Plane Downsampling",
                            "<span>Controls the granularity of the search for the \"best\" clipping plane.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, 16)

                        ->DataElement(AZ_CRC("ConvexHullDownsampling", 0xd79bae19), &ConvexDecompositionParams::m_convexHullDownsampling, "Hull Downsampling",
                            "<span>Controls the precision of the convex hull generation process during the clipping "
                            "plane selection stage.</span>")
                            ->Attribute(AZ::Edit::Attributes::Min, 1)
                            ->Attribute(AZ::Edit::Attributes::Max, 16)

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ConvexDecompositionParams::m_pca, "Enable PCA",
                            "<span>Enable or disable normalizing the mesh before applying the convex "
                            "decomposition.</span>")

                        ->DataElement(AZ::Edit::UIHandlers::CheckBox, &ConvexDecompositionParams::m_projectHullVertices, "Project Hull Vertices",
                            "<span>Project the output convex hull vertices onto the original source mesh to increase "
                            "the floating point accuracy of the results.</span>");
                }
            }
        }

        float ConvexDecompositionParams::GetConcavity() const
        {
            return m_concavity;
        }

        float ConvexDecompositionParams::GetAlpha() const
        {
            return m_alpha;
        }

        float ConvexDecompositionParams::GetBeta() const
        {
            return m_beta;
        }

        float ConvexDecompositionParams::GetMinVolumePerConvexHull() const
        {
            return m_minVolumePerConvexHull;
        }

        AZ::u32 ConvexDecompositionParams::GetResolution() const
        {
            return m_resolution;
        }

        AZ::u32 ConvexDecompositionParams::GetMaxNumVerticesPerConvexHull() const
        {
            return m_maxNumVerticesPerConvexHull;
        }

        AZ::u32 ConvexDecompositionParams::GetPlaneDownsampling() const
        {
            return m_planeDownsampling;
        }

        AZ::u32 ConvexDecompositionParams::GetConvexHullDownsampling() const
        {
            return m_convexHullDownsampling;
        }

        AZ::u32 ConvexDecompositionParams::GetMaxConvexHulls() const
        {
            return m_maxConvexHulls;
        }

        bool ConvexDecompositionParams::GetPca() const
        {
            return m_pca;
        }

        AZ::u32 ConvexDecompositionParams::GetMode() const
        {
            return m_mode;
        }

        bool ConvexDecompositionParams::GetProjectHullVertices() const
        {
            return m_projectHullVertices;
        }


        AZ_CLASS_ALLOCATOR_IMPL(MeshGroup, AZ::SystemAllocator, 0)

        MeshGroup::MeshGroup()
            : m_id(AZ::Uuid::CreateRandom())
            , m_materialLibraryChangedHandler(
                [this](const AZ::Data::AssetId& materialLibraryAssetId)
                {
                    OnMaterialLibraryChanged(materialLibraryAssetId);
                })
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                physicsSystem->RegisterOnMaterialLibraryChangedEventHandler(m_materialLibraryChangedHandler);
            }
        }

        MeshGroup::~MeshGroup()
        {
            m_materialLibraryChangedHandler.Disconnect();
        }

        void MeshGroup::Reflect(AZ::ReflectContext* context)
        {
            TriangleMeshAssetParams::Reflect(context);
            ConvexAssetParams::Reflect(context);
            PrimitiveAssetParams::Reflect(context);
            ConvexDecompositionParams::Reflect(context);

            if (
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                serializeContext
            )
            {
                serializeContext->Class<MeshGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()->Version(2, &MeshGroup::VersionConverter)
                    ->Field("id", &MeshGroup::m_id)
                    ->Field("name", &MeshGroup::m_name)
                    ->Field("NodeSelectionList", &MeshGroup::m_nodeSelectionList)
                    ->Field("export method", &MeshGroup::m_exportMethod)
                    ->Field("TriangleMeshAssetParams", &MeshGroup::m_triangleMeshAssetParams)
                    ->Field("ConvexAssetParams", &MeshGroup::m_convexAssetParams)
                    ->Field("PrimitiveAssetParams", &MeshGroup::m_primitiveAssetParams)
                    ->Field("DecomposeMeshes", &MeshGroup::m_decomposeMeshes)
                    ->Field("ConvexDecompositionParams", &MeshGroup::m_convexDecompositionParams)
                    ->Field("MaterialSlots", &MeshGroup::m_materialSlots)
                    ->Field("PhysicsMaterials", &MeshGroup::m_physicsMaterials)
                    ->Field("rules", &MeshGroup::m_rules);

                if (
                    AZ::EditContext* editContext = serializeContext->GetEditContext();
                    editContext
                )
                {
                    editContext->Class<MeshGroup>("PhysX Mesh group", "Configure PhysX mesh data exporting.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::NameLabelOverride, "")

                        ->DataElement(AZ_CRC("ManifestName", 0x5215b349), &MeshGroup::m_name, "Name PhysX Mesh",
                            "<span>Name for the group. This name will also be used as a part of the name for the "
                            "generated file.</span>")

                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshGroup::m_nodeSelectionList, "Select meshes",
                            "<span>Select the meshes to be included in the mesh group.</span>")
                            ->Attribute("FilterName", "meshes")
                            ->Attribute("FilterType", AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid())
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshGroup::OnNodeSelectionChanged)

                        ->DataElement(AZ::Edit::UIHandlers::ComboBox, &MeshGroup::m_exportMethod, "Export As",
                            "<span>The cooking method to be applied to this mesh group. For the asset to be usable as "
                            "a rigid body, select \"Convex\" or \"Primitive\".</span>")
                            ->EnumAttribute(MeshExportMethod::TriMesh, "Triangle Mesh")
                            ->EnumAttribute(MeshExportMethod::Convex, "Convex")
                            ->EnumAttribute(MeshExportMethod::Primitive, "Primitive")
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshGroup::OnExportMethodChanged)

                        ->DataElement(AZ_CRC("DecomposeMeshes", 0xe0e2ac1e), &MeshGroup::m_decomposeMeshes, "Decompose Meshes",
                            "<span>If enables, this option will apply the V-HACD algorithm to split each node "
                            "into approximately convex parts. Each part will individually be exported as a convex "
                            "collider using the parameters configured above.</span>")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetDecomposeMeshesVisibility)
                            ->Attribute(AZ::Edit::Attributes::ChangeNotify, &MeshGroup::OnDecomposeMeshesChanged)

                        ->DataElement(AZ_CRC("TriangleMeshAssetParams", 0x1a408def), &MeshGroup::m_triangleMeshAssetParams, "Triangle Mesh Asset Parameters",
                            "<span>Configure the parameters controlling the exported triangle mesh asset.</span>")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsTriMesh)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ_CRC("ConvexAssetParams", 0x296b516c), &MeshGroup::m_convexAssetParams, "Convex Asset Parameters",
                            "<span>Configure the parameters controlling the exported convex asset.</span>")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsConvex)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ_CRC("PrimitiveAssetParams", 0xa9a5caa9), &MeshGroup::m_primitiveAssetParams, "Primitive Asset Parameters",
                            "<span>Configure the parameters controlling the exported primitive asset.</span>")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetExportAsPrimitive)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ_CRC("ConvexDecompositionParams", 0xd31a158c), &MeshGroup::m_convexDecompositionParams, "Decomposition Parameters",
                            "<span>Configure the parameters controlling the approximate convex decomposition algorithm.</span>")
                            ->Attribute(AZ::Edit::Attributes::Visibility, &MeshGroup::GetDecomposeMeshes)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshGroup::m_physicsMaterials, "Physics Materials",
                            "<span>Configure which physics materials to use for each element.</span>")
                            ->Attribute(AZ::Edit::Attributes::IndexedChildNameLabelOverride, &MeshGroup::GetMaterialSlotLabel)
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::ContainerCanBeModified, false)
                            ->ElementAttribute(AZ::Edit::UIHandlers::Handler, AZ::Edit::UIHandlers::ComboBox)
                            ->ElementAttribute(AZ::Edit::Attributes::StringList, &MeshGroup::GetPhysicsMaterialNames)

                        ->DataElement(AZ::Edit::UIHandlers::Default, &MeshGroup::m_rules, "",
                            "Add or remove rules to fine-tune the export process.")
                            ->Attribute(AZ::Edit::Attributes::Visibility, AZ_CRC("PropertyVisibility_ShowChildrenOnly", 0xef428f20));
                }
            }
        }

        const AZStd::string& MeshGroup::GetName() const
        {
            return m_name;
        }

        void MeshGroup::SetName(const AZStd::string& name)
        {
            m_name = name;
        }

        void MeshGroup::SetName(AZStd::string&& name)
        {
            m_name = AZStd::move(name);
        }

        const AZ::Uuid& MeshGroup::GetId() const
        {
            return m_id;
        }

        void MeshGroup::OverrideId(const AZ::Uuid& id)
        {
            m_id = id;
        }

        bool MeshGroup::GetExportAsConvex() const
        {
            return m_exportMethod == MeshExportMethod::Convex;
        }

        bool MeshGroup::GetExportAsTriMesh() const
        {
            return m_exportMethod == MeshExportMethod::TriMesh;
        }

        bool MeshGroup::GetExportAsPrimitive() const
        {
            return m_exportMethod == MeshExportMethod::Primitive;
        }

        bool MeshGroup::GetDecomposeMeshes() const
        {
            return (GetExportAsConvex() || GetExportAsPrimitive()) && m_decomposeMeshes;
        }

        const AZStd::vector<AZStd::string>& MeshGroup::GetPhysicsMaterials() const
        {
            return m_physicsMaterials;
        }

        const AZStd::vector<AZStd::string>& MeshGroup::GetMaterialSlots() const
        {
            return m_materialSlots;
        }

        void MeshGroup::SetSceneGraph(const AZ::SceneAPI::Containers::SceneGraph* graph)
        {
            m_graph = graph;
        }

        void MeshGroup::UpdateMaterialSlots()
        {
            if (!m_graph)
            {
                return;
            }

            AZStd::optional<Utils::AssetMaterialsData> assetMaterialData = Utils::GatherMaterialsFromMeshGroup(*this, *m_graph);
            if (!assetMaterialData)
            {
                return;
            }

            Utils::UpdateAssetPhysicsMaterials(assetMaterialData->m_sourceSceneMaterialNames, m_materialSlots, m_physicsMaterials);
        }

        AZ::SceneAPI::Containers::RuleContainer& MeshGroup::GetRuleContainer()
        {
            return m_rules;
        }

        const AZ::SceneAPI::Containers::RuleContainer& MeshGroup::GetRuleContainerConst() const
        {
            return m_rules;
        }

        AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& MeshGroup::GetSceneNodeSelectionList()
        {
            return m_nodeSelectionList;
        }

        const AZ::SceneAPI::DataTypes::ISceneNodeSelectionList& MeshGroup::GetSceneNodeSelectionList() const
        {
            return m_nodeSelectionList;
        }

        TriangleMeshAssetParams& MeshGroup::GetTriangleMeshAssetParams()
        {
            return m_triangleMeshAssetParams;
        }

        const TriangleMeshAssetParams& MeshGroup::GetTriangleMeshAssetParams() const
        {
            return m_triangleMeshAssetParams;
        }

        ConvexAssetParams& MeshGroup::GetConvexAssetParams()
        {
            return m_convexAssetParams;
        }

        const ConvexAssetParams& MeshGroup::GetConvexAssetParams() const
        {
            return m_convexAssetParams;
        }

        PrimitiveAssetParams& MeshGroup::GetPrimitiveAssetParams()
        {
            return m_primitiveAssetParams;
        }

        const PrimitiveAssetParams& MeshGroup::GetPrimitiveAssetParams() const
        {
            return m_primitiveAssetParams;
        }

        ConvexDecompositionParams& MeshGroup::GetConvexDecompositionParams()
        {
            return m_convexDecompositionParams;
        }

        const ConvexDecompositionParams& MeshGroup::GetConvexDecompositionParams() const
        {
            return m_convexDecompositionParams;
        }

        AZ::u32 MeshGroup::OnNodeSelectionChanged()
        {
            UpdateMaterialSlots();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 MeshGroup::OnExportMethodChanged()
        {
            UpdateMaterialSlots();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        AZ::u32 MeshGroup::OnDecomposeMeshesChanged()
        {
            UpdateMaterialSlots();
            return AZ::Edit::PropertyRefreshLevels::EntireTree;
        }

        bool MeshGroup::GetDecomposeMeshesVisibility() const
        {
            return GetExportAsConvex() || GetExportAsPrimitive();
        }

        AZStd::string MeshGroup::GetMaterialSlotLabel(int index) const
        {
            if (index < m_materialSlots.size())
            {
                return m_materialSlots[index];
            }
            else
            {
                return "<Unknown>";
            }
        }

        AZStd::vector<AZStd::string> MeshGroup::GetPhysicsMaterialNames() const
        {
            if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
            {
                if (const auto* physicsConfiguration = physicsSystem->GetConfiguration();
                    physicsConfiguration && physicsConfiguration->m_materialLibraryAsset)
                {
                    const auto& materials = physicsConfiguration->m_materialLibraryAsset->GetMaterialsData();

                    AZStd::vector<AZStd::string> physicsMaterialNames;
                    physicsMaterialNames.reserve(materials.size() + 1);

                    physicsMaterialNames.emplace_back(Physics::DefaultPhysicsMaterialLabel);
                    for (const auto& material : materials)
                    {
                        physicsMaterialNames.emplace_back(material.m_configuration.m_surfaceType);
                    }

                    return physicsMaterialNames;
                }
            }
            return {Physics::DefaultPhysicsMaterialLabel};
        }

        void MeshGroup::OnMaterialLibraryChanged([[maybe_unused]] const AZ::Data::AssetId& materialLibraryAssetId)
        {
            AzToolsFramework::PropertyEditorGUIMessages::Bus::Broadcast(&AzToolsFramework::PropertyEditorGUIMessages::RequestRefresh,
                AzToolsFramework::PropertyModificationRefreshLevel::Refresh_AttributesAndValues);
        }

        bool MeshGroup::VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Remove the material rule.
            if (classElement.GetVersion() < 1)
            {
                if (
                    int ruleContainerNodeIndex = classElement.FindElement(AZ_CRC("rules", 0x899a993c));
                    ruleContainerNodeIndex >= 0
                )
                {
                    AZ::SerializeContext::DataElementNode& ruleContainerNode = classElement.GetSubElement(ruleContainerNodeIndex);
                    AZ::SceneAPI::Containers::RuleContainer ruleContainer;
                    if (ruleContainerNode.GetData<AZ::SceneAPI::Containers::RuleContainer>(ruleContainer))
                    {
                        auto materialRule = ruleContainer.FindFirstByType<AZ::SceneAPI::SceneData::MaterialRule>();
                        ruleContainer.RemoveRule(materialRule);
                    }

                    ruleContainerNode.SetData<AZ::SceneAPI::Containers::RuleContainer>(context, ruleContainer);
                }
            }

            // Move fields to sub-classes.
            if (classElement.GetVersion() < 2)
            {
                auto readAndRemoveElement = [&](const AZ::u32 crc, auto& dst) {
                    if (
                        int index = classElement.FindElement(crc);
                        index >= 0
                    )
                    {
                        classElement.GetSubElement(index).GetData(dst);
                        classElement.RemoveElement(index);
                    }
                };

                // Triangle mesh asset parameters.
                TriangleMeshAssetParams triangleMeshAssetParams;
                readAndRemoveElement(AZ_CRC("MergeMeshes", 0x118c4a63), triangleMeshAssetParams.m_mergeMeshes);
                readAndRemoveElement(AZ_CRC("WeldVertices", 0xe4e0c33c), triangleMeshAssetParams.m_weldVertices);
                readAndRemoveElement(AZ_CRC("DisableCleanMesh", 0xc720ef8e), triangleMeshAssetParams.m_disableCleanMesh);
                readAndRemoveElement(AZ_CRC("Force32BitIndices", 0x640dfd70), triangleMeshAssetParams.m_force32BitIndices);
                readAndRemoveElement(AZ_CRC("SuppressTriangleMeshRemapTable", 0x8b818a60), triangleMeshAssetParams.m_suppressTriangleMeshRemapTable);
                readAndRemoveElement(AZ_CRC("BuildTriangleAdjacencies", 0xbb5a9b49), triangleMeshAssetParams.m_buildTriangleAdjacencies);
                readAndRemoveElement(AZ_CRC("MeshWeldTolerance", 0x37df452d), triangleMeshAssetParams.m_meshWeldTolerance);
                readAndRemoveElement(AZ_CRC("NumTrisPerLeaf", 0x391bf6d1), triangleMeshAssetParams.m_numTrisPerLeaf);
                classElement.AddElementWithData(context, "TriangleMeshAssetParams", triangleMeshAssetParams);

                // Convex asset parameters.
                ConvexAssetParams convexAssetParams;
                readAndRemoveElement(AZ_CRC("AreaTestEpsilon", 0x3c6f6877), convexAssetParams.m_areaTestEpsilon);
                readAndRemoveElement(AZ_CRC("PlaneTolerance", 0xa8640bac), convexAssetParams.m_planeTolerance);
                readAndRemoveElement(AZ_CRC("Use16bitIndices", 0xb81adbfa), convexAssetParams.m_use16bitIndices);
                readAndRemoveElement(AZ_CRC("CheckZeroAreaTriangles", 0xa8b649c4), convexAssetParams.m_checkZeroAreaTriangles);
                readAndRemoveElement(AZ_CRC("QuantizeInput", 0xe64b9553), convexAssetParams.m_quantizeInput);
                readAndRemoveElement(AZ_CRC("UsePlaneShifting", 0xa10bad2e), convexAssetParams.m_usePlaneShifting);
                readAndRemoveElement(AZ_CRC("ShiftVertices", 0x580b6169), convexAssetParams.m_shiftVertices);
                readAndRemoveElement(AZ_CRC("GaussMapLimit", 0x409f655e), convexAssetParams.m_gaussMapLimit);
                readAndRemoveElement(AZ_CRC("BuildGpuData", 0x0b7b0568), convexAssetParams.m_buildGpuData);
                classElement.AddElementWithData(context, "ConvexAssetParams", convexAssetParams);

                // Convex asset parameters. The volume term coefficient must be converted explicitly.
                PrimitiveAssetParams primitiveAssetParams;
                readAndRemoveElement(AZ_CRC("PrimitiveShapeTarget", 0x3e142e71), primitiveAssetParams.m_primitiveShapeTarget);
                if (
                    int index = classElement.FindElement(AZ_CRC("VolumeTermCoefficient", 0xf471b1e2));
                    index >= 0
                )
                {
                    AZ::u32 oldValue = 0;
                    classElement.GetSubElement(index).GetData(oldValue);
                    classElement.RemoveElement(index);
                    primitiveAssetParams.m_volumeTermCoefficient = oldValue * 2.0e-5f;
                }
                classElement.AddElementWithData(context, "PrimitiveAssetParams", primitiveAssetParams);

                // Convert 'export as convex' to 'export method'
                // export as primitive was not previously available
                bool exportAsConvex;
                readAndRemoveElement(AZ::Crc32("export as convex"), exportAsConvex);
                classElement.AddElementWithData(context, "export method", exportAsConvex ? MeshExportMethod::Convex : MeshExportMethod::TriMesh);
            }

            return true;
        }
    }
}
