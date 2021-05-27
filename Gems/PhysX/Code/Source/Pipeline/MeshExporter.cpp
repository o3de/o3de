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

#include <PhysX_precompiled.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Containers/Utilities/SceneGraphUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <SceneAPI/SceneCore/Events/ExportProductList.h>
#include <SceneAPI/SceneCore/Utilities/FileUtilities.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>

#include <PhysX/MeshAsset.h>
#include <AzFramework/Physics/Material.h>
#include <Source/Pipeline/MeshAssetHandler.h>
#include <Source/Pipeline/MeshExporter.h>
#include <Source/Pipeline/PrimitiveShapeFitter/PrimitiveShapeFitter.h>
#include <Source/Pipeline/MeshGroup.h>
#include <Source/Utils.h>

#include <PxPhysicsAPI.h>
#include <VHACD.h>

#include <Cry_Math.h>
#include <MathConversion.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzCore/XML/rapidxml.h>
#include <AzCore/std/algorithm.h>
#include <GFxFramework/MaterialIO/Material.h>

// A utility macro helping set/clear bits in a single line
#define SET_BITS(flags, condition, bits) flags = (condition) ? ((flags) | (bits)) : ((flags) & ~(bits))

namespace PhysX
{
    namespace Pipeline
    {
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneUtil = AZ::SceneAPI::Utilities;

        static physx::PxDefaultAllocator pxDefaultAllocatorCallback;
        static const char* const DefaultMaterialName = "default";

        // Implementation of the PhysX error callback interface directing errors to ErrorWindow output.
        static class PxExportErrorCallback
            : public physx::PxErrorCallback
        {
        public:
            void reportError([[maybe_unused]] physx::PxErrorCode::Enum code, [[maybe_unused]] const char* message, [[maybe_unused]] const char* file, [[maybe_unused]] int line) override final
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "PxErrorCode %i: %s (line %i in %s)", code, message, line, file);
            }
        } pxDefaultErrorCallback;

        // A struct to store the asset-wide material names shared by multiple shapes
        struct AssetMaterialsData
        {
            // Material names coming from FBX, these will be Mesh Surfaces in the Collider Component
            AZStd::vector<AZStd::string> m_fbxMaterialNames;

            // Look-up table for fbxMaterialNames
            AZStd::unordered_map<AZStd::string, size_t> m_materialIndexByName;
        };

        // A struct to store the geometry data per FBX node
        struct NodeCollisionGeomExportData
        {
            AZStd::vector<Vec3> m_vertices;
            AZStd::vector<vtx_idx> m_indices;
            AZStd::vector<AZ::u16> m_perFaceMaterialIndices;
            AZStd::string m_nodeName;
        };

        // Implementation of the V-HACD log callback interface directing all messages to LogWindow output.
        static class VHACDLogCallback
            : public VHACD::IVHACD::IUserLogger
        {
            void Log([[maybe_unused]] const char* const msg) override final
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "V-HACD: %s", msg);
            }
        } vhacdDefaultLogCallback;

        // Scope-guarded interface to VHACD library. Uses lazy initialization and RAII to free resources upon deletion.
        class ScopedVHACD
        {
        public:
            ScopedVHACD()
                : vhacdPtr(nullptr)
            {
            }

            ~ScopedVHACD()
            {
                if (vhacdPtr)
                {
                    vhacdPtr->Clean();
                    vhacdPtr->Release();
                }
            }

            ScopedVHACD(ScopedVHACD const&) = delete;
            ScopedVHACD& operator=(ScopedVHACD const&) = delete;

            VHACD::IVHACD& operator*()
            {
                return *GetImplementation();
            }

            VHACD::IVHACD* operator->()
            {
                return GetImplementation();
            }

        private:
            VHACD::IVHACD* GetImplementation()
            {
                if (!vhacdPtr)
                {
                    vhacdPtr = VHACD::CreateVHACD();
                    AZ_Assert(vhacdPtr, "Failed to create VHACD instance.");
                }

                return vhacdPtr;
            }

            VHACD::IVHACD* vhacdPtr;
        };

        MeshExporter::MeshExporter()
        {
            BindToCall(&MeshExporter::ProcessContext);
        }

        void MeshExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<MeshExporter, AZ::SceneAPI::SceneCore::ExportingComponent>()->Version(3);
            }
        }

        namespace Utils
        {
            // Utility function doing look-up in fbxMaterialNames and inserting the name if it's not found
            AZ::u16 InsertMaterialIndexByName(const AZStd::string& materialName, AssetMaterialsData& materials)
            {
                AZStd::vector<AZStd::string>& fbxMaterialNames = materials.m_fbxMaterialNames;
                AZStd::unordered_map<AZStd::string, size_t>& materialIndexByName = materials.m_materialIndexByName;

                // Check if we have this material in the list
                auto materialIndexIter = materialIndexByName.find(materialName);

                if (materialIndexIter != materialIndexByName.end())
                {
                    return materialIndexIter->second;
                }

                // Add it to the list otherwise
                fbxMaterialNames.push_back(materialName);

                AZ::u16 newIndex = fbxMaterialNames.size() - 1;
                materialIndexByName[materialName] = newIndex;

                return newIndex;
            }

            // Building a map between FBX material name and the corresponding Cry surface type that is set in the .mtl file.
            void BuildMaterialToSurfaceTypeMap(const AZStd::string& materialFilename,
                AZStd::unordered_map<AZStd::string, AZStd::string>& materialToSurfaceTypeMap)
            {
                AZ::IO::SystemFile mtlFile;
                bool fileOpened = mtlFile.Open(materialFilename.c_str(), AZ::IO::SystemFile::SF_OPEN_READ_ONLY);
                if (fileOpened && mtlFile.Length() != 0)
                {
                    //Read material override file into a buffer
                    AZStd::vector<char> buffer(mtlFile.Length());
                    mtlFile.Read(mtlFile.Length(), buffer.data());
                    mtlFile.Close();

                    //Apparently in rapidxml if 'parse_no_data_nodes' isn't set it creates both value and data nodes
                    //with the data nodes having precedence such that updating values doesn't work.
                    AZ::rapidxml::xml_document<char>  document;
                    document.parse<AZ::rapidxml::parse_no_data_nodes>(buffer.data());

                    //Parse MTL file for materials and/or submaterials.
                    AZ::rapidxml::xml_node<char>* rootMaterialNode = document.first_node(AZ::GFxFramework::MaterialExport::g_materialString);

                    AZ::rapidxml::xml_node<char>* subMaterialNode = rootMaterialNode->first_node(AZ::GFxFramework::MaterialExport::g_subMaterialString);

                    if (subMaterialNode)
                    {
                        for (AZ::rapidxml::xml_node<char>* materialNode = subMaterialNode->first_node(AZ::GFxFramework::MaterialExport::g_materialString);
                            materialNode;
                            materialNode = materialNode->next_sibling(AZ::GFxFramework::MaterialExport::g_materialString))
                        {
                            AZ::rapidxml::xml_attribute<char>* nameAttribute = materialNode->first_attribute(AZ::GFxFramework::MaterialExport::g_nameString);
                            if (nameAttribute)
                            {
                                AZStd::string materialName = nameAttribute->value();
                                AZStd::string surfaceTypeName = DefaultMaterialName;

                                AZ::rapidxml::xml_attribute<char>* surfaceTypeNode = materialNode->first_attribute("SurfaceType");
                                if (surfaceTypeNode && surfaceTypeNode->value_size() != 0)
                                {
                                    surfaceTypeName = surfaceTypeNode->value();
                                }

                                materialToSurfaceTypeMap[materialName] = surfaceTypeName;
                            }
                            else
                            {
                                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "A SubMaterial without Name found in the .mtl file: %s", materialFilename.c_str());
                            }
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "No SubMaterial node in the .mtl file: %s", materialFilename.c_str());
                    }
                }
            }

            void UpdateAssetMaterialsFromCrySurfaceTypes(const AZStd::vector<AZStd::string>& fbxMaterialNames,
                const AZStd::unordered_map<AZStd::string, AZStd::string>& materialToSurfaceTypeMap,
                MeshAssetData& assetData)
            {
                AZStd::vector<AZStd::string>& materialNames = assetData.m_materialNames;
                AZ_Assert(materialNames.empty(),
                    "UpdateAssetMaterialsFromCrySurfaceTypes: Mesh Asset Data should not have materials already assigned.");

                materialNames.clear();
                materialNames.reserve(fbxMaterialNames.size());

                for (const AZStd::string& fbxMaterial : fbxMaterialNames)
                {
                    AZStd::string materialName;

                    // Here we assign the actual engine surface type based on the material name
                    auto materialToSurfaceIt = materialToSurfaceTypeMap.find(fbxMaterial);
                    if (materialToSurfaceIt != materialToSurfaceTypeMap.end()
                        && !materialToSurfaceIt->second.empty())
                    {
                        materialName = materialToSurfaceIt->second;

                        // Remove the mat_ prefix since the material library generated from surface types doesn't have it.
                        if (materialName.find("mat_") == 0)
                        {
                            materialName = materialName.substr(4);
                        }
                    }
                    else
                    {
                        materialName = DefaultMaterialName;
                    }

                    materialNames.emplace_back(AZStd::move(materialName));
                }

                // Asset mesh surfaces match FBX materials. These are the names that users see in the Collider Component in the Editor.
                assetData.m_surfaceNames = fbxMaterialNames;
            }

            bool ValidateCookedTriangleMesh(void* assetData, AZ::u32 assetDataSize)
            {
                physx::PxDefaultMemoryInputData inpStream(static_cast<physx::PxU8*>(assetData), assetDataSize);
                physx::PxTriangleMesh* triangleMesh = PxGetPhysics().createTriangleMesh(inpStream);

                bool success = triangleMesh != nullptr;
                triangleMesh->release();
                return success;
            }

            bool ValidateCookedConvexMesh(void* assetData, AZ::u32 assetDataSize)
            {
                physx::PxDefaultMemoryInputData inpStream(static_cast<physx::PxU8*>(assetData), assetDataSize);
                physx::PxConvexMesh* convexMesh = PxGetPhysics().createConvexMesh(inpStream);

                bool success = convexMesh != nullptr;
                convexMesh->release();
                return success;
            }

            AZStd::vector<AZStd::string> GenerateLocalNodeMaterialMap(const AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex)
            {
                AZStd::vector<AZStd::string> materialNames;

                auto view = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(
                    graph,
                    nodeIndex,
                    graph.GetContentStorage().begin(),
                    true
                );

                for (auto it = view.begin(), itEnd = view.end(); it != itEnd; ++it)
                {
                    if ((*it) && (*it)->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IMaterialData::TYPEINFO_Uuid()))
                    {
                        AZStd::string nodeName = graph.GetNodeName(graph.ConvertToNodeIndex(it.GetHierarchyIterator())).GetName();
                        materialNames.push_back(nodeName);
                    }
                }

                return materialNames;
            }
        }

        static physx::PxMeshMidPhase::Enum GetMidPhaseStructureType(const AZStd::string& platformIdentifier)
        {
            // Use by default 3.4 since 3.3 is being deprecated (despite being default)
            physx::PxMeshMidPhase::Enum ret = physx::PxMeshMidPhase::eBVH34;

            // Fallback to 3.3 on Android and iOS platforms since they don't support SSE2, which is required for 3.4
            if (platformIdentifier == "android" || platformIdentifier == "ios")
            {
                ret = physx::PxMeshMidPhase::eBVH33;
            }
            return ret;
        }

        // Checks that the entire mesh is assigned (at most) one material (required for convexes and primitives).
        static void RequireSingleFaceMaterial(const AZStd::vector<AZ::u16>& faceMaterials)
        {
            AZStd::unordered_set<AZ::u16> uniqueFaceMaterials(faceMaterials.begin(), faceMaterials.end());
            if (uniqueFaceMaterials.size() > 1)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::WarningWindow,
                    "Should only have 1 material assigned to a non-triangle mesh. Assigned: %d", uniqueFaceMaterials.size());
            }
        }

        // Cooks the geometry provided into a memory buffer based on the rules set in MeshGroup
        bool CookPhysXMesh(
            const AZStd::vector<Vec3>& vertices,
            const AZStd::vector<AZ::u32>& indices,
            const AZStd::vector<AZ::u16>& faceMaterials,
            AZStd::vector<AZ::u8>* output,
            const MeshGroup& meshGroup,
            const AZStd::string& platformIdentifier)
        {
            bool cookingSuccessful = false;
            AZStd::string cookingResultErrorCodeString;
            const ConvexAssetParams& convexAssetParams = meshGroup.GetConvexAssetParams();
            const TriangleMeshAssetParams& triangleMeshAssetParams = meshGroup.GetTriangleMeshAssetParams();
            bool shouldExportAsConvex = meshGroup.GetExportAsConvex();

            physx::PxCookingParams pxCookingParams = physx::PxCookingParams(physx::PxTolerancesScale());

            pxCookingParams.buildGPUData = false;
            pxCookingParams.midphaseDesc.setToDefault(GetMidPhaseStructureType(platformIdentifier));

            if (shouldExportAsConvex)
            {
                if (convexAssetParams.GetCheckZeroAreaTriangles())
                {
                    pxCookingParams.areaTestEpsilon = convexAssetParams.GetAreaTestEpsilon();
                }

                pxCookingParams.planeTolerance = convexAssetParams.GetPlaneTolerance();
                pxCookingParams.gaussMapLimit = convexAssetParams.GetGaussMapLimit();
            }
            else
            {
                pxCookingParams.midphaseDesc.mBVH34Desc.numPrimsPerLeaf = triangleMeshAssetParams.GetNumTrisPerLeaf();
                pxCookingParams.meshWeldTolerance = triangleMeshAssetParams.GetMeshWeldTolerance();
                pxCookingParams.buildTriangleAdjacencies = triangleMeshAssetParams.GetBuildTriangleAdjacencies();
                pxCookingParams.suppressTriangleMeshRemapTable = triangleMeshAssetParams.GetSuppressTriangleMeshRemapTable();

                if (triangleMeshAssetParams.GetWeldVertices())
                {
                    pxCookingParams.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eWELD_VERTICES;
                }

                if (triangleMeshAssetParams.GetDisableCleanMesh())
                {
                    pxCookingParams.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eDISABLE_CLEAN_MESH;
                }

                if (triangleMeshAssetParams.GetForce32BitIndices())
                {
                    pxCookingParams.meshPreprocessParams |= physx::PxMeshPreprocessingFlag::eFORCE_32BIT_INDICES;
                }
            }

            physx::PxCooking* pxCooking = PxCreateCooking(PX_PHYSICS_VERSION, PxGetFoundation(), pxCookingParams);
            AZ_Assert(pxCooking, "Failed to create PxCooking");

            physx::PxBoundedData strideData;
            strideData.count = vertices.size();
            strideData.stride = sizeof(Vec3);
            strideData.data = vertices.data();

            physx::PxDefaultMemoryOutputStream cookedMeshData;

            if (shouldExportAsConvex)
            {
                physx::PxConvexMeshDesc convexDesc;
                convexDesc.points = strideData;
                convexDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

                SET_BITS(convexDesc.flags, convexAssetParams.GetUse16bitIndices(), physx::PxConvexFlag::e16_BIT_INDICES);
                SET_BITS(convexDesc.flags, convexAssetParams.GetCheckZeroAreaTriangles(), physx::PxConvexFlag::eCHECK_ZERO_AREA_TRIANGLES);
                SET_BITS(convexDesc.flags, convexAssetParams.GetQuantizeInput(), physx::PxConvexFlag::eQUANTIZE_INPUT);
                SET_BITS(convexDesc.flags, convexAssetParams.GetUsePlaneShifting(), physx::PxConvexFlag::ePLANE_SHIFTING);
                SET_BITS(convexDesc.flags, convexAssetParams.GetBuildGpuData(), physx::PxConvexFlag::eGPU_COMPATIBLE);
                SET_BITS(convexDesc.flags, convexAssetParams.GetShiftVertices(), physx::PxConvexFlag::eSHIFT_VERTICES);

                physx::PxConvexMeshCookingResult::Enum convexCookingResultCode = physx::PxConvexMeshCookingResult::eSUCCESS;

                cookingSuccessful =
                    pxCooking->cookConvexMesh(convexDesc, cookedMeshData, &convexCookingResultCode)
                    && Utils::ValidateCookedConvexMesh(cookedMeshData.getData(), cookedMeshData.getSize());

                cookingResultErrorCodeString = PhysX::Utils::ConvexCookingResultToString(convexCookingResultCode);

                // Check how many unique materials are assigned onto the convex mesh.
                // Report it to the user if there's more than 1 since PhysX only supports a single material assigned to a convex
                RequireSingleFaceMaterial(faceMaterials);
            }
            else
            {
                physx::PxTriangleMeshDesc meshDesc;
                meshDesc.points = strideData;

                meshDesc.triangles.count = indices.size() / 3;
                meshDesc.triangles.stride = sizeof(AZ::u32) * 3;
                meshDesc.triangles.data = indices.data();

                meshDesc.materialIndices.stride = sizeof(AZ::u16);
                meshDesc.materialIndices.data = faceMaterials.data();

                physx::PxTriangleMeshCookingResult::Enum trimeshCookingResultCode = physx::PxTriangleMeshCookingResult::eSUCCESS;

                cookingSuccessful =
                    pxCooking->cookTriangleMesh(meshDesc, cookedMeshData, &trimeshCookingResultCode)
                    && Utils::ValidateCookedTriangleMesh(cookedMeshData.getData(), cookedMeshData.getSize());

                cookingResultErrorCodeString = PhysX::Utils::TriMeshCookingResultToString(trimeshCookingResultCode);
            }

            if (cookingSuccessful)
            {
                output->insert(output->end(), cookedMeshData.getData(), cookedMeshData.getData() + cookedMeshData.getSize());
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Cooking Mesh failed: %s", cookingResultErrorCodeString.c_str());
            }

            pxCooking->release();
            return cookingSuccessful;
        }

        // Utility function finding out the .mtl file for a given FBX (at the moment it's the same name as FBX but with .mtl extension)
        static AZStd::string GetAssetMaterialFilename(const AZ::SceneAPI::Events::ExportEventContext& context)
        {
            const AZ::SceneAPI::Containers::Scene& scene = context.GetScene();

            AZStd::string materialFilename = scene.GetSourceFilename();
            AzFramework::StringFunc::Path::ReplaceExtension(materialFilename, ".mtl");
            return materialFilename;
        }

        // Processes the collected data and writes into a file
        static AZ::SceneAPI::Events::ProcessingResult WritePxMeshAsset(
            AZ::SceneAPI::Events::ExportEventContext& context,
            const AZStd::vector<NodeCollisionGeomExportData>& totalExportData,
            const AssetMaterialsData &assetMaterialsData,
            const MeshGroup& meshGroup)
        {
            SceneEvents::ProcessingResult result = SceneEvents::ProcessingResult::Ignored;

            AZStd::string assetName = meshGroup.GetName();
            AZStd::string filename = SceneUtil::FileUtilities::CreateOutputFileName(assetName, context.GetOutputDirectory(), MeshAssetHandler::s_assetFileExtension);

            MeshAssetData assetData;

            const AZStd::string& materialFilename = GetAssetMaterialFilename(context);

            // Read the information about surface type for each material from the .mtl file
            AZStd::unordered_map<AZStd::string, AZStd::string> fbxMaterialToCrySurfaceTypeMap;
            Utils::BuildMaterialToSurfaceTypeMap(materialFilename, fbxMaterialToCrySurfaceTypeMap);

            // Assign the materials into cooked data
            Utils::UpdateAssetMaterialsFromCrySurfaceTypes(assetMaterialsData.m_fbxMaterialNames, fbxMaterialToCrySurfaceTypeMap, assetData);

            for (const NodeCollisionGeomExportData& subMesh : totalExportData)
            {
                MeshAssetData::ShapeConfigurationPair shape;

                if (meshGroup.GetExportAsPrimitive())
                {
                    // Only one material can be assigned to a primitive collider, so report a warning if the mesh has
                    // multiple materials assigned to it.
                    RequireSingleFaceMaterial(subMesh.m_perFaceMaterialIndices);

                    const PrimitiveAssetParams& primitiveAssetParams = meshGroup.GetPrimitiveAssetParams();

                    shape = FitPrimitiveShape(
                        subMesh.m_nodeName,
                        subMesh.m_vertices,
                        primitiveAssetParams.GetVolumeTermCoefficient(),
                        primitiveAssetParams.GetPrimitiveShapeTarget()
                    );
                }
                else
                {
                    // Cook the mesh into a binary buffer.
                    AZStd::vector<AZ::u8> physxData;
                    bool success = CookPhysXMesh(subMesh.m_vertices, subMesh.m_indices, subMesh.m_perFaceMaterialIndices,
                        &(physxData), meshGroup, context.GetPlatformIdentifier());

                    if (success)
                    {
                        AZStd::shared_ptr<Physics::CookedMeshShapeConfiguration> shapeConfig =
                            AZStd::make_shared<Physics::CookedMeshShapeConfiguration>();

                        shapeConfig->SetCookedMeshData(
                            physxData.data(),
                            physxData.size(),
                            meshGroup.GetExportAsConvex() ? Physics::CookedMeshShapeConfiguration::MeshType::Convex
                                                          : Physics::CookedMeshShapeConfiguration::MeshType::TriangleMesh
                        );

                        shape.second = shapeConfig;
                    }
                    else
                    {
                        AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Mesh cooking terminated unsuccessfully.");
                    }
                }

                if (shape.second)
                {
                    assetData.m_colliderShapes.push_back(shape);
                }
                else
                {
                    AZ_TracePrintf(
                        AZ::SceneAPI::Utilities::ErrorWindow,
                        "WritePxMeshAsset: Failed to create asset. Node: %s",
                        subMesh.m_nodeName.c_str()
                    );

                    return SceneEvents::ProcessingResult::Failure;
                }

                if (meshGroup.GetExportAsTriMesh())
                {
                    assetData.m_materialIndexPerShape.push_back(MeshAssetData::TriangleMeshMaterialIndex);
                }
                else
                {
                    AZ_Assert(
                        !subMesh.m_perFaceMaterialIndices.empty(),
                        "WritePxMeshAsset: m_perFaceMaterialIndices must be not empty! Please make sure you have a material assigned to the geometry. Node: %s",
                        subMesh.m_nodeName.c_str()
                    );
                    AZ_Assert(
                        subMesh.m_perFaceMaterialIndices[0] != MeshAssetData::TriangleMeshMaterialIndex,
                        "WritePxMeshAsset: m_perFaceMaterialIndices has invalid material index! Node: %s",
                        subMesh.m_nodeName.c_str()
                    );

                    assetData.m_materialIndexPerShape.push_back(subMesh.m_perFaceMaterialIndices[0]);
                }
            }

            if (PhysX::Utils::WriteCookedMeshToFile(filename, assetData))
            {
                AZStd::string productUuidString = meshGroup.GetId().ToString<AZStd::string>();
                AZ::Uuid productUuid = AZ::Uuid::CreateData(productUuidString.data(), productUuidString.size() * sizeof(productUuidString[0]));

                context.GetProductList().AddProduct(AZStd::move(filename), productUuid, AZ::AzTypeInfo<MeshAsset>::Uuid(), AZStd::nullopt, AZStd::nullopt);
                result = SceneEvents::ProcessingResult::Success;
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Unable to write to a file for a PhysX mesh asset. AssetName: %s, filename: %s", assetName.c_str(), filename.c_str());
                result = SceneEvents::ProcessingResult::Failure;
            }

            return result;
        }

        void DecomposeAndAppendMeshes(
            ScopedVHACD& decomposer,
            VHACD::IVHACD::Parameters vhacdParams,
            AZStd::vector<NodeCollisionGeomExportData>& totalExportData,
            const NodeCollisionGeomExportData& nodeExportData
        )
        {
            RequireSingleFaceMaterial(nodeExportData.m_perFaceMaterialIndices);
            AZ_Assert(
                !nodeExportData.m_perFaceMaterialIndices.empty(),
                "DecomposeAndAppendMeshes: Empty per-face material vector. Node: %s",
                nodeExportData.m_nodeName.c_str()
            );

            decomposer->Clean();

            // Convert the vertices to a float array suitable for passing to V-HACD.
            AZStd::vector<float> vhacdVertices;
            vhacdVertices.reserve(nodeExportData.m_vertices.size() * 3);
            for (const Vec3& vertex : nodeExportData.m_vertices)
            {
                vhacdVertices.emplace_back(vertex[0]);
                vhacdVertices.emplace_back(vertex[1]);
                vhacdVertices.emplace_back(vertex[2]);
            }

            if constexpr (AZStd::is_same<decltype(nodeExportData.m_indices)::value_type, uint32_t>::value)
            {
                decomposer->Compute(
                    vhacdVertices.data(),
                    vhacdVertices.size() / 3,
                    nodeExportData.m_indices.data(),
                    nodeExportData.m_indices.size() / 3,
                    vhacdParams
                );
            }
            else
            {
                // Convert the indices to an unsigned 32-bit integer array suitable for passing to V-HACD.
                AZStd::vector<uint32_t> vhacdIndices;
                vhacdIndices.reserve(nodeExportData.m_indices.size());
                for (auto index : nodeExportData.m_indices)
                {
                    vhacdIndices.emplace_back(index);
                }

                decomposer->Compute(
                    vhacdVertices.data(),
                    vhacdVertices.size() / 3,
                    vhacdIndices.data(),
                    vhacdIndices.size() / 3,
                    vhacdParams
                );
            }

            const AZ::u32 numberOfHulls = decomposer->GetNConvexHulls();

            AZ_Assert(numberOfHulls > 0, "V-HACD returned no convex hulls.");
            AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Convex decomposition returned %d hulls", numberOfHulls);

            for (AZ::u32 hullCounter = 0; hullCounter < numberOfHulls; ++hullCounter)
            {
                VHACD::IVHACD::ConvexHull convexHull;
                decomposer->GetConvexHull(hullCounter, convexHull);

                NodeCollisionGeomExportData nodeExportDataConvexPart;

                // Copy vertices.
                nodeExportDataConvexPart.m_vertices.reserve(convexHull.m_nPoints);
                for (AZ::u32 vertexCounter = 0; vertexCounter < convexHull.m_nPoints; ++vertexCounter)
                {
                    double* vertex = convexHull.m_points + 3 * vertexCounter;
                    nodeExportDataConvexPart.m_vertices.emplace_back(Vec3(
                        static_cast<float>(vertex[0]),
                        static_cast<float>(vertex[1]),
                        static_cast<float>(vertex[2])
                    ));
                }

                // Copy indices.
                nodeExportDataConvexPart.m_indices.reserve(convexHull.m_nTriangles * 3);
                for (AZ::u32 indexCounter = 0; indexCounter < convexHull.m_nTriangles * 3; ++indexCounter)
                {
                    nodeExportDataConvexPart.m_indices.emplace_back(convexHull.m_triangles[indexCounter]);
                }

                // Set up single per-face material.
                nodeExportDataConvexPart.m_perFaceMaterialIndices = AZStd::vector<AZ::u16>(
                    convexHull.m_nTriangles, nodeExportData.m_perFaceMaterialIndices[0]
                );

                nodeExportDataConvexPart.m_nodeName = nodeExportData.m_nodeName + "_" + AZStd::to_string(hullCounter);
                totalExportData.emplace_back(AZStd::move(nodeExportDataConvexPart));
            }
        }

        SceneEvents::ProcessingResult MeshExporter::ProcessContext(SceneEvents::ExportEventContext& context) const
        {
            AZ_TraceContext("Exporter", "PhysX");

            SceneEvents::ProcessingResultCombiner result;

            const AZ::SceneAPI::Containers::Scene& scene = context.GetScene();
            const AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();

            const SceneContainers::SceneManifest& manifest = context.GetScene().GetManifest();

            SceneContainers::SceneManifest::ValueStorageConstData valueStorage = manifest.GetValueStorage();
            auto view = SceneContainers::MakeExactFilterView<MeshGroup>(valueStorage);

            ScopedVHACD decomposer;

            for (const MeshGroup& pxMeshGroup : view)
            {
                // Export data per node
                AZStd::vector<NodeCollisionGeomExportData> totalExportData;
                AssetMaterialsData assetMaterialData;

                const AZStd::string& groupName = pxMeshGroup.GetName();

                AZ_TraceContext("Group Name", groupName);

                const auto& sceneNodeSelectionList = pxMeshGroup.GetSceneNodeSelectionList();

                size_t selectedNodeCount = sceneNodeSelectionList.GetSelectedNodeCount();

                // Setup VHACD parameters if required.
                VHACD::IVHACD::Parameters vhacdParams;
                if (pxMeshGroup.GetDecomposeMeshes())
                {
                    const ConvexDecompositionParams& convexDecompositionParams = pxMeshGroup.GetConvexDecompositionParams();

                    vhacdParams.m_callback = nullptr;
                    vhacdParams.m_logger = &vhacdDefaultLogCallback;
                    vhacdParams.m_concavity = convexDecompositionParams.GetConcavity();
                    vhacdParams.m_alpha = convexDecompositionParams.GetAlpha();
                    vhacdParams.m_beta = convexDecompositionParams.GetBeta();
                    vhacdParams.m_minVolumePerCH = convexDecompositionParams.GetMinVolumePerConvexHull();
                    vhacdParams.m_resolution = convexDecompositionParams.GetResolution();
                    vhacdParams.m_maxNumVerticesPerCH = convexDecompositionParams.GetMaxNumVerticesPerConvexHull();
                    vhacdParams.m_planeDownsampling = convexDecompositionParams.GetPlaneDownsampling();
                    vhacdParams.m_convexhullDownsampling = convexDecompositionParams.GetConvexHullDownsampling();
                    vhacdParams.m_maxConvexHulls = convexDecompositionParams.GetMaxConvexHulls();
                    vhacdParams.m_pca = convexDecompositionParams.GetPca();
                    vhacdParams.m_mode = convexDecompositionParams.GetMode();
                    vhacdParams.m_projectHullVertices = convexDecompositionParams.GetProjectHullVertices();
                }
                else
                {
                    totalExportData.reserve(selectedNodeCount);
                }

                for (size_t index = 0; index < selectedNodeCount; index++)
                {
                    AZ::SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.Find(sceneNodeSelectionList.GetSelectedNode(index));
                    auto nodeMesh = azrtti_cast<const AZ::SceneAPI::DataTypes::IMeshData*>(*graph.ConvertToStorageIterator(nodeIndex));

                    if (!nodeMesh)
                    {
                        continue;
                    }

                    const AZ::SceneAPI::Containers::SceneGraph::Name& nodeName = graph.GetNodeName(nodeIndex);

                    const AZStd::vector<AZStd::string> localFbxMaterialsList = Utils::GenerateLocalNodeMaterialMap(graph, nodeIndex);
                    const AZ::SceneAPI::DataTypes::MatrixType worldTransform = SceneUtil::BuildWorldTransform(graph, nodeIndex);

                    NodeCollisionGeomExportData nodeExportData;
                    nodeExportData.m_nodeName = nodeName.GetName();

                    const AZ::u32 vertexCount = nodeMesh->GetVertexCount();
                    const AZ::u32 faceCount = nodeMesh->GetFaceCount();

                    nodeExportData.m_vertices.resize(vertexCount);

                    for (AZ::u32 vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex)
                    {
                        AZ::Vector3 pos = nodeMesh->GetPosition(vertexIndex);
                        pos = worldTransform * pos;
                        nodeExportData.m_vertices[vertexIndex] = AZVec3ToLYVec3(pos);
                    }

                    nodeExportData.m_indices.resize(faceCount * 3);
                    nodeExportData.m_perFaceMaterialIndices.resize(faceCount);

                    if (localFbxMaterialsList.empty())
                    {
                        AZ_TracePrintf(
                            AZ::SceneAPI::Utilities::WarningWindow,
                            "Node '%s' does not have any material assigned to it. Material '%s' will be used.",
                            nodeExportData.m_nodeName.c_str(), DefaultMaterialName
                        );
                    }

                    for (AZ::u32 faceIndex = 0; faceIndex < faceCount; ++faceIndex)
                    {
                        AZStd::string materialName = DefaultMaterialName;
                        if (!localFbxMaterialsList.empty())
                        {
                            int materialId = nodeMesh->GetFaceMaterialId(faceIndex);
                            if (materialId >= localFbxMaterialsList.size())
                            {
                                AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow,
                                    "materialId %d for face %d is out of bound for localFbxMaterialsList (size %d).",
                                    materialId, faceIndex, localFbxMaterialsList.size());

                                return SceneEvents::ProcessingResult::Failure;
                            }

                            materialName = localFbxMaterialsList[materialId];
                        }

                        const AZ::SceneAPI::DataTypes::IMeshData::Face& face = nodeMesh->GetFaceInfo(faceIndex);
                        nodeExportData.m_indices[faceIndex * 3] = face.vertexIndex[0];
                        nodeExportData.m_indices[faceIndex * 3 + 1] = face.vertexIndex[1];
                        nodeExportData.m_indices[faceIndex * 3 + 2] = face.vertexIndex[2];

                        AZ::u16 materialIndex = Utils::InsertMaterialIndexByName(materialName, assetMaterialData);
                        nodeExportData.m_perFaceMaterialIndices[faceIndex] = materialIndex;
                    }

                    if (pxMeshGroup.GetDecomposeMeshes())
                    {
                        DecomposeAndAppendMeshes(decomposer, vhacdParams, totalExportData, nodeExportData);
                    }
                    else
                    {
                        totalExportData.emplace_back(AZStd::move(nodeExportData));
                    }
                }

                // Merge triangle meshes if there's more than 1
                if (pxMeshGroup.GetExportAsTriMesh() && pxMeshGroup.GetTriangleMeshAssetParams().GetMergeMeshes() && totalExportData.size() > 1)
                {
                    NodeCollisionGeomExportData mergedData;
                    mergedData.m_nodeName = groupName;

                    AZStd::vector<Vec3>& mergedVertices = mergedData.m_vertices;
                    AZStd::vector<vtx_idx>& mergedIndices = mergedData.m_indices;
                    AZStd::vector<AZ::u16>& mergedPerFaceMaterials = mergedData.m_perFaceMaterialIndices;

                    // Here we add the geometry data for each node into a single merged one
                    // Vertices & materials can be added directly but indices need to be incremented
                    // by the amount of vertices already added in the last iteration
                    for (const NodeCollisionGeomExportData& exportData : totalExportData)
                    {
                        vtx_idx startingIndex = mergedVertices.size();

                        mergedVertices.insert(mergedVertices.end(), exportData.m_vertices.begin(), exportData.m_vertices.end());

                        mergedPerFaceMaterials.insert(mergedPerFaceMaterials.end(),
                            exportData.m_perFaceMaterialIndices.begin(), exportData.m_perFaceMaterialIndices.end());

                        mergedIndices.reserve(mergedIndices.size() + exportData.m_indices.size());

                        AZStd::transform(exportData.m_indices.begin(), exportData.m_indices.end(),
                            AZStd::back_inserter(mergedIndices), [startingIndex](vtx_idx index)
                        {
                            return index + startingIndex;
                        });
                    }

                    // Clear the data per node and use only the merged one
                    totalExportData.clear();
                    totalExportData.emplace_back(AZStd::move(mergedData));
                }

                if (!totalExportData.empty())
                {
                    result += WritePxMeshAsset(context, totalExportData, assetMaterialData, pxMeshGroup);
                }
            }

            return result.GetResult();
        }
    }
}
