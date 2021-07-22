/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Generation/Components/TangentGenerator/TangentGenerateComponent.h>
#include <Generation/Components/TangentGenerator/TangentGenerators/MikkTGenerator.h>
#include <Generation/Components/TangentGenerator/TangentGenerators/BlendShapeMikkTGenerator.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexTangentData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexBitangentData.h>

#include <SceneAPI/SceneData/Rules/TangentsRule.h>

#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/ConvertIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <SceneAPI/SceneCore/DataTypes/DataTypeUtilities.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneData/GraphData/BlendShapeData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexBitangentData.h>
#include <SceneAPI/SceneData/GraphData/MeshVertexTangentData.h>

#include <AzToolsFramework/Debug/TraceContext.h>

#include <AzCore/Math/Vector4.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Math/ToString.h>


namespace AZ::SceneGenerationComponents
{
    TangentGenerateComponent::TangentGenerateComponent()
    {
        BindToCall(&TangentGenerateComponent::GenerateTangentData);
    }


    void TangentGenerateComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<TangentGenerateComponent, AZ::SceneAPI::SceneCore::GenerationComponent>()->Version(1);
        }
    }

    const AZ::SceneAPI::SceneData::TangentsRule& TangentGenerateComponent::GetTangentSpaceRule(const AZ::SceneAPI::Containers::Scene& scene) const
    {
        for (const auto& object : scene.GetManifest().GetValueStorage())
        {
            if (object->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IGroup::TYPEINFO_Uuid()))
            {
                const AZ::SceneAPI::DataTypes::IGroup* group = azrtti_cast<const AZ::SceneAPI::DataTypes::IGroup*>(object.get());
                const AZ::SceneAPI::SceneData::TangentsRule* rule = group->GetRuleContainerConst().FindFirstByType<AZ::SceneAPI::SceneData::TangentsRule>().get();
                if (rule)
                {
                    return *rule;
                }
            }
        }

        return AZ::SceneAPI::SceneData::TangentsRule::GetDefault();
    }

    AZ::SceneAPI::Events::ProcessingResult TangentGenerateComponent::GenerateTangentData(TangentGenerateContext& context)
    {
        // Iterate over all graph content and filter out all meshes.
        AZ::SceneAPI::Containers::SceneGraph& graph = context.GetScene().GetGraph();
        AZ::SceneAPI::Containers::SceneGraph::ContentStorageData graphContent = graph.GetContentStorage();

        // Build a list of mesh data nodes.
        AZStd::vector<AZStd::pair<AZ::SceneAPI::DataTypes::IMeshData*, AZ::SceneAPI::Containers::SceneGraph::NodeIndex> > meshes;
        for (auto item = graphContent.begin(); item != graphContent.end(); ++item)
        {
            // Skip anything that isn't a mesh.
            if (!(*item) || !(*item)->RTTI_IsTypeOf(AZ::SceneAPI::DataTypes::IMeshData::TYPEINFO_Uuid()))
            {
                continue;
            }

            // Get the mesh data and node index and store them in the vector as a pair, so we can iterate over them later.
            auto* mesh = static_cast<AZ::SceneAPI::DataTypes::IMeshData*>(item->get());
            AZ::SceneAPI::Containers::SceneGraph::NodeIndex nodeIndex = graph.ConvertToNodeIndex(item);
            meshes.emplace_back(mesh, nodeIndex);
        }

        // Iterate over them. We had to build the array before as this method can insert new nodes, so using the iterator directly would fail.
        for (auto& [mesh, nodeIndex] : meshes)
        {
            // Generate tangents for the mesh (if this is desired or needed).
            if (!GenerateNormalsAndTangentsForMesh(context.GetScene(), nodeIndex, mesh))
            {
                return AZ::SceneAPI::Events::ProcessingResult::Failure;
            }

            // Now that we have the tangents and bitangents, calculate the tangent w values for the ones that we imported from the scene file, as they only have xyz.
            UpdateFbxTangentWValues(graph, nodeIndex, mesh);
        }

        return AZ::SceneAPI::Events::ProcessingResult::Success;
    }

    void TangentGenerateComponent::UpdateFbxTangentWValues(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, const AZ::SceneAPI::DataTypes::IMeshData* meshData)
    {
        // Iterate over all UV sets.
        AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData = FindUvData(graph, nodeIndex, 0);
        size_t uvSetIndex = 0;
        while (uvData)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexTangentData* fbxTangentData = FindTangentData(graph, nodeIndex, uvSetIndex);
            AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* fbxBitangentData = FindBitangentData(graph, nodeIndex, uvSetIndex);

            if (fbxTangentData && fbxBitangentData)
            {
                const size_t numVerts = uvData->GetCount();
                AZ_Assert((numVerts == fbxTangentData->GetCount()) && (numVerts == fbxBitangentData->GetCount()), "Number of vertices inside UV set is not the same as number of tangents and bitangents.");
                for (size_t i = 0; i < numVerts; ++i)
                {
                    // This code calculates the best tangent.w value, which is either -1 or +1, depending on the bitangent being mirrored or not.
                    // We determine this by checking the angle between the generated tangent by doing a cross product between the tangent and normal, and the actual real bitangent.
                    // It is no guarantee that using "cross(normal, tangent.xyz)* tangent.w" will result in the right bitangent, as the basis might not be orthogonal.
                    // But we still go for the best guess.
                    AZ::Vector4 tangent = fbxTangentData->GetTangent(i);
                    AZ::Vector3 tangentDir = tangent.GetAsVector3();
                    tangentDir.NormalizeSafe();
                    AZ::Vector3 normal = meshData->GetNormal(static_cast<AZ::u32>(i));
                    normal.NormalizeSafe();
                    AZ::Vector3 generatedBitangent = normal.Cross(tangentDir);

                    float dot = fbxBitangentData->GetBitangent(i).Dot(generatedBitangent);
                    dot = AZ::GetMax(dot, -1.0f);
                    dot = AZ::GetMin(dot, 1.0f);
                    const float angle = acosf(dot);
                    if (angle > AZ::Constants::HalfPi)
                    {
                        tangent = fbxTangentData->GetTangent(i);
                        tangent.SetW(-1.0f);
                    }
                    else
                    {
                        tangent = fbxTangentData->GetTangent(i);
                        tangent.SetW(1.0f);
                    }
                    fbxTangentData->SetTangent(i, tangent);
                }
            }

            // Find the next UV set.
            uvData = FindUvData(graph, nodeIndex, ++uvSetIndex);
        }
    }

    void TangentGenerateComponent::FindBlendShapes(
        AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        AZStd::vector<AZ::SceneData::GraphData::BlendShapeData*>& outBlendShapes) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(
            graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneData::GraphData::BlendShapeData* blendShape =
                azrtti_cast<AZ::SceneData::GraphData::BlendShapeData*>(child->second.get());
            if (blendShape)
            {
                outBlendShapes.emplace_back(blendShape);
            }
        }
    }

    bool TangentGenerateComponent::GenerateNormalsAndTangentsForMesh(AZ::SceneAPI::Containers::Scene& scene, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::SceneAPI::DataTypes::IMeshData* meshData)
    {
        // Check what tangent spaces we need.
        const AZ::SceneAPI::SceneData::TangentsRule& tangentRule = GetTangentSpaceRule(scene);

        // Find all blend shape data under the mesh. We need to generate the tangent and bitangent for blend shape as well.
        AZStd::vector<AZ::SceneData::GraphData::BlendShapeData*> blendShapes;
        FindBlendShapes(scene.GetGraph(), nodeIndex, blendShapes);

        // Generate normals/tangents/bitangents for all uv sets.
        bool allSuccess = true;

        const size_t uvSetCount = CalcUvSetCount(scene.GetGraph(), nodeIndex);
        for (size_t uvSetIndex = 0; uvSetIndex < uvSetCount; ++uvSetIndex)
        {
            PerUvGenerateContext ctx{
                scene,
                nodeIndex,
                uvSetIndex,
                tangentRule,
                meshData,
                blendShapes
            };

            allSuccess &= ctx.GenerateNormalsForMeshUv();
            allSuccess &= ctx.GenerateTangentsForMeshUv();
        }

        return allSuccess;
    }

    bool TangentGenerateComponent::PerUvGenerateContext::GenerateTangentsForMeshUv() const
    {
        AZ::SceneAPI::Containers::SceneGraph& graph = scene.GetGraph();
        AZ::SceneAPI::DataTypes::IMeshVertexUVData* uvData = TangentGenerateComponent::FindUvData(graph, nodeIndex, uvSetIndex);
        if (!uvData)
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Cannot generate tangents for uv set %zu as it cannot be retrieved.\n", uvSetIndex);
            return false;
        }

        // Check if we had tangents inside the source scene file.
        AZ::SceneAPI::DataTypes::TangentSpace tangentSpace = rule.GetTangentSpace();
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData* tangentData = TangentGenerateComponent::FindTangentData(graph, nodeIndex, uvSetIndex);
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* bitangentData = TangentGenerateComponent::FindBitangentData(graph, nodeIndex, uvSetIndex);

        // If all we need is import from the source scene, and we have tangent data from the source scene already, then skip generating.
        if ((tangentSpace == AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene))
        {
            if (tangentData && bitangentData)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Using source scene tangents and bitangents for uv set %zu for mesh '%s'.\n",
                    uvSetIndex, scene.GetGraph().GetNodeName(nodeIndex).GetName());
            }
            else
            {
                // In case there are no tangents/bitangents while the user selected to use the source ones, default to MikkT.
                AZ_Warning(AZ::SceneAPI::Utilities::WarningWindow, false, "Cannot use source scene tangents as there are none in the asset for mesh '%s' for uv set %zu. Defaulting to generating tangents using MikkT.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
                tangentSpace = AZ::SceneAPI::DataTypes::TangentSpace::MikkT;
            }
        }

        if (!tangentData)
        {
            if (!AZ::SceneGenerationComponents::TangentGenerateComponent::CreateTangentLayer(scene.GetManifest(), nodeIndex, meshData->GetVertexCount(), uvSetIndex,
                tangentSpace, graph, &tangentData))
            {
                AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create tangents data set for mesh %s for uv set %zu.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
                return false;
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Successfully created tangents data set for mesh %s for uv set %zu.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
            }
        }
        if (!bitangentData)
        {
            if (!AZ::SceneGenerationComponents::TangentGenerateComponent::CreateBitangentLayer(scene.GetManifest(), nodeIndex, meshData->GetVertexCount(), uvSetIndex,
                tangentSpace, graph, &bitangentData))
            {
                AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create bitangents data set for mesh %s for uv set %zu.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
                return false;
            }
            else
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Successfully created bitangents data set for mesh %s for uv set %zu.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
            }
        }
        tangentData->SetTangentSpace(tangentSpace);
        bitangentData->SetTangentSpace(tangentSpace);

        bool allSuccess = true;

        switch (tangentSpace)
        {
        // Generate using MikkT space.
        case AZ::SceneAPI::DataTypes::TangentSpace::MikkT:
        {
            const bool success = AZ::TangentGeneration::Mesh::MikkT::GenerateTangents(meshData, uvData, tangentData, bitangentData);
            if (success)
            {
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Successfully generated tangent space for mesh %s for uv set %zu.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
            }
            else
            {
                AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to generate tangent space for mesh %s for uv set %zu.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex);
            }

            allSuccess &= success;

            for (AZ::SceneData::GraphData::BlendShapeData* blendShape : blendShapes)
            {
                allSuccess &= AZ::TangentGeneration::BlendShape::MikkT::GenerateTangents(blendShape, uvSetIndex);
            }
        }
        break;

        case AZ::SceneAPI::DataTypes::TangentSpace::FromSourceScene:
        {
            // Nothing to do
        };
        break;

        default:
        {
            AZ_Assert(false, "Unknown tangent space selected (spaceID=%d) for UV set %d, cannot generate tangents!\n", static_cast<AZ::u32>(tangentSpace), uvSetIndex);
            allSuccess = false;
        }
        }

        return allSuccess;
    }


    template<class DataSource>
    AZStd::optional<AZStd::string> NormalizeNormals(DataSource* meshData)
    {
        AZStd::vector<AZStd::pair<u32, AZ::Vector3>> badVertexIndices;
        for (u32 vertexIndex = 0; vertexIndex < meshData->GetVertexCount(); ++vertexIndex)
        {
            const AZ::Vector3 normalIn = meshData->GetNormal(vertexIndex);
            const auto normalOut = normalIn.GetNormalized();
            if (::isnan(normalOut.GetX()) || ::isnan(normalOut.GetY()) || ::isnan(normalOut.GetZ()))
            {
                badVertexIndices.emplace_back(vertexIndex, normalIn);
                meshData->SetNormal(vertexIndex, AZ::Vector3::CreateAxisZ(1.f));
            }
            else
            {
                meshData->SetNormal(vertexIndex, normalOut);
            }
        }

        if (!badVertexIndices.empty())
        {
            static const auto IndexVectorPairToString = [](const AZStd::vector<AZStd::pair<u32, AZ::Vector3>>& v)
            {
                AZStd::string s = "{";

                for (const auto& e : v)
                {
                    s += e.first;
                    s += ":";
                    s += AZ::ToString(e.second);
                    s += ", ";
                }

                s += "}";
                return s;
            };

            auto badVertexIndicesStr = IndexVectorPairToString(badVertexIndices);
            if (badVertexIndicesStr.length() > 4096)
            {
                badVertexIndicesStr.resize(4096);
                badVertexIndicesStr += " ... truncated";
            }

            return badVertexIndicesStr;
        }

        return {};
    }

    decltype(auto) GetVertexIndex(AZ::SceneData::GraphData::BlendShapeData* dataSource, AZ::u32 faceIndex, AZ::u32 localVertexIndex)
    {
        return dataSource->GetFaceVertexIndex(faceIndex, localVertexIndex);
    }

    decltype(auto) GetVertexIndex(AZ::SceneAPI::DataTypes::IMeshData* dataSource, AZ::u32 faceIndex, AZ::u32 localVertexIndex)
    {
        return dataSource->GetVertexIndex(faceIndex, localVertexIndex);
    }

    template<class DataSource>
    AZStd::optional<AZStd::string> GenerateFlatNormals(DataSource* meshData)
    {
        for (u32 vertexIndex = 0; vertexIndex < meshData->GetVertexCount(); ++vertexIndex)
        {
            meshData->SetNormal(vertexIndex, AZ::Vector3::CreateZero());
        }

        for (u32 faceIndex = 0; faceIndex < meshData->GetFaceCount(); ++faceIndex)
        {
            const auto v0 = GetVertexIndex(meshData, faceIndex, 0);
            const auto v1 = GetVertexIndex(meshData, faceIndex, 1);
            const auto v2 = GetVertexIndex(meshData, faceIndex, 2);

            const auto& p0 = meshData->GetPosition(v0);
            const auto& p1 = meshData->GetPosition(v1);
            const auto& p2 = meshData->GetPosition(v2);

            const auto edge0 = p0 - p1;
            const auto edge1 = p2 - p1;

            // The face normal is `normalize(e0 X e1)'
            // The area of the triangle is `0.5 * length(e0 X e1)'
            // So this is the face normal, weighted by twice the area of the triangle
            const auto weightedFaceNormal = edge0.Cross(edge1);

            const auto AddToNormal = [&](u32 vi)
            {
                meshData->SetNormal(vi, meshData->GetNormal(vi) + weightedFaceNormal);
            };

            AddToNormal(v0); AddToNormal(v1); AddToNormal(v2);
        }

        return {};
    }

    template<class DataSource>
    AZStd::optional<AZStd::string> GenerateSmoothedNormalsFromFlat(DataSource* meshData)
    {
        // Compares vector3s by their exact binary representation
        // This is intentional! We only want to identify truly identical vertices
        struct Vector3_Less
        {
            // Lexographic comparison helper:
            //  - returns `{ result }' iff `a<b' (result==true) or `a>b' (result==false)
            //  - returns `{}' iff `a==b'
            static AZStd::optional<bool> lex_lt(float a, float b)
            {
                // Explicitly handle NaN here, as passing a comparison which doesn't form a total order
                // to `map' can produce undefined behavior, and the comparison operator for floats
                // will return e.g. `0 < nan == false' and `nan < 0 == false'
                const bool a_nan = std::isnan(a);
                const bool b_nan = std::isnan(b);

                // Compare nan<non-nan == true
                if (a_nan && !b_nan) { return { true }; }

                // Compare non-nan < nan == false
                // NB: redundant if your standard library conforms to `IEEE 754',
                // because then this is precisely the behavior of `<'
                else if (!a_nan && b_nan) { return { false }; }

                // Compare nan < nan == ??
                else if (a_nan && b_nan) { return {}; }

                // Non-nan values
                else if (a < b) { return { true }; }
                else if (a > b) { return { false }; }
                else { return {}; }
            }

            bool operator()(const AZ::Vector3& v0, const AZ::Vector3& v1) const
            {
                if (auto r = lex_lt(v0.GetX(), v1.GetX())) { return *r; }
                if (auto r = lex_lt(v0.GetY(), v1.GetY())) { return *r; }
                if (auto r = lex_lt(v0.GetZ(), v1.GetZ())) { return *r; }

                return false; // Equal, so not less than
            }
        };

        AZStd::map<AZ::Vector3, AZ::Vector3, Vector3_Less> verticesByBucket;

        // For all identical vertices, average those normals
        for (u32 vertexIndex = 0; vertexIndex < meshData->GetVertexCount(); ++vertexIndex)
        {
            const auto& pos = meshData->GetPosition(vertexIndex);
            const auto& normal = meshData->GetNormal(vertexIndex);
            auto [it, didInsert] = verticesByBucket.emplace(pos, normal);
            if (!didInsert)
            {
                (*it).second += normal;
            }
        }

        // For every vertex, use the smoothed vertex for that group
        for (u32 vertexIndex = 0; vertexIndex < meshData->GetVertexCount(); ++vertexIndex)
        {
            const auto& pos = meshData->GetPosition(vertexIndex);
            const auto& smoothedNormal = verticesByBucket[pos];
            meshData->SetNormal(vertexIndex, smoothedNormal);
        }

        return {};
    }

    template<class Fn>
    struct lifetime_timer
    {
        lifetime_timer(Fn&& fn)
            : m_fn(fn), m_t0(AZStd::chrono::system_clock::now())
        {
        }

        ~lifetime_timer()
        {
            const auto t1 = AZStd::chrono::system_clock::now();
            m_fn(m_t0, t1);
        }

        AZ_DISABLE_COPY_MOVE(lifetime_timer);

    private:
        Fn m_fn;
        AZStd::chrono::system_clock::time_point m_t0;
    };

    bool TangentGenerateComponent::PerUvGenerateContext::GenerateNormalsForMeshUv() const
    {
        bool allSuccess = true;

        {
            auto prof = lifetime_timer(
            [&](auto t0, auto t1)
            {
                const float seconds = AZStd::chrono::duration<float>(t1 - t0).count();
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Generated normals for mesh %s for uv set %zu took %f seconds.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex, seconds);
            });
            allSuccess &= GenerateNormalsForMeshUv(meshData);
        }

        size_t blendShapeIndex = 0;
        for (AZ::SceneData::GraphData::BlendShapeData* blendShape : blendShapes)
        {
            auto prof = lifetime_timer(
                [&](auto t0, auto t1)
            {
                const float seconds = AZStd::chrono::duration<float>(t1 - t0).count();
                AZ_TracePrintf(AZ::SceneAPI::Utilities::LogWindow, "Generated normals for mesh %s blend shape #%llu for uv set %zu took %f seconds.\n",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(),
                    blendShapeIndex,
                    uvSetIndex,
                    seconds);
            });
            allSuccess &= GenerateNormalsForMeshUv(blendShape);
            blendShapeIndex++;
        }

        return allSuccess;
    }

    template<class DataSource>
    bool TangentGenerateComponent::PerUvGenerateContext::GenerateNormalsForMeshUv(DataSource* dataSource) const
    {
        const AZ::SceneAPI::NormalsSource normalsSource = rule.GetNormalsSource();

        // If the normals source is the scene normals, nothing to do here
        if (normalsSource != AZ::SceneAPI::NormalsSource::FromSourceScene)
        {
            AZ_Assert(
                normalsSource == AZ::SceneAPI::NormalsSource::Flat || normalsSource == AZ::SceneAPI::NormalsSource::Smoothed,
                "Unknown normals source selected (spaceID=%d) for UV set %d, cannot generate normals!\n", static_cast<AZ::u32>(normalsSource), uvSetIndex);

            auto mbErr = GenerateFlatNormals(dataSource);
            if (mbErr)
            {
                AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false,
                    "Could not generate flat normals for mesh %s for uv set %zu. \n%s",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex,
                    mbErr->c_str()
                );
                return false;
            }

            if (normalsSource == AZ::SceneAPI::NormalsSource::Smoothed)
            {
                mbErr = GenerateSmoothedNormalsFromFlat(dataSource);
                if (mbErr)
                {
                    AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false,
                        "Could not generate smoothed normals for mesh %s for uv set %zu. \n%s",
                        scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex,
                        mbErr->c_str()
                    );
                    return false;
                }
            }
        }

        // If we re-generated normals, we must re-normalize, as the generation step creates weighted normals
        if (normalsSource != AZ::SceneAPI::NormalsSource::FromSourceScene || rule.GetShouldRenormalizeNormals())
        {
            auto mbErr = NormalizeNormals(dataSource);
            if (mbErr)
            {
                AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false,
                    "Could not re-normalize normals for mesh %s for uv set %zu. \n%s",
                    scene.GetGraph().GetNodeName(nodeIndex).GetName(), uvSetIndex,
                    mbErr->c_str()
                );
                return false;
            }
        }

        return true;
    }

    size_t TangentGenerateComponent::CalcUvSetCount(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex) const
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        size_t result = 0;
        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexUVData*>(child->second.get());
            if (data)
            {
                result++;
            }
        }

        return result;
    }

    AZ::SceneAPI::DataTypes::IMeshVertexUVData* TangentGenerateComponent::FindUvData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 uvSet)
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        AZ::u64 uvSetIndex = 0;
        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexUVData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexUVData*>(child->second.get());
            if (data)
            {
                if (uvSetIndex == uvSet)
                {
                    return data;
                }
                uvSetIndex++;
            }
        }

        return nullptr;
    }

    AZ::SceneAPI::DataTypes::IMeshVertexTangentData* TangentGenerateComponent::FindTangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex)
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexTangentData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexTangentData*>(child->second.get());
            if (data && setIndex == data->GetTangentSetIndex())
            {
                return data;
            }
        }

        return nullptr;
    }

    bool TangentGenerateComponent::CreateTangentLayer(AZ::SceneAPI::Containers::SceneManifest& manifest,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        size_t numVerts,
        size_t uvSetIndex,
        AZ::SceneAPI::DataTypes::TangentSpace tangentSpace,
        AZ::SceneAPI::Containers::SceneGraph& graph,
        AZ::SceneAPI::DataTypes::IMeshVertexTangentData** outTangentData)
    {
        *outTangentData = nullptr;

        AZStd::shared_ptr<SceneData::GraphData::MeshVertexTangentData> tangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexTangentData>();
        tangentData->Resize(numVerts);

        AZ_Assert(tangentData, "Failed to allocate tangent data for scene graph.");
        if (!tangentData)
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to allocate tangent data.\n");
            return false;
        }

        tangentData->SetTangentSetIndex(uvSetIndex);
        tangentData->SetTangentSpace(tangentSpace);

        const AZStd::string tangentGeneratedName = AZStd::string::format("TangentSet_%zu", uvSetIndex);
        const AZStd::string tangentSetName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexBitangentData>(tangentGeneratedName, manifest);
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex newIndex = graph.AddChild(nodeIndex, tangentSetName.c_str(), tangentData);
        AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for tangent attribute.");
        if (!newIndex.IsValid())
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create node in scene graph that stores tangent data.\n");
            return false;
        }
        graph.MakeEndPoint(newIndex);

        *outTangentData = tangentData.get();
        return true;
    }

    AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* TangentGenerateComponent::FindBitangentData(AZ::SceneAPI::Containers::SceneGraph& graph, const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex, AZ::u64 setIndex)
    {
        const auto nameContentView = AZ::SceneAPI::Containers::Views::MakePairView(graph.GetNameStorage(), graph.GetContentStorage());

        auto meshChildView = AZ::SceneAPI::Containers::Views::MakeSceneGraphChildView<AZ::SceneAPI::Containers::Views::AcceptEndPointsOnly>(graph, nodeIndex, nameContentView.begin(), true);
        for (auto child = meshChildView.begin(); child != meshChildView.end(); ++child)
        {
            AZ::SceneAPI::DataTypes::IMeshVertexBitangentData* data = azrtti_cast<AZ::SceneAPI::DataTypes::IMeshVertexBitangentData*>(child->second.get());
            if (data && setIndex == data->GetBitangentSetIndex())
            {
                return data;
            }
        }

        return nullptr;
    }

    bool TangentGenerateComponent::CreateBitangentLayer(AZ::SceneAPI::Containers::SceneManifest& manifest,
        const AZ::SceneAPI::Containers::SceneGraph::NodeIndex& nodeIndex,
        size_t numVerts,
        size_t uvSetIndex,
        AZ::SceneAPI::DataTypes::TangentSpace tangentSpace,
        AZ::SceneAPI::Containers::SceneGraph& graph,
        AZ::SceneAPI::DataTypes::IMeshVertexBitangentData** outBitangentData)
    {
        *outBitangentData = nullptr;

        AZStd::shared_ptr<AZ::SceneData::GraphData::MeshVertexBitangentData> bitangentData = AZStd::make_shared<AZ::SceneData::GraphData::MeshVertexBitangentData>();
        bitangentData->Resize(numVerts);

        AZ_Assert(bitangentData, "Failed to allocate bitangent data for scene graph.");
        if (!bitangentData)
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to allocate bitangent data.\n");
            return false;
        }

        bitangentData->SetBitangentSetIndex(uvSetIndex);
        bitangentData->SetTangentSpace(tangentSpace);

        const AZStd::string bitangentGeneratedName = AZStd::string::format("BitangentSet_%zu", uvSetIndex);
        const AZStd::string bitangentSetName = AZ::SceneAPI::DataTypes::Utilities::CreateUniqueName<SceneData::GraphData::MeshVertexBitangentData>(bitangentGeneratedName, manifest);
        AZ::SceneAPI::Containers::SceneGraph::NodeIndex newIndex = graph.AddChild(nodeIndex, bitangentSetName.c_str(), bitangentData);
        AZ_Assert(newIndex.IsValid(), "Failed to create SceneGraph node for bitangent attribute.");
        if (!newIndex.IsValid())
        {
            AZ_Error(AZ::SceneAPI::Utilities::ErrorWindow, false, "Failed to create node in scene graph that stores bitangent data.\n");
            return false;
        }
        graph.MakeEndPoint(newIndex);

        *outBitangentData = bitangentData.get();
        return true;
    }
} // namespace AZ::SceneGenerationComponents
