/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Plane.h>
#include "RenderUtil.h"
#include <MCore/Source/Algorithms.h>
#include <MCore/Source/Compare.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include "OrthographicCamera.h"
#include "OrbitCamera.h"

namespace MCommon
{
    // gizmo colors
    MCore::RGBAColor ManipulatorColors::s_selectionColor         = MCore::RGBAColor(1.0f, 1.0f, 0.0f);
    MCore::RGBAColor ManipulatorColors::s_selectionColorDarker   = MCore::RGBAColor(0.5f, 0.5f, 0.0f, 0.5f);
    MCore::RGBAColor ManipulatorColors::s_red                    = MCore::RGBAColor(0.781f, 0.0f, 0.0f);
    MCore::RGBAColor ManipulatorColors::s_green                  = MCore::RGBAColor(0.0f, 0.609f, 0.0f);
    MCore::RGBAColor ManipulatorColors::s_blue                   = MCore::RGBAColor(0.0f, 0.0f, 0.762f);


    // static variables
    uint32 RenderUtil::s_numMaxLineVertices          = 8192 * 16;// 8096 * 16 * sizeof(LineVertex) = 3,5 MB
    uint32 RenderUtil::s_numMaxMeshVertices          = 1024;
    uint32 RenderUtil::s_numMaxMeshIndices           = 1024 * 3;
    uint32 RenderUtil::s_numMax2DLines               = 8192;
    uint32 RenderUtil::s_numMaxTriangleVertices      = 8192 * 16;// 8096 * 16 * sizeof(LineVertex) = 3,5 MB
    float RenderUtil::s_wireframeSphereSegmentCount = 16.0f;


    // constructor
    RenderUtil::RenderUtil()
        : m_devicePixelRatio(1.0f)
    {
        m_vertexBuffer   = new LineVertex[s_numMaxLineVertices];
        m_m2DLines        = new Line2D[s_numMax2DLines];
        m_numVertices    = 0;
        m_num2DLines     = 0;
        m_unitSphereMesh = CreateSphere(1.0f);
        m_cylinderMesh   = CreateCylinder(2.0f, 1.0f, 2.0f);
        m_arrowHeadMesh  = CreateArrowHead(1.0f, 0.5f);
        m_unitCubeMesh   = CreateCube(1.0f);
        m_font           = new VectorFont(this);
    }


    // destructor
    RenderUtil::~RenderUtil()
    {
        delete[]    m_vertexBuffer;
        delete[]    m_m2DLines;
        delete      m_unitSphereMesh;
        delete      m_cylinderMesh;
        delete      m_unitCubeMesh;
        delete      m_arrowHeadMesh;
        delete      m_font;

        // get rid of the world space positions
        m_worldSpacePositions.clear();
    }


    // render lines from the local vertex buffer
    void RenderUtil::RenderLines()
    {
        // check if we have to render anything and skip directly in case the line vertex buffer is empty
        if (m_numVertices == 0)
        {
            return;
        }

        // render the lines and reset the number of vertices
        RenderLines(m_vertexBuffer, m_numVertices);
        m_numVertices = 0;
    }


    // render 2D lines from the local line buffer
    void RenderUtil::Render2DLines()
    {
        // check if we have to render anything and skip directly in case the line buffer is empty
        if (m_num2DLines == 0)
        {
            return;
        }

        // render the lines and reset the number of lines
        Render2DLines(m_m2DLines, m_num2DLines);
        m_num2DLines = 0;
    }


    // render triangles from the local vertex buffer
    void RenderUtil::RenderTriangles()
    {
        // check if we have to render anything and skip directly in case there are no triangles
        if (m_triangleVertices.empty())
        {
            return;
        }

        // render the triangles and clear the array
        RenderTriangles(m_triangleVertices);
        m_triangleVertices.clear();
    }


    // render a grid
    void RenderUtil::RenderGrid(AZ::Vector2 start, AZ::Vector2 end, const AZ::Vector3& normal, float scale, const MCore::RGBAColor& mainAxisColor, const MCore::RGBAColor& gridColor, const MCore::RGBAColor& subStepColor, bool directlyRender)
    {
        start.SetX(start.GetX() - MCore::Math::FMod(start.GetX(), scale));
        start.SetY(start.GetY() - MCore::Math::FMod(start.GetY(), scale));
        end.SetX(end.GetX() - MCore::Math::FMod(end.GetX(), scale));
        end.SetY(end.GetY() - MCore::Math::FMod(end.GetY(), scale));

        AZ::Vector3 gridLineStart, gridLineEnd;
        MCore::RGBAColor color;

        const AZ::Matrix3x3 matRotate = MCore::GetRotationMatrixFromTwoVectors(AZ::Vector3(0.0f, 1.0f, 0.0f), normal);

        const uint32    gridBlockSize       = 5;
        const uint32    numVerticalLines    = static_cast<uint32>((end.GetX() - start.GetX()) / scale);// x component
        const uint32    numHorizontalLines  = static_cast<uint32>((end.GetY() - start.GetY()) / scale);// y component
        const float     scaledGridBlockSize = gridBlockSize * scale;
        const float     maxFmodError        = scale * 0.1f;

        // render all vertical grid lines
        for (uint32 x = 0; x <= numVerticalLines; ++x)
        {
            gridLineStart.Set(start.GetX() + x * scale, 0.0f, start.GetY());
            gridLineEnd.Set(gridLineStart.GetX(), 0.0f, end.GetY());

            const float fmodStartValue = MCore::Math::FMod(MCore::Math::Abs(gridLineStart.GetX()), scaledGridBlockSize);

            // coordinate axis
            if (MCore::Compare<float>::CheckIfIsClose(gridLineStart.GetX(), 0.0f, maxFmodError))
            {
                color = mainAxisColor;
            }
            // substep lines
            else if (MCore::Compare<float>::CheckIfIsClose(fmodStartValue, 0.0f, maxFmodError) || MCore::Compare<float>::CheckIfIsClose(fmodStartValue, scaledGridBlockSize, maxFmodError))
            {
                color = subStepColor;
            }
            // block lines
            else
            {
                color = gridColor;
            }

            RenderLine(matRotate * gridLineStart, matRotate * gridLineEnd, color);
        }

        // render all horizontal grid lines
        for (uint32 y = 0; y <= numHorizontalLines; ++y)
        {
            gridLineStart.Set(start.GetX(), 0.0f, start.GetY() + y * scale);
            gridLineEnd.Set(end.GetX(), 0.0f, gridLineStart.GetZ());

            const float fmodStartValue = MCore::Math::FMod(MCore::Math::Abs(gridLineStart.GetZ()), scaledGridBlockSize);

            // coordinate axis
            if (MCore::Compare<float>::CheckIfIsClose(gridLineStart.GetZ(), 0.0f, maxFmodError))
            {
                color = mainAxisColor;
            }
            // substep lines
            else if (MCore::Compare<float>::CheckIfIsClose(fmodStartValue, 0.0f, maxFmodError) || MCore::Compare<float>::CheckIfIsClose(fmodStartValue, scaledGridBlockSize, maxFmodError))
            {
                color = subStepColor;
            }
            // block lines
            else
            {
                color = gridColor;
            }

            RenderLine(matRotate * gridLineStart, matRotate * gridLineEnd, color);
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }


    // render the current bounding box of the given actor instance
    void RenderUtil::RenderAabb(const AZ::Aabb& box, const MCore::RGBAColor& color, bool directlyRender)
    {
        AZ::Vector3 min = box.GetMin();
        AZ::Vector3 max = box.GetMax();

        // generate our vertices
        AZ::Vector3 p[8];
        p[0].Set(min.GetX(), min.GetY(), min.GetZ());
        p[1].Set(max.GetX(), min.GetY(), min.GetZ());
        p[2].Set(max.GetX(), min.GetY(), max.GetZ());
        p[3].Set(min.GetX(), min.GetY(), max.GetZ());
        p[4].Set(min.GetX(), max.GetY(), min.GetZ());
        p[5].Set(max.GetX(), max.GetY(), min.GetZ());
        p[6].Set(max.GetX(), max.GetY(), max.GetZ());
        p[7].Set(min.GetX(), max.GetY(), max.GetZ());

        // render the box
        RenderLine(p[0], p[1], color);
        RenderLine(p[1], p[2], color);
        RenderLine(p[2], p[3], color);
        RenderLine(p[3], p[0], color);

        RenderLine(p[4], p[5], color);
        RenderLine(p[5], p[6], color);
        RenderLine(p[6], p[7], color);
        RenderLine(p[7], p[4], color);

        RenderLine(p[0], p[4], color);
        RenderLine(p[1], p[5], color);
        RenderLine(p[2], p[6], color);
        RenderLine(p[3], p[7], color);

        if (directlyRender)
        {
            RenderLines();
        }
    }


    // render selection gizmo around the given AABB
    void RenderUtil::RenderSelection(const AZ::Aabb& box, const MCore::RGBAColor& color, bool directlyRender)
    {
        const AZ::Vector3&      min     = box.GetMin();
        const AZ::Vector3&      max     = box.GetMax();
        const float             radius  = AZ::Vector3(box.GetMax() - box.GetMin()).GetLength() * 0.5f;
        const float             scale   = radius * 0.1f;
        const AZ::Vector3       up      = AZ::Vector3(0.0f, 1.0f, 0.0f) * scale;
        const AZ::Vector3       right   = AZ::Vector3(1.0f, 0.0f, 0.0f) * scale;
        const AZ::Vector3       front   = AZ::Vector3(0.0f, 0.0f, 1.0f) * scale;

        // generate our vertices
        const AZStd::array p
        {
            AZ::Vector3{min.GetX(), min.GetY(), min.GetZ()},
            AZ::Vector3{max.GetX(), min.GetY(), min.GetZ()},
            AZ::Vector3{max.GetX(), min.GetY(), max.GetZ()},
            AZ::Vector3{min.GetX(), min.GetY(), max.GetZ()},
            AZ::Vector3{min.GetX(), max.GetY(), min.GetZ()},
            AZ::Vector3{max.GetX(), max.GetY(), min.GetZ()},
            AZ::Vector3{max.GetX(), max.GetY(), max.GetZ()},
            AZ::Vector3{min.GetX(), max.GetY(), max.GetZ()},
        };

        // render the box
        RenderLine(p[0], p[0] + up,    color);
        RenderLine(p[0], p[0] + right, color);
        RenderLine(p[0], p[0] + front, color);

        RenderLine(p[1], p[1] + up,    color);
        RenderLine(p[1], p[1] - right, color);
        RenderLine(p[1], p[1] + front, color);

        RenderLine(p[2], p[2] + up,    color);
        RenderLine(p[2], p[2] - right, color);
        RenderLine(p[2], p[2] - front, color);

        RenderLine(p[3], p[3] + up,    color);
        RenderLine(p[3], p[3] + right, color);
        RenderLine(p[3], p[3] - front, color);

        RenderLine(p[4], p[4] - up,    color);
        RenderLine(p[4], p[4] + right, color);
        RenderLine(p[4], p[4] + front, color);

        RenderLine(p[5], p[5] - up,    color);
        RenderLine(p[5], p[5] - right, color);
        RenderLine(p[5], p[5] + front, color);

        RenderLine(p[6], p[6] - up,    color);
        RenderLine(p[6], p[6] - right, color);
        RenderLine(p[6], p[6] - front, color);

        RenderLine(p[7], p[7] - up,    color);
        RenderLine(p[7], p[7] + right, color);
        RenderLine(p[7], p[7] - front, color);

        if (directlyRender)
        {
            RenderLines();
        }
    }


    // constructor
    RenderUtil::AABBRenderSettings::AABBRenderSettings()
    {
        m_nodeBasedAabb              = true;
        m_meshBasedAabb              = true;
        m_staticBasedAabb            = true;
        m_staticBasedColor           = MCore::RGBAColor(0.0f, 0.7f, 0.7f);
        m_nodeBasedColor             = MCore::RGBAColor(1.0f, 0.0f, 0.0f);
        m_meshBasedColor             = MCore::RGBAColor(0.0f, 0.0f, 0.7f);
    }


    // render the given types of AABBs of a actor instance
    void RenderUtil::RenderAabbs(EMotionFX::ActorInstance* actorInstance, const AABBRenderSettings& renderSettings, bool directlyRender)
    {
        const size_t lodLevel = actorInstance->GetLODLevel();

        // handle the node based AABB
        if (renderSettings.m_nodeBasedAabb)
        {
            // calculate the node based AABB
            AZ::Aabb box;
            actorInstance->CalcNodeBasedAabb(&box);

            // render the aabb
            if (box.IsValid())
            {
                RenderAabb(box, renderSettings.m_nodeBasedColor);
            }
        }

        // handle the mesh based AABB
        if (renderSettings.m_meshBasedAabb)
        {
            // calculate the mesh based AABB
            AZ::Aabb box;
            actorInstance->CalcMeshBasedAabb(lodLevel, &box);

            // render the aabb
            if (box.IsValid())
            {
                RenderAabb(box, renderSettings.m_meshBasedColor);
            }
        }

        if (renderSettings.m_staticBasedAabb)
        {
            // calculate the static based AABB
            AZ::Aabb box;
            actorInstance->CalcStaticBasedAabb(&box);

            // render the aabb
            if (box.IsValid())
            {
                RenderAabb(box, renderSettings.m_staticBasedColor);
            }
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }


    // render a simple line based skeleton
    void RenderUtil::RenderSimpleSkeleton(EMotionFX::ActorInstance* actorInstance, const AZStd::unordered_set<size_t>* visibleJointIndices,
        const AZStd::unordered_set<size_t>* selectedJointIndices, const MCore::RGBAColor& color, const MCore::RGBAColor& selectedColor,
        float jointSphereRadius, bool directlyRender)
    {
        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();

        const size_t numNodes = actorInstance->GetNumEnabledNodes();
        for (size_t n = 0; n < numNodes; ++n)
        {
            const EMotionFX::Node* joint = skeleton->GetNode(actorInstance->GetEnabledNode(n));
            const size_t jointIndex = joint->GetNodeIndex();

            if (!visibleJointIndices || visibleJointIndices->empty() ||
                (visibleJointIndices->find(jointIndex) != visibleJointIndices->end()))
            {
                const AZ::Vector3 currentJointPos = pose->GetWorldSpaceTransform(jointIndex).m_position;
                const bool jointSelected = selectedJointIndices->find(jointIndex) != selectedJointIndices->end();

                const size_t parentIndex = joint->GetParentIndex();
                if (parentIndex != InvalidIndex)
                {
                    const bool parentSelected = selectedJointIndices->find(parentIndex) != selectedJointIndices->end();
                    const AZ::Vector3 parentJointPos = pose->GetWorldSpaceTransform(parentIndex).m_position;
                    RenderLine(currentJointPos, parentJointPos, parentSelected ? selectedColor : color);
                }

                RenderSphere(currentJointPos, jointSphereRadius, jointSelected ? selectedColor : color);
            }
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }

    // render wireframe mesh
    void RenderUtil::RenderWireframe(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, const MCore::RGBAColor& color, bool directlyRender, float offsetScale)
    {
        // check if the mesh is valid and skip the node in case it's not
        if (mesh == NULL)
        {
            return;
        }

        PrepareForMesh(mesh, worldTM);

        const float scale = 0.01f * offsetScale;

        AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS);
        MCore::RGBAColor* vertexColors = (MCore::RGBAColor*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_COLORS128);

        const size_t numSubMeshes = mesh->GetNumSubMeshes();
        for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
        {
            EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
            const uint32 numTriangles = subMesh->GetNumPolygons();
            const uint32 startVertex = subMesh->GetStartVertex();
            const uint32* indices = subMesh->GetIndices();

            for (uint32 triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
            {
                const uint32 triangleStartIndex = triangleIndex * 3;
                const uint32 indexA = indices[triangleStartIndex + 0] + startVertex;
                const uint32 indexB = indices[triangleStartIndex + 1] + startVertex;
                const uint32 indexC = indices[triangleStartIndex + 2] + startVertex;

                const AZ::Vector3 posA = m_worldSpacePositions[indexA] + normals[indexA] * scale;
                const AZ::Vector3 posB = m_worldSpacePositions[indexB] + normals[indexB] * scale;
                const AZ::Vector3 posC = m_worldSpacePositions[indexC] + normals[indexC] * scale;

                if (vertexColors)
                {
                    RenderLine(posA, posB, vertexColors[indexA]);
                    RenderLine(posB, posC, vertexColors[indexB]);
                    RenderLine(posC, posA, vertexColors[indexC]);
                }
                else
                {
                    RenderLine(posA, posB, color);
                    RenderLine(posB, posC, color);
                    RenderLine(posC, posA, color);
                }
            }
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }

    // render vertex and face normals
    void RenderUtil::RenderNormals(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, bool vertexNormals, bool faceNormals, float vertexNormalsScale, float faceNormalsScale, const MCore::RGBAColor& colorVertexNormals, const MCore::RGBAColor& colorFaceNormals, bool directlyRender)
    {
        // check if the mesh is valid and skip the node in case it's not
        if (mesh == NULL)
        {
            return;
        }

        // check if we need to render anything at all
        if (vertexNormals == false && faceNormals == false)
        {
            return;
        }

        PrepareForMesh(mesh, worldTM);

        AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS);

        // render face normals
        if (faceNormals)
        {
            const size_t numSubMeshes = mesh->GetNumSubMeshes();
            for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
                const uint32 numTriangles = subMesh->GetNumPolygons();
                const uint32 startVertex = subMesh->GetStartVertex();
                const uint32* indices = subMesh->GetIndices();

                for (uint32 triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
                {
                    const uint32 triangleStartIndex = triangleIndex * 3;
                    const uint32 indexA = indices[triangleStartIndex + 0] + startVertex;
                    const uint32 indexB = indices[triangleStartIndex + 1] + startVertex;
                    const uint32 indexC = indices[triangleStartIndex + 2] + startVertex;

                    const AZ::Vector3& posA = m_worldSpacePositions[ indexA ];
                    const AZ::Vector3& posB = m_worldSpacePositions[ indexB ];
                    const AZ::Vector3& posC = m_worldSpacePositions[ indexC ];

                    const AZ::Vector3 normalDir = (posB - posA).Cross(posC - posA).GetNormalized();

                    // calculate the center pos
                    const AZ::Vector3 normalPos = (posA + posB + posC) * (1.0f/3.0f);

                    RenderLine(normalPos, normalPos + (normalDir * faceNormalsScale), colorFaceNormals);
                }
            }
        }

        // render vertex normals
        if (vertexNormals)
        {
            const size_t numSubMeshes = mesh->GetNumSubMeshes();
            for (uint32 subMeshIndex = 0; subMeshIndex < numSubMeshes; ++subMeshIndex)
            {
                EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(subMeshIndex);
                const uint32 numVertices = subMesh->GetNumVertices();
                const uint32 startVertex = subMesh->GetStartVertex();

                for (uint32 j = 0; j < numVertices; ++j)
                {
                    const uint32 vertexIndex = j + startVertex;
                    const AZ::Vector3& position = m_worldSpacePositions[vertexIndex];
                    const AZ::Vector3 normal = worldTM.TransformVector(normals[vertexIndex]).GetNormalizedSafe() * vertexNormalsScale;
                    RenderLine(position, position + normal, colorVertexNormals);
                }
            }
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }


    // render tangents and bitangents of the mesh
    void RenderUtil::RenderTangents(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM, float scale, const MCore::RGBAColor& colorTangents, const MCore::RGBAColor& mirroredBitangentColor, const MCore::RGBAColor& colorBitangent, bool directlyRender)
    {
        // check if the mesh is valid and skip the node in case it's not
        if (mesh == NULL)
        {
            return;
        }

        // get the tangents and check if this mesh actually has tangents
        AZ::Vector4* tangents = static_cast<AZ::Vector4*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
        if (tangents == NULL)
        {
            return;
        }

        AZ::Vector3* bitangents = static_cast<AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_BITANGENTS));

        PrepareForMesh(mesh, worldTM);

        AZ::Vector3* normals = (AZ::Vector3*)mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS);
        const uint32 numVertices = mesh->GetNumVertices();

        // render the tangents and bitangents
        AZ::Vector3 orgTangent, tangent, bitangent;
        for (uint32 i = 0; i < numVertices; ++i)
        {
            orgTangent.Set(tangents[i].GetX(), tangents[i].GetY(), tangents[i].GetZ());
            tangent = (worldTM.TransformVector(orgTangent)).GetNormalized();

            if (bitangents)
            {
                bitangent = bitangents[i];
            }
            else
            {
                bitangent = tangents[i].GetW() * normals[i].Cross(orgTangent);
            }
            bitangent = (worldTM.TransformVector(bitangent)).GetNormalizedSafe();

            RenderLine(m_worldSpacePositions[i], m_worldSpacePositions[i] + (tangent * scale), colorTangents);

            if (tangents[i].GetW() < 0.0f)
            {
                RenderLine(m_worldSpacePositions[i], m_worldSpacePositions[i] + (bitangent * scale), mirroredBitangentColor);
            }
            else
            {
                RenderLine(m_worldSpacePositions[i], m_worldSpacePositions[i] + (bitangent * scale), colorBitangent);
            }
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }


    // precalculate data for rendering for the given mesh
    void RenderUtil::PrepareForMesh(EMotionFX::Mesh* mesh, const AZ::Transform& worldTM)
    {
        // check if we have already prepared for the given mesh
        if (m_currentMesh == mesh)
        {
            return;
        }

        // set our new current mesh
        m_currentMesh = mesh;

        // get the number of vertices and the data
        const uint32    numVertices = m_currentMesh->GetNumVertices();
        AZ::Vector3*    positions   = (AZ::Vector3*)m_currentMesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS);

        // check if the vertices fits in our buffer
        if (m_worldSpacePositions.size() < numVertices)
        {
            m_worldSpacePositions.resize(numVertices);
        }

        // pre-calculate the world space positions
        for (uint32 i = 0; i < numVertices; ++i)
        {
            m_worldSpacePositions[i] = worldTM.TransformPoint(positions[i]);
        }
    }


    // calculate the size of the joint sphere
    float RenderUtil::GetBoneScale(EMotionFX::ActorInstance* actorInstance, EMotionFX::Node* node)
    {
        // get the transform data
        EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();

        const size_t            nodeIndex       = node->GetNodeIndex();
        const size_t            parentIndex     = node->GetParentIndex();
        const AZ::Vector3       nodeWorldPos    = pose->GetWorldSpaceTransform(nodeIndex).m_position;

        if (parentIndex != InvalidIndex)
        {
            const AZ::Vector3       parentWorldPos  = pose->GetWorldSpaceTransform(parentIndex).m_position;
            const AZ::Vector3       bone            = parentWorldPos - nodeWorldPos;
            const float             boneLength      = MCore::SafeLength(bone);

            // 10% of the bone length is the sphere size
            return boneLength * 0.1f;
        }

        return 0.0f;
    }


    // render the advanced skeleton
    void RenderUtil::RenderSkeleton(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<size_t>& boneList, const AZStd::unordered_set<size_t>* visibleJointIndices, const AZStd::unordered_set<size_t>* selectedJointIndices, const MCore::RGBAColor& color, const MCore::RGBAColor& selectedColor)
    {
        // check if our render util supports rendering meshes, if not render the fallback skeleton using lines only
        if (GetIsMeshRenderingSupported() == false)
        {
            RenderSimpleSkeleton(actorInstance, visibleJointIndices, selectedJointIndices, color, selectedColor, true);
            return;
        }

        // get the actor it is an instance from
        EMotionFX::Actor* actor = actorInstance->GetActor();
        EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();

        // iterate through all enabled nodes
        MCore::RGBAColor tempColor;
        const size_t numEnabled = actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numEnabled; ++i)
        {
            EMotionFX::Node* joint = skeleton->GetNode(actorInstance->GetEnabledNode(i));
            const size_t jointIndex = joint->GetNodeIndex();
            const size_t parentIndex = joint->GetParentIndex();

            // check if this node has a parent and is a bone, if not skip it
            if (parentIndex == InvalidIndex || AZStd::find(begin(boneList), end(boneList), jointIndex) == end(boneList))
            {
                continue;
            }

            if (!visibleJointIndices || visibleJointIndices->empty() ||
                (visibleJointIndices->find(jointIndex) != visibleJointIndices->end()))
            {
                const AZ::Vector3       nodeWorldPos        = pose->GetWorldSpaceTransform(jointIndex).m_position;
                const AZ::Vector3       parentWorldPos      = pose->GetWorldSpaceTransform(parentIndex).m_position;
                const AZ::Vector3       bone                = parentWorldPos - nodeWorldPos;
                const AZ::Vector3       boneDirection       = MCore::SafeNormalize(bone);
                const float             boneLength          = MCore::SafeLength(bone);
                const float             boneScale           = GetBoneScale(actorInstance, joint);
                const float             parentBoneScale     = GetBoneScale(actorInstance, skeleton->GetNode(parentIndex));
                const float             cylinderSize        = boneLength - boneScale - parentBoneScale;
                const AZ::Vector3       boneStartPosition   = nodeWorldPos + boneDirection * boneScale;

                // check if the current bone is selected and set the color according to it
                if (selectedJointIndices && selectedJointIndices->find(jointIndex) != selectedJointIndices->end())
                {
                    tempColor = selectedColor;
                }
                else
                {
                    tempColor = color;
                }

                // render the bone cylinder, the cylinder will be directed towards the node's parent and must fit between the spheres
                RenderCylinder(boneScale, parentBoneScale, cylinderSize, boneStartPosition, boneDirection, tempColor);
                RenderSphere(nodeWorldPos, boneScale, tempColor);
            }
        }
    }


    // render node orientations
    void RenderUtil::RenderNodeOrientations(EMotionFX::ActorInstance* actorInstance, const AZStd::vector<size_t>& boneList, const AZStd::unordered_set<size_t>* visibleJointIndices, const AZStd::unordered_set<size_t>* selectedJointIndices, float scale, bool scaleBonesOnLength)
    {
        // get the actor and the transform data
        const float unitScale = 1.0f / (float)MCore::Distance::ConvertValue(1.0f, MCore::Distance::UNITTYPE_METERS, EMotionFX::GetEMotionFX().GetUnitType());
        const EMotionFX::Actor*         actor           = actorInstance->GetActor();
        const EMotionFX::Skeleton*      skeleton        = actor->GetSkeleton();
        const EMotionFX::TransformData* transformData   = actorInstance->GetTransformData();
        const EMotionFX::Pose*          pose            = transformData->GetCurrentPose();
        const float                     constPreScale   = scale * unitScale * 3.0f;
        AxisRenderingSettings axisRenderingSettings;

        const size_t numEnabled = actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numEnabled; ++i)
        {
            EMotionFX::Node* joint = skeleton->GetNode(actorInstance->GetEnabledNode(i));
            const size_t jointIndex = joint->GetNodeIndex();
            const size_t parentIndex = joint->GetParentIndex();

            if (!visibleJointIndices || visibleJointIndices->empty() ||
                (visibleJointIndices->find(jointIndex) != visibleJointIndices->end()))
            {
                // either scale the bones based on their length or use the normal size
                if (scaleBonesOnLength && parentIndex != InvalidIndex && AZStd::find(begin(boneList), end(boneList), jointIndex) != end(boneList))
                {
                    static const float axisBoneScale = 50.0f;
                    axisRenderingSettings.m_size = GetBoneScale(actorInstance, joint) * constPreScale * axisBoneScale;
                }
                else
                {
                    axisRenderingSettings.m_size = constPreScale;
                }

                // check if the current bone is selected and set the color according to it
                if (selectedJointIndices && selectedJointIndices->find(jointIndex) != selectedJointIndices->end())
                {
                    axisRenderingSettings.m_selected = true;
                }
                else
                {
                    axisRenderingSettings.m_selected = false;
                }

                axisRenderingSettings.m_worldTm = pose->GetWorldSpaceTransform(jointIndex).ToAZTransform();
                RenderLineAxis(axisRenderingSettings);
            }
        }
    }


    // visualize the actor bind pose
    void RenderUtil::RenderBindPose(EMotionFX::ActorInstance* actorInstance, const MCore::RGBAColor& color, bool directlyRender)
    {
        // get the actor it is an instance from
        EMotionFX::Actor*       actor       = actorInstance->GetActor();
        EMotionFX::Skeleton*    skeleton    = actor->GetSkeleton();
        const EMotionFX::Pose*  pose        = actorInstance->GetTransformData()->GetCurrentPose();

        AxisRenderingSettings axisRenderingSettings;

        // iterate through all enabled nodes
        const size_t numEnabled = actorInstance->GetNumEnabledNodes();
        for (size_t i = 0; i < numEnabled; ++i)
        {
            EMotionFX::Node*    node        = skeleton->GetNode(actorInstance->GetEnabledNode(i));
            const size_t        nodeIndex   = node->GetNodeIndex();

            // render node orientation
            const EMotionFX::Transform worldTransform = pose->GetWorldSpaceTransform(nodeIndex);
            axisRenderingSettings.m_size     = GetBoneScale(actorInstance, node) * 5.0f;
            axisRenderingSettings.m_worldTm  = worldTransform.ToAZTransform();
            RenderLineAxis(axisRenderingSettings);// line based axis rendering

            // skip root nodes for the line based skeleton rendering, you could also use curNode->IsRootNode()
            // but we use the parent index here, as we will reuse it
            size_t parentIndex = node->GetParentIndex();
            if (parentIndex != InvalidIndex)
            {
                const AZ::Vector3 endPos = pose->GetWorldSpaceTransform(parentIndex).m_position;
                RenderLine(worldTransform.m_position, endPos, color);
            }
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////

    // constructor
    RenderUtil::UtilMesh::UtilMesh()
    {
    }


    // destructor
    RenderUtil::UtilMesh::~UtilMesh()
    {
    }


    // recalculate the normals of the mesh
    void RenderUtil::UtilMesh::CalculateNormals(bool counterClockWise)
    {
        // check if the normals actually got allocated
        if (m_normals.empty())
        {
            return;
        }

        // reset all normals to the zero vector
        const size_t numNormals = m_normals.size();
        MCore::MemSet(&m_normals[0], 0, sizeof(AZ::Vector3) * numNormals);

        // iterate through all vertices and sum up the face normals
        uint32 i;
        uint32 indexA, indexB, indexC;
        AZ::Vector3 v1, v2, normal;

        for (i = 0; i < numNormals; i += 3)
        {
            indexA = m_indices[i];
            indexB = m_indices[i + (counterClockWise ? 1 : 2)];
            indexC = m_indices[i + (counterClockWise ? 2 : 1)];

            v1 = m_positions[indexB] - m_positions[indexA];
            v2 = m_positions[indexC] - m_positions[indexA];

            normal = v1.Cross(v2);

            m_normals[indexA] = m_normals[indexA] + normal;
            m_normals[indexB] = m_normals[indexB] + normal;
            m_normals[indexC] = m_normals[indexC] + normal;
        }

        // normalize all the normals
        for (i = 0; i < numNormals; ++i)
        {
            m_normals[i] = m_normals[i].GetNormalized();
        }
    }


    // allocate memory for the vertices, indices and normals
    void RenderUtil::UtilMesh::Allocate(uint32 numVertices, uint32 numIndices, bool hasNormals)
    {
        AZ_Assert(numVertices > 0 && numIndices % 3 == 0, "Invalid numVertices or numIndices");
        AZ_Assert(m_positions.empty() && m_indices.empty() && m_normals.empty(), "data already initialized");

        // allocate the buffers
        m_positions.resize(numVertices);
        m_indices.resize(numIndices);
        if (hasNormals)
        {
            m_normals.resize(numVertices);
        }
    }


    // create a cylinder mesh
    RenderUtil::UtilMesh* RenderUtil::CreateCylinder(float baseRadius, float topRadius, float length, uint32 numSegments)
    {
        // create the util mesh for our cylinder
        UtilMesh* cylinderMesh = new UtilMesh();

        // allocate memory for the vertices, normals and indices
        const uint32    numVertices = numSegments * 2;
        const uint32    numIndices  = numSegments * 2 * 3;
        cylinderMesh->Allocate(numVertices, numIndices, true);

        // construct the actual mesh
        FillCylinder(cylinderMesh, baseRadius, topRadius, length, true);

        return cylinderMesh;
    }


    // fill the cylinder vertex and index buffers
    void RenderUtil::FillCylinder(UtilMesh* mesh, float baseRadius, float topRadius, float length, bool calculateNormals)
    {
        // check if the positions and the indices have been allocated already by the CreateCylinder() function
        if (mesh->m_positions.empty() || mesh->m_indices.empty())
        {
            return;
        }

        // number of segments/sides of the cylinder
        const uint32 numSegments = static_cast<uint32>(mesh->m_positions.size()) / 2;

        // fill in the vertices
        uint32 i;
        for (i = 0; i < numSegments; ++i)
        {
            const float p = i / (float)numSegments * 2.0f * MCore::Math::pi;
            const float z = MCore::Math::Sin(p);
            const float y = MCore::Math::Cos(p);

            mesh->m_positions[i]               = AZ::Vector3(0.0f,    y * baseRadius, z * baseRadius);
            mesh->m_positions[i + numSegments] = AZ::Vector3(-length, y * topRadius,  z * topRadius);
        }

        // fill in the indices
        uint32 c = 0;
        for (i = 0; i < numSegments; ++i)
        {
            mesh->m_indices[c++] = i;
            mesh->m_indices[c++] = ((i + 1) % numSegments);
            mesh->m_indices[c++] = i + numSegments;
        }

        for (i = 0; i < numSegments; ++i)
        {
            mesh->m_indices[c++] = i + numSegments;
            mesh->m_indices[c++] = ((i + 1) % numSegments);
            mesh->m_indices[c++] = ((i + 1) % numSegments) + numSegments;
        }

        // recalculate normals if desired
        if (calculateNormals)
        {
            mesh->CalculateNormals();
        }
    }


    // render the given cylinder
    void RenderUtil::RenderCylinder(float baseRadius, float topRadius, float length, const AZ::Vector3& position, const AZ::Vector3& direction, const MCore::RGBAColor& color)
    {
        AZ::Transform worldTM;

        // rotate the cylinder to the desired direction
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(direction, AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::epsilon) == false)
        {
            worldTM = MCore::GetRotationMatrixAxisAngle(AZ::Vector3(-1.0f, 0.0f, 0.0f).Cross(direction), MCore::Math::ACos(direction.Dot(AZ::Vector3(-1.0f, 0.0f, 0.0f))));
        }
        else
        {
            worldTM = AZ::Transform::CreateFromQuaternion(MCore::AzEulerAnglesToAzQuat(0.0f, 0.0f, MCore::Math::DegreesToRadians(180.0f)));
        }

        // set the cylinder to the given position
        worldTM.SetTranslation(position);

        // render the cylinder
        RenderCylinder(baseRadius, topRadius, length, color, worldTM);
    }


    // construct a sphere mesh
    RenderUtil::UtilMesh* RenderUtil::CreateSphere(float radius, uint32 numSegments)
    {
        uint32 i;

        // create our util mesh for the sphere
        UtilMesh* sphereMesh = new UtilMesh();

        // calculate the number of vertices and indices
        const uint32 numVertices =  (numSegments - 2) * numSegments + 2;
        uint32 numIndices       =  (numSegments - 3) * 6;
        numIndices              += (numSegments - 3) * (numSegments - 1) * 6;
        numIndices              += (numSegments - 1) * 3;
        numIndices              += (numSegments - 1) * 3;
        numIndices              += 6;

        // allocate memory for the vertices, normals and indices
        sphereMesh->Allocate(numVertices, numIndices, true);

        // fill the vertices
        for (i = 1; i < numSegments - 1; ++i)
        {
            const float z = (1.0f - (i / (float)(numSegments - 1)) * 2.0f);
            const float r = MCore::Math::Sin(MCore::Math::ACos(z)) * radius;

            for (uint32 j = 0; j < numSegments; j++)
            {
                const float p = (j / (float)numSegments) * MCore::Math::pi * 2.0f;
                const float x = r * MCore::Math::Sin(p);
                const float y = r * MCore::Math::Cos(p);

                sphereMesh->m_positions[(i - 1) * numSegments + j] = AZ::Vector3(x, y, z * radius);
            }
        }

        // the highest and lowest vertices
        sphereMesh->m_positions[(numSegments - 2) * numSegments + 0]   = AZ::Vector3(0.0f, 0.0f,  radius);
        sphereMesh->m_positions[(numSegments - 2) * numSegments + 1]   = AZ::Vector3(0.0f, 0.0f, -radius);

        // calculate normals
        const size_t numPositions = sphereMesh->m_positions.size();
        for (i = 0; i < numPositions; ++i)
        {
            sphereMesh->m_normals[i] = -sphereMesh->m_positions[i].GetNormalized();
        }

        // fill the indices
        uint32 c = 0;
        for (i = 1; i < numSegments - 2; ++i)
        {
            for (uint32 j = 0; j < numSegments - 1; j++)
            {
                sphereMesh->m_indices[c++] = (i - 1) * numSegments + j;
                sphereMesh->m_indices[c++] = (i - 1) * numSegments + j + 1;
                sphereMesh->m_indices[c++] = i * numSegments + j;

                sphereMesh->m_indices[c++] = (i - 1) * numSegments + j + 1;
                sphereMesh->m_indices[c++] = i * numSegments + j + 1;
                sphereMesh->m_indices[c++] = i * numSegments + j;
            }

            sphereMesh->m_indices[c++] = (i - 1) * numSegments + numSegments - 1;
            sphereMesh->m_indices[c++] = (i - 1) * numSegments;
            sphereMesh->m_indices[c++] = i * numSegments + numSegments - 1;

            sphereMesh->m_indices[c++] = i * numSegments;
            sphereMesh->m_indices[c++] = (i - 1) * numSegments;
            sphereMesh->m_indices[c++] = i * numSegments + numSegments - 1;
        }

        // highest and deepest indices
        for (i = 0; i < numSegments - 1; ++i)
        {
            sphereMesh->m_indices[c++] = i;
            sphereMesh->m_indices[c++] = i + 1;
            sphereMesh->m_indices[c++] = (numSegments - 2) * numSegments;
        }

        sphereMesh->m_indices[c++] = numSegments - 1;
        sphereMesh->m_indices[c++] = 0;
        sphereMesh->m_indices[c++] = (numSegments - 2) * numSegments;

        for (i = 0; i < numSegments - 1; ++i)
        {
            sphereMesh->m_indices[c++] = (numSegments - 3) * numSegments + i;
            sphereMesh->m_indices[c++] = (numSegments - 3) * numSegments + i + 1;
            sphereMesh->m_indices[c++] = (numSegments - 2) * numSegments + 1;
        }

        sphereMesh->m_indices[c++] = (numSegments - 3) * numSegments + (numSegments - 1);
        sphereMesh->m_indices[c++] = (numSegments - 3) * numSegments;
        sphereMesh->m_indices[c++] = (numSegments - 2) * numSegments + 1;

        return sphereMesh;
    }


    // render the given sphere
    void RenderUtil::RenderSphere(const AZ::Vector3& position, float radius, const MCore::RGBAColor& color)
    {
        // setup the world space matrix of the sphere
        AZ::Transform sphereTransform = AZ::Transform::CreateUniformScale(radius);
        sphereTransform.SetTranslation(position);

        // render the sphere
        RenderSphere(color, sphereTransform);
    }


    // render a circle using the RenderLine and RenderTriangles function
    void RenderUtil::RenderCircle(const AZ::Transform& worldTM, float radius, uint32 numSegments, const MCore::RGBAColor& color, float startAngle, float endAngle, bool fillCircle, const MCore::RGBAColor& fillColor, bool cullFaces, const AZ::Vector3& camRollAxis)
    {
        // if culling is enabled but cam roll axis has not been set return without rendering
        if (cullFaces && camRollAxis == AZ::Vector3::CreateZero())
        {
            return;
        }

        // calculate the angle and step size
        const float angleRange = endAngle - startAngle;

        // if the angle range is too small to see, return directly
        if (angleRange - MCore::Math::epsilon < 0.0f)
        {
            return;
        }

        const float stepSize    = angleRange / (float)(numSegments * (angleRange / MCore::Math::twoPi));

        // render circle segments within the calculated range
        for (float i = startAngle; i < endAngle - stepSize; i += stepSize)
        {
            // calculate the origin relative position
            const float p1 = i;
            const float x1 = radius * MCore::Math::Sin(p1);
            const float y1 = radius * MCore::Math::Cos(p1);
            const float p2 = i + stepSize;
            const float x2 = radius * MCore::Math::Sin(p2);
            const float y2 = radius * MCore::Math::Cos(p2);

            // convert position values into 3D vectors
            AZ::Vector3 pos1(x1, y1, 0.0f);
            AZ::Vector3 pos2(x2, y2, 0.0f);

            // perform world transformation
            pos1 = worldTM.TransformPoint(pos1);
            pos2 = worldTM.TransformPoint(pos2);

            // render line segment
            if (cullFaces == false ||
                MCore::InRange(MCore::Math::ACos((pos2 - worldTM.GetTranslation()).GetNormalized().Dot(camRollAxis)), MCore::Math::halfPi - (MCore::Math::halfPi / 18.0f), MCore::Math::pi))
            {
                RenderLine(pos1, pos2, color);
            }

            if (fillCircle)
            {
                RenderTriangle(worldTM.GetTranslation(), pos2, pos1, fillColor);
            }
        }
    }


    // construct a cube mesh
    RenderUtil::UtilMesh* RenderUtil::CreateCube(float size)
    {
        // set number of triangles and vertices
        const uint32 numVertices    = 8;
        const uint32 numTriangles   = 12;

        // create the mesh
        UtilMesh* mesh = new UtilMesh();
        mesh->Allocate(numVertices, numTriangles * 3, true);

        // define the vertices
        mesh->m_positions[0] = AZ::Vector3(-0.5f, -0.5f, -0.5f) * size;
        mesh->m_positions[1] = AZ::Vector3(0.5f, -0.5f, -0.5f) * size;
        mesh->m_positions[2] = AZ::Vector3(0.5f, 0.5f, -0.5f) * size;
        mesh->m_positions[3] = AZ::Vector3(-0.5f, 0.5f, -0.5f) * size;
        mesh->m_positions[4] = AZ::Vector3(-0.5f, -0.5f, 0.5f) * size;
        mesh->m_positions[5] = AZ::Vector3(0.5f, -0.5f, 0.5f) * size;
        mesh->m_positions[6] = AZ::Vector3(0.5f, 0.5f, 0.5f) * size;
        mesh->m_positions[7] = AZ::Vector3(-0.5f, 0.5f, 0.5f) * size;

        // define the indices
        mesh->m_indices[0]  = 0;
        mesh->m_indices[1]  = 1;
        mesh->m_indices[2]  = 2;

        mesh->m_indices[3]  = 0;
        mesh->m_indices[4]  = 2;
        mesh->m_indices[5]  = 3;

        mesh->m_indices[6]  = 1;
        mesh->m_indices[7]  = 5;
        mesh->m_indices[8]  = 6;

        mesh->m_indices[9]  = 1;
        mesh->m_indices[10] = 6;
        mesh->m_indices[11] = 2;

        mesh->m_indices[12] = 5;
        mesh->m_indices[13] = 4;
        mesh->m_indices[14] = 7;

        mesh->m_indices[15] = 5;
        mesh->m_indices[16] = 7;
        mesh->m_indices[17] = 6;

        mesh->m_indices[18] = 4;
        mesh->m_indices[19] = 0;
        mesh->m_indices[20] = 3;

        mesh->m_indices[21] = 4;
        mesh->m_indices[22] = 3;
        mesh->m_indices[23] = 7;

        mesh->m_indices[24] = 1;
        mesh->m_indices[25] = 0;
        mesh->m_indices[26] = 4;

        mesh->m_indices[27] = 1;
        mesh->m_indices[28] = 4;
        mesh->m_indices[29] = 5;

        mesh->m_indices[30] = 3;
        mesh->m_indices[31] = 2;
        mesh->m_indices[32] = 6;

        mesh->m_indices[33] = 3;
        mesh->m_indices[34] = 6;
        mesh->m_indices[35] = 7;

        // calculate the normals
        mesh->CalculateNormals();

        // return the mesh
        return mesh;
    }


    // construct the arrow head mesh used for rendering
    RenderUtil::UtilMesh* RenderUtil::CreateArrowHead(float height, float radius)
    {
        const uint32 numSegments    = 12;
        const uint32 numTriangles   = numSegments * 2;
        const uint32 numVertices    = numTriangles * 3;

        // construct the arrow head mesh and allocate memory for the vertices, indices and normals
        UtilMesh* mesh = new UtilMesh();
        mesh->Allocate(numVertices, numVertices, true);

        // fill in the indices
        for (uint32 i = 0; i < numVertices; ++i)
        {
            mesh->m_indices[i] = i;
        }

        // fill in the vertices and recalculate the normals
        FillArrowHead(mesh, height, radius, true);

        return mesh;
    }


    // fill the arrow head mesh vertices, indices and recalculate the normals if desired
    void RenderUtil::FillArrowHead(UtilMesh* mesh, float height, float radius, bool calculateNormals)
    {
        static AZ::Vector3      points[12];
        size_t                  pointNr     = 0;
        const size_t            numVertices = mesh->m_positions.size();
        const size_t            numTriangles = numVertices / 3;
        assert(numTriangles * 3 == numVertices);
        const size_t            numSegments = numTriangles / 2;
        assert(numSegments * 2 == numTriangles);
        const size_t            angleStep   = 30;
        assert(360 / angleStep == numSegments);

        // check and prevent the radius being greater than 30% of the height
        if (radius > (height * 0.3f))
        {
            radius = height * 0.3f;
        }

        // construct the segment points
        for (size_t angle = angleStep; angle <= 360; angle += angleStep)
        {
            float theta     = MCore::Math::DegreesToRadians(static_cast<float>(angle));
            float x         = MCore::Math::Cos(theta) * radius;
            float z         = MCore::Math::Sin(theta) * radius;
            points[pointNr] = AZ::Vector3(x, 0.0f, z);
            pointNr++;
        }

        // get some data used for constructing the arrow head mesh
        size_t  vertexNr;
        AZ::Vector3 segmentPoint;
        //AZ::Vector3 center     = AZ::Vector3(0.0f, height * 0.25f, 0.0f);   // real arrow head
        const AZ::Vector3 center           = AZ::Vector3::CreateZero();     // normal cone
        const AZ::Vector3 top              = AZ::Vector3(0.0f, height, 0.0f);
        AZ::Vector3 previousPoint    = points[(numSegments - 1)];

        for (size_t i = 0; i < numSegments; ++i)
        {
            // preprocess data
            segmentPoint    = points[i];
            vertexNr        = i * 6;

            // triangle 1
            mesh->m_positions[vertexNr + 0] = segmentPoint;
            mesh->m_positions[vertexNr + 1] = previousPoint;
            mesh->m_positions[vertexNr + 2] = center;

            // triangle 2
            mesh->m_positions[vertexNr + 3] = previousPoint;
            mesh->m_positions[vertexNr + 4] = segmentPoint;
            mesh->m_positions[vertexNr + 5] = top;

            // postprocess data
            previousPoint = segmentPoint;
        }

        // recalculate the normals if desired
        if (calculateNormals)
        {
            mesh->CalculateNormals();
        }
    }


    // render the given arrow head
    void RenderUtil::RenderArrowHead(float height, float radius, const AZ::Vector3& position, const AZ::Vector3& direction, const MCore::RGBAColor& color)
    {
        AZ::Transform worldTM;

        // rotate the arrow head to the desired direction
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(direction, AZ::Vector3(0.0f, -1.0f, 0.0f), MCore::Math::epsilon) == false)
        {
            worldTM = MCore::GetRotationMatrixAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f).Cross(direction), MCore::Math::ACos(direction.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))));
        }
        else
        {
            worldTM = AZ::Transform::CreateFromQuaternion(MCore::AzEulerAnglesToAzQuat(AZ::Vector3(MCore::Math::DegreesToRadians(180.0f), 0.0f, 0.0f)));
        }

        // translate the arrow head to the given position
        worldTM.SetTranslation(position);

        // render the arrow head
        RenderArrowHead(height, radius, color, worldTM);
    }


    void RenderUtil::RenderArrow(float size, const AZ::Vector3& position, const AZ::Vector3& direction, const MCore::RGBAColor& color)
    {
        const float arrowHeadRadius = size * 0.1f;
        const float arrowHeadHeight = size * 0.3f;
        const float axisCylinderRadius = size * 0.02f;
        const float axisCylinderHeight = size * 0.7f + arrowHeadHeight * 0.25f;

        RenderCylinder(axisCylinderRadius, axisCylinderRadius, axisCylinderHeight, position, direction, color);
        RenderArrowHead(arrowHeadHeight, arrowHeadRadius, position + direction * (axisCylinderHeight - 0.25f * arrowHeadHeight), direction, color);
    }


    // render mesh based axis
    void RenderUtil::RenderAxis(float size, const AZ::Vector3& position, const AZ::Vector3& right, const AZ::Vector3& up, const AZ::Vector3& forward)
    {
        static const MCore::RGBAColor xAxisColor(1.0f, 0.0f, 0.0f);
        static const MCore::RGBAColor yAxisColor(0.0f, 1.0f, 0.0f);
        static const MCore::RGBAColor zAxisColor(0.0f, 0.0f, 1.0f);
        static const MCore::RGBAColor centerColor(0.5f, 0.5f, 0.5f);

        // render zero/center sphere
        RenderSphere(position, size, centerColor);

        RenderArrow(size, position, right, xAxisColor);
        RenderArrow(size, position, up, yAxisColor);
        RenderArrow(size, position, forward, zAxisColor);
    }


    // constructor
    RenderUtil::AxisRenderingSettings::AxisRenderingSettings()
    {
        m_size           = 1.0f;
        m_renderXAxis    = true;
        m_renderYAxis    = true;
        m_renderZAxis    = true;
        m_renderXAxisName = false;
        m_renderYAxisName = false;
        m_renderZAxisName = false;
        m_selected       = false;
    }


    // render line based axis
    void RenderUtil::RenderLineAxis(const AxisRenderingSettings& settings)
    {
        const float             size                = settings.m_size;
        const AZ::Transform&    worldTM             = settings.m_worldTm;
        const AZ::Vector3&      cameraRight         = settings.m_cameraRight;
        const AZ::Vector3&      cameraUp            = settings.m_cameraUp;
        const float             arrowHeadRadius     = size * 0.1f;
        const float             arrowHeadHeight     = size * 0.3f;
        const float             axisHeight          = size * 0.7f;
        const AZ::Vector3       position            = worldTM.GetTranslation();

        if (settings.m_renderXAxis)
        {
            // set the color
            MCore::RGBAColor xAxisColor = MCore::RGBAColor(1.0f, 0.0f, 0.0f);

            MCore::RGBAColor xSelectedColor;
            if (settings.m_selected)
            {
                xSelectedColor = MCore::RGBAColor(1.0f, 0.647f, 0.0f);
            }
            else
            {
                xSelectedColor = xAxisColor;
            }

            const AZ::Vector3       xAxisDir = (worldTM.TransformPoint(AZ::Vector3(size, 0.0f, 0.0f)) - position).GetNormalized();
            const AZ::Vector3 xAxisArrowStart = position + xAxisDir * axisHeight;
            RenderArrowHead(arrowHeadHeight, arrowHeadRadius, xAxisArrowStart, xAxisDir, xSelectedColor);
            RenderLine(position, xAxisArrowStart, xAxisColor);

            if (settings.m_renderXAxisName)
            {
                const AZ::Vector3 xNamePos = position + xAxisDir * (size * 1.15f);
                RenderLine(xNamePos + cameraUp * (-0.15f * size) + cameraRight * (0.1f * size),    xNamePos + cameraUp * (0.15f * size) + cameraRight * (-0.1f * size),    xAxisColor);
                RenderLine(xNamePos + cameraUp * (-0.15f * size) + cameraRight * (-0.1f * size),   xNamePos + cameraUp * (0.15f * size) + cameraRight * (0.1f * size),     xAxisColor);
            }
        }

        if (settings.m_renderYAxis)
        {
            // set the color
            MCore::RGBAColor yAxisColor = MCore::RGBAColor(0.0f, 1.0f, 0.0f);

            MCore::RGBAColor ySelectedColor;
            if (settings.m_selected)
            {
                ySelectedColor = MCore::RGBAColor(1.0f, 0.647f, 0.0f);
            }
            else
            {
                ySelectedColor = yAxisColor;
            }

            const AZ::Vector3       yAxisDir = (worldTM.TransformPoint(AZ::Vector3(0.0f, size, 0.0f)) - position).GetNormalized();
            const AZ::Vector3       yAxisArrowStart = position + yAxisDir * axisHeight;
            RenderArrowHead(arrowHeadHeight, arrowHeadRadius, yAxisArrowStart, yAxisDir, ySelectedColor);
            RenderLine(position, yAxisArrowStart, yAxisColor);

            if (settings.m_renderYAxisName)
            {
                const AZ::Vector3 yNamePos = position + yAxisDir * (size * 1.15f);
                RenderLine(yNamePos,   yNamePos + cameraRight * (-0.1f * size) + cameraUp * (0.15f * size),    yAxisColor);
                RenderLine(yNamePos,   yNamePos + cameraRight * (0.1f * size) + cameraUp * (0.15f * size),     yAxisColor);
                RenderLine(yNamePos,   yNamePos + cameraUp * (-0.15f * size),                              yAxisColor);
            }
        }

        if (settings.m_renderZAxis)
        {
            // set the color
            MCore::RGBAColor zAxisColor = MCore::RGBAColor(0.0f, 0.0f, 1.0f);

            MCore::RGBAColor zSelectedColor;
            if (settings.m_selected)
            {
                zSelectedColor = MCore::RGBAColor(1.0f, 0.647f, 0.0f);
            }
            else
            {
                zSelectedColor = zAxisColor;
            }

            const AZ::Vector3       zAxisDir = (worldTM.TransformPoint(AZ::Vector3(0.0f, 0.0f, size)) - position).GetNormalized();
            const AZ::Vector3       zAxisArrowStart = position + zAxisDir * axisHeight;
            RenderArrowHead(arrowHeadHeight, arrowHeadRadius, zAxisArrowStart, zAxisDir, zSelectedColor);
            RenderLine(position, zAxisArrowStart, zAxisColor);

            if (settings.m_renderZAxisName)
            {
                const AZ::Vector3 zNamePos = position + zAxisDir * (size * 1.15f);
                RenderLine(zNamePos + cameraRight * (-0.1f * size) + cameraUp * (0.15f * size),    zNamePos + cameraRight * (0.1f * size) + cameraUp * (0.15f * size),            zAxisColor);
                RenderLine(zNamePos + cameraRight * (0.1f * size) + cameraUp * (0.15f * size),    zNamePos + cameraRight * (-0.1f * size) + cameraUp * (-0.15f * size),           zAxisColor);
                RenderLine(zNamePos + cameraRight * (-0.1f * size) + cameraUp * (-0.15f * size),   zNamePos + cameraRight * (0.1f * size) + cameraUp * (-0.15f * size),           zAxisColor);
            }
        }
    }


    // calculate the visible area of the grid for the given camera
    void RenderUtil::CalcVisibleGridArea(Camera* camera, uint32 screenWidth, uint32 screenHeight, float unitSize, AZ::Vector2* outGridStart, AZ::Vector2* outGridEnd)
    {
        // render the grid
        AZ::Vector2 gridStart(0.0f, 0.0f);
        AZ::Vector2 gridEnd(0.0f, 0.0f);
        if (camera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            AZ::Matrix4x4 proj = camera->GetProjectionMatrix();
            AZ::Matrix4x4 view = camera->GetViewMatrix();

            AZ::Vector3 a = MCore::UnprojectOrtho(0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight), -1.0f, proj, view);
            AZ::Vector3 b = MCore::UnprojectOrtho(static_cast<float>(screenWidth), static_cast<float>(screenHeight), static_cast<float>(screenWidth), static_cast<float>(screenHeight), 1.0f, proj, view);

            OrthographicCamera* orthoCamera = static_cast<OrthographicCamera*>(camera);
            switch (orthoCamera->GetMode())
            {
            case OrthographicCamera::VIEWMODE_FRONT:
            {
                gridStart.SetX(MCore::Min(a.GetX(), b.GetX()) - unitSize);
                gridStart.SetY(MCore::Min(a.GetZ(), b.GetZ()) - unitSize);
                gridEnd.SetX(MCore::Max(a.GetX(), b.GetX()) + unitSize);
                gridEnd.SetY(MCore::Max(a.GetZ(), b.GetZ()) + unitSize);
                break;
            }
            case OrthographicCamera::VIEWMODE_BACK:
            {
                a = AZ::Vector3(a.GetX(), -a.GetY(), a.GetZ());
                b = AZ::Vector3(b.GetX(), -b.GetY(), b.GetZ());
                gridStart.SetX(MCore::Min(a.GetX(), b.GetX()) - unitSize);
                gridStart.SetY(MCore::Min(a.GetZ(), b.GetZ()) - unitSize);
                gridEnd.SetX(MCore::Max(a.GetX(), b.GetX()) + unitSize);
                gridEnd.SetY(MCore::Max(a.GetZ(), b.GetZ()) + unitSize);
                break;
            }
            case OrthographicCamera::VIEWMODE_LEFT:
            {
                a = AZ::Vector3(a.GetX(), -a.GetY(), a.GetZ());
                b = AZ::Vector3(b.GetX(), -b.GetY(), b.GetZ());
                gridStart.SetX(MCore::Min(a.GetY(), b.GetY()) - unitSize);
                gridStart.SetY(MCore::Min(a.GetZ(), b.GetZ()) - unitSize);
                gridEnd.SetX(MCore::Max(a.GetY(), b.GetY()) + unitSize);
                gridEnd.SetY(MCore::Max(a.GetZ(), b.GetZ()) + unitSize);
                break;
            }
            case OrthographicCamera::VIEWMODE_RIGHT:
            {
                gridStart.SetX(MCore::Min(a.GetY(), b.GetY()) - unitSize);
                gridStart.SetY(MCore::Min(a.GetZ(), b.GetZ()) - unitSize);
                gridEnd.SetX(MCore::Max(a.GetY(), b.GetY()) + unitSize);
                gridEnd.SetY(MCore::Max(a.GetZ(), b.GetZ()) + unitSize);
                break;
            }
            case OrthographicCamera::VIEWMODE_TOP:
            {
                gridStart.SetX(MCore::Min(a.GetX(), b.GetX()) - unitSize);
                gridStart.SetY(MCore::Min(a.GetY(), b.GetY()) - unitSize);
                gridEnd.SetX(MCore::Max(a.GetX(), b.GetX()) + unitSize);
                gridEnd.SetY(MCore::Max(a.GetY(), b.GetY()) + unitSize);
                break;
            }
            case OrthographicCamera::VIEWMODE_BOTTOM:
            {
                a = AZ::Vector3(a.GetX(), -a.GetY(), a.GetZ());
                b = AZ::Vector3(b.GetX(), -b.GetY(), b.GetZ());
                gridStart.SetX(MCore::Min(a.GetX(), b.GetX()) - unitSize);
                gridStart.SetY(MCore::Min(a.GetY(), b.GetY()) - unitSize);
                gridEnd.SetX(MCore::Max(a.GetX(), b.GetX()) + unitSize);
                gridEnd.SetY(MCore::Max(a.GetY(), b.GetY()) + unitSize);
                break;
            }
            }
        }
        else
        {
            const float cameraScreenWidth     = static_cast<float>(camera->GetScreenWidth());
            const float cameraScreenHeight    = static_cast<float>(camera->GetScreenHeight());

            // find the 4 corners of the frustum
            AZ::Vector3 corners[4];
            const AZ::Matrix4x4 inversedProjectionMatrix = MCore::InvertProjectionMatrix(camera->GetProjectionMatrix());
            const AZ::Matrix4x4 inversedViewMatrix = MCore::InvertProjectionMatrix(camera->GetViewMatrix());

            corners[0] = MCore::Unproject(0.0f, 0.0f, cameraScreenWidth, cameraScreenHeight, camera->GetFarClipDistance(), inversedProjectionMatrix, inversedViewMatrix);
            corners[1] = MCore::Unproject(cameraScreenWidth, 0.0f, cameraScreenWidth, cameraScreenHeight, camera->GetFarClipDistance(), inversedProjectionMatrix, inversedViewMatrix);
            corners[2] = MCore::Unproject(cameraScreenWidth, cameraScreenHeight, cameraScreenWidth, cameraScreenHeight, camera->GetFarClipDistance(), inversedProjectionMatrix, inversedViewMatrix);
            corners[3] = MCore::Unproject(0.0f, cameraScreenHeight, cameraScreenWidth, cameraScreenHeight, camera->GetFarClipDistance(), inversedProjectionMatrix, inversedViewMatrix);

            // calculate the intersection points with the ground plane and create an AABB around those
            // if there is no intersection point then use the ray target as point, which is the projection onto the far plane basically
            AZ::Aabb aabb = AZ::Aabb::CreateNull();
            AZ::Vector3 intersectionPoint;
            const AZ::Plane groundPlane = AZ::Plane::CreateFromNormalAndPoint(AZ::Vector3(0.0f, 0.0f, 1.0f), AZ::Vector3::CreateZero());
            for (AZ::u32 i = 0; i < 4; ++i)
            {
                if (groundPlane.IntersectSegment(camera->GetPosition(), corners[i], intersectionPoint))
                {
                    corners[i] = intersectionPoint;
                }

                aabb.AddPoint(corners[i]);
            }

            // set the grid start and end values
            gridStart.SetX(aabb.GetMin().GetX() - unitSize);
            gridStart.SetY(aabb.GetMin().GetY() - unitSize);
            gridEnd.SetX(aabb.GetMax().GetX() + unitSize);
            gridEnd.SetY(aabb.GetMax().GetY() + unitSize);
        }

        *outGridStart = gridStart;
        *outGridEnd = gridEnd;
    }


    // get aabb which includes all actor instances
    AZ::Aabb RenderUtil::CalcSceneAabb()
    {
        AZ::Aabb finalAABB = AZ::Aabb::CreateNull();

        // get the number of actor instances and iterate through them
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            // get the actor instance and update its transformations and meshes
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);

            if (actorInstance->GetIsOwnedByRuntime())
            {
                continue;
            }

            actorInstance->UpdateTransformations(0.0f, true);
            actorInstance->UpdateMeshDeformers(0.0f);

            // get the mesh based bounding box
            AZ::Aabb boundingBox;
            actorInstance->CalcMeshBasedAabb(actorInstance->GetLODLevel(), &boundingBox);

            // in case there aren't any meshes, use the node based bounding box
            if (!boundingBox.IsValid())
            {
                actorInstance->CalcNodeBasedAabb(&boundingBox);
            }

            // make sure the actor instance is covered in our world bounding box
            finalAABB.AddAabb(boundingBox);
        }

        return finalAABB;
    }


    // visualize the trajectory
    void RenderUtil::RenderTrajectory(const AZ::Transform& worldTM, const MCore::RGBAColor& innerColor, const MCore::RGBAColor& borderColor, float scale)
    {
        // get the position and some direction vectors of the trajectory node matrix
        const AZ::Vector3 center        = worldTM.GetTranslation();
        const AZ::Vector3 forward       = MCore::GetRight(worldTM).GetNormalized();
        const AZ::Vector3 right         = MCore::GetForward(worldTM).GetNormalized();
        const float trailWidthHalf      = 0.5f;
        const float trailLengh          = 2.0f;
        const float arrowWidthHalf      = 1.5f;
        const float arrowLength         = 2.0f;

        AZ::Vector3 vertices[7];
        /*
            //              4
            //             / \
            //            /   \
            //          /       \
            //        /           \
            //      /               \
            //    5-----6       2-----3
            //          |       |
            //          |       |
            //          |       |
            //          |       |
            //          |       |
            //         0---------1
        */
        // construct the arrow vertices
        vertices[0] = center + AZ::Vector3(-right * trailWidthHalf - forward * trailLengh) * scale;
        vertices[1] = center + AZ::Vector3(right * trailWidthHalf - forward * trailLengh) * scale;
        vertices[2] = center + AZ::Vector3(right * trailWidthHalf) * scale;
        vertices[3] = center + AZ::Vector3(right * arrowWidthHalf) * scale;
        vertices[4] = center + AZ::Vector3(forward * arrowLength) * scale;
        vertices[5] = center + AZ::Vector3(-right * arrowWidthHalf) * scale;
        vertices[6] = center + AZ::Vector3(-right * trailWidthHalf) * scale;

        // render the solid arrow
        RenderTriangle(vertices[0], vertices[1], vertices[2], innerColor);
        RenderTriangle(vertices[2], vertices[6], vertices[0], innerColor);
        RenderTriangle(vertices[3], vertices[4], vertices[2], innerColor);
        RenderTriangle(vertices[2], vertices[4], vertices[6], innerColor);
        RenderTriangle(vertices[6], vertices[4], vertices[5], innerColor);

        // render the border
        RenderLine(vertices[0], vertices[1], borderColor);
        RenderLine(vertices[1], vertices[2], borderColor);
        RenderLine(vertices[2], vertices[3], borderColor);
        RenderLine(vertices[3], vertices[4], borderColor);
        RenderLine(vertices[4], vertices[5], borderColor);
        RenderLine(vertices[5], vertices[6], borderColor);
        RenderLine(vertices[6], vertices[0], borderColor);
    }


    // visualize the trajectory
    void RenderUtil::RenderTrajectory(EMotionFX::ActorInstance* actorInstance, const MCore::RGBAColor& innerColor, const MCore::RGBAColor& borderColor, float scale)
    {
        EMotionFX::Actor*   actor     = actorInstance->GetActor();
        const size_t        nodeIndex = actor->GetMotionExtractionNodeIndex();

        // in case the motion extraction node is not set, return directly
        if (nodeIndex == InvalidIndex)
        {
            return;
        }

        // get the world TM for the trajectory
        EMotionFX::Transform transform = actorInstance->GetTransformData()->GetCurrentPose()->GetWorldSpaceTransform(nodeIndex).ProjectedToGroundPlane();
        AZ::Transform worldTM = transform.ToAZTransform();

        // pass it down to the real rendering function
        RenderTrajectory(worldTM, innerColor, borderColor, scale);
    }


    // render the trajectory trace particles as one big curved arrow
    void RenderUtil::RenderTrajectoryPath(TrajectoryTracePath* trajectoryPath, const MCore::RGBAColor& innerColor, float scale)
    {
        // make sure the incoming trajectory trace path is valid
        if (trajectoryPath == NULL)
        {
            return;
        }

        // get some helper variables and check if there is a motion extraction node set
        EMotionFX::ActorInstance*   actorInstance   = trajectoryPath->m_actorInstance;
        EMotionFX::Actor*           actor           = actorInstance->GetActor();
        EMotionFX::Node*            extractionNode  = actor->GetMotionExtractionNode();
        if (extractionNode == NULL)
        {
            return;
        }

        // fast access to the trajectory trace particles
        const AZStd::vector<MCommon::RenderUtil::TrajectoryPathParticle>& traceParticles = trajectoryPath->m_traceParticles;
        const size_t numTraceParticles = traceParticles.size();
        if (traceParticles.empty())
        {
            return;
        }

        const float trailWidthHalf          = 0.25f;
        const float trailLength             = 2.0f;
        const float arrowWidthHalf          = 0.75f;
        const float arrowLength             = 1.5f;
        const AZ::Vector3 liftFromGround(0.0f, 0.0f, 0.0001f);

        const AZ::Transform trajectoryWorldTM = actorInstance->GetWorldSpaceTransform().ToAZTransform();

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // Render arrow head
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // get the position and some direction vectors of the trajectory node matrix
        EMotionFX::Transform worldTM = traceParticles[numTraceParticles - 1].m_worldTm;
        AZ::Vector3 right = MCore::GetRight(trajectoryWorldTM).GetNormalized();
        AZ::Vector3 center = trajectoryWorldTM.GetTranslation();
        AZ::Vector3 forward = MCore::GetForward(trajectoryWorldTM).GetNormalized();
        AZ::Vector3 up(0.0f, 0.0f, 1.0f);

        AZ::Vector3 vertices[7];
        AZ::Vector3 oldLeft, oldRight;

        /*
            //              4
            //             / \
            //            /   \
            //          /       \
            //        /           \
            //      /               \
            //    5-----6       2-----3
            //          |       |
            //          |       |
            //          |       |
            //          |       |
            //          |       |
            //          0-------1
        */
        // construct the arrow vertices
        vertices[0] = center + (-right * trailWidthHalf - forward * trailLength) * scale;
        vertices[1] = center + (right * trailWidthHalf - forward * trailLength) * scale;
        vertices[2] = center + (right * trailWidthHalf) * scale;
        vertices[3] = center + (right * arrowWidthHalf) * scale;
        vertices[4] = center + (forward * arrowLength) * scale;
        vertices[5] = center + (-right * arrowWidthHalf) * scale;
        vertices[6] = center + (-right * trailWidthHalf) * scale;

        oldLeft     = vertices[6];
        oldRight    = vertices[2];

        AZ::Vector3 arrowOldLeft = oldLeft;
        AZ::Vector3 arrowOldRight = oldRight;

        // render the solid arrow
        MCore::RGBAColor arrowColor = innerColor * 1.2f;
        arrowColor.Clamp();
        RenderTriangle(vertices[3] + liftFromGround, vertices[4] + liftFromGround, vertices[2] + liftFromGround, arrowColor);
        RenderTriangle(vertices[2] + liftFromGround, vertices[4] + liftFromGround, vertices[6] + liftFromGround, arrowColor);
        RenderTriangle(vertices[6] + liftFromGround, vertices[4] + liftFromGround, vertices[5] + liftFromGround, arrowColor);

        //////////////////////////////////////////////////////////////////////////////////////////////////////
        // Render arrow tail (actual path)
        //////////////////////////////////////////////////////////////////////////////////////////////////////
        AZ::Vector3 a, b;
        MCore::RGBAColor color = innerColor;

        // render the path from the arrow head towards the tail
        for (size_t i = numTraceParticles - 1; i > 0; i--)
        {
            // calculate the normalized distance to the head, this value also represents the alpha value as it fades away while getting closer to the end
            float normalizedDistance = (float)i / numTraceParticles;

            // get the start and end point of the line segment and calculate the delta between them
            worldTM     = traceParticles[i].m_worldTm;
            a           = worldTM.m_position;
            b           = traceParticles[i - 1].m_worldTm.m_position;
            right       = MCore::GetRight(worldTM.ToAZTransform()).GetNormalized();

            if (i > 1 && i < numTraceParticles - 3)
            {
                const AZ::Vector3 deltaA = traceParticles[i - 2].m_worldTm.m_position - traceParticles[i - 1].m_worldTm.m_position;
                const AZ::Vector3 deltaB = traceParticles[i - 1].m_worldTm.m_position - traceParticles[i    ].m_worldTm.m_position;
                const AZ::Vector3 deltaC = traceParticles[i    ].m_worldTm.m_position - traceParticles[i + 1].m_worldTm.m_position;
                const AZ::Vector3 deltaD = traceParticles[i + 1].m_worldTm.m_position - traceParticles[i + 2].m_worldTm.m_position;
                AZ::Vector3 delta = deltaA + deltaB + deltaC + deltaD;
                delta = MCore::SafeNormalize(delta);

                right = up.Cross(delta);
            }

            /*
                    //              .
                    //              .
                    //              .
                    //(oldLeft) 0   a   1 (oldRight)
                    //          |       |
                    //          |       |
                    //          |       |
                    //          |       |
                    //          |       |
                    //          2---b---3
            */

            // construct the arrow vertices
            vertices[0] = oldLeft;
            vertices[1] = oldRight;
            vertices[2] = b + AZ::Vector3(-right * trailWidthHalf) * scale;
            vertices[3] = b + AZ::Vector3(right * trailWidthHalf) * scale;

            // make sure we perfectly align with the arrow head
            if (i == numTraceParticles - 1)
            {
                normalizedDistance = 1.0f;
                vertices[0] = arrowOldLeft;
                vertices[1] = arrowOldRight;
            }

            // render the solid arrow
            color.m_a = normalizedDistance;
            RenderTriangle(vertices[0] + liftFromGround, vertices[2] + liftFromGround, vertices[1] + liftFromGround, color);
            RenderTriangle(vertices[1] + liftFromGround, vertices[2] + liftFromGround, vertices[3] + liftFromGround, color);

            // overwrite the old left and right values so that they can be used for the next trace particle
            oldLeft  = vertices[2];
            oldRight = vertices[3];
        }

        // make sure we render all lines within one call and with the correct render flags set
        RenderLines();
    }


    // reset the trajectory path
    void RenderUtil::ResetTrajectoryPath(TrajectoryTracePath* trajectoryPath)
    {
        // make sure the incoming trajectory trace path is valid
        if (trajectoryPath == NULL)
        {
            return;
        }

        // remove all particles while keeping the data in memory
        trajectoryPath->m_traceParticles.clear();
    }


    // render the name of the given node at the node position
    void RenderUtil::RenderText(const char* text, uint32 textSize, const AZ::Vector3& globalPos, MCommon::Camera* camera, uint32 screenWidth, uint32 screenHeight, const MCore::RGBAColor& color)
    {
        // project the node world space position to the screen space
        const AZ::Vector3 projectedPoint = MCore::Project(globalPos, camera->GetViewProjMatrix(), screenWidth, screenHeight);

        // perform clipping and make sure the node is actually visible, if not skip rendering its name
        if (projectedPoint.GetX() < 0.0f || projectedPoint.GetX() > screenWidth || projectedPoint.GetY() < 0.0f || projectedPoint.GetY() > screenHeight)
        {
            return;
        }

        if (camera->GetType() != MCommon::OrthographicCamera::TYPE_ID && projectedPoint.GetZ() < 0.0f)
        {
            return;
        }

        // render the text
        RenderText(projectedPoint.GetX(), projectedPoint.GetY(), text, color, static_cast<float>(textSize), true);
    }


    // render node names for all enabled nodes
    void RenderUtil::RenderNodeNames(EMotionFX::ActorInstance* actorInstance, Camera* camera, uint32 screenWidth, uint32 screenHeight, const MCore::RGBAColor& color, const MCore::RGBAColor& selectedColor, const AZStd::unordered_set<size_t>& visibleJointIndices, const AZStd::unordered_set<size_t>& selectedJointIndices)
    {
        const EMotionFX::Actor* actor = actorInstance->GetActor();
        const EMotionFX::Skeleton* skeleton = actor->GetSkeleton();
        const EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
        const EMotionFX::Pose* pose = transformData->GetCurrentPose();
        const size_t numEnabledNodes = actorInstance->GetNumEnabledNodes();

        for (size_t i = 0; i < numEnabledNodes; ++i)
        {
            const EMotionFX::Node* joint = skeleton->GetNode(actorInstance->GetEnabledNode(i));
            const size_t jointIndex = joint->GetNodeIndex();
            const AZ::Vector3 worldPos = pose->GetWorldSpaceTransform(jointIndex).m_position;

            // check if the current enabled node is along the visible nodes and render it if that is the case
            if (visibleJointIndices.empty() ||
                (visibleJointIndices.find(jointIndex) != visibleJointIndices.end()))
            {
                MCore::RGBAColor finalColor;
                if (selectedJointIndices.find(jointIndex) != selectedJointIndices.end())
                {
                    finalColor = selectedColor;
                }
                else
                {
                    finalColor = color;
                }

                RenderText(joint->GetName(), 11, worldPos, camera, screenWidth, screenHeight, finalColor);
            }
        }
    }


    void RenderUtil::RenderWireframeBox(const AZ::Vector3& dimensions, const AZ::Transform& worldTM, const MCore::RGBAColor& color, bool directlyRender)
    {
        AZ::Vector3 min = AZ::Vector3(-dimensions.GetX() * 0.5f, -dimensions.GetY() * 0.5f, -dimensions.GetZ() * 0.5f);
        AZ::Vector3 max = AZ::Vector3(dimensions.GetX() * 0.5f,  dimensions.GetY() * 0.5f,  dimensions.GetZ() * 0.5f);

        AZ::Vector3 p[8];
        p[0].Set(min.GetX(), min.GetY(), min.GetZ());
        p[1].Set(max.GetX(), min.GetY(), min.GetZ());
        p[2].Set(max.GetX(), min.GetY(), max.GetZ());
        p[3].Set(min.GetX(), min.GetY(), max.GetZ());
        p[4].Set(min.GetX(), max.GetY(), min.GetZ());
        p[5].Set(max.GetX(), max.GetY(), min.GetZ());
        p[6].Set(max.GetX(), max.GetY(), max.GetZ());
        p[7].Set(min.GetX(), max.GetY(), max.GetZ());

        for (int i = 0; i < 8; ++i)
        {
            p[i] = worldTM.TransformPoint(p[i]);
        }

        RenderLine(p[0], p[1], color);
        RenderLine(p[1], p[2], color);
        RenderLine(p[2], p[3], color);
        RenderLine(p[3], p[0], color);

        RenderLine(p[4], p[5], color);
        RenderLine(p[5], p[6], color);
        RenderLine(p[6], p[7], color);
        RenderLine(p[7], p[4], color);

        RenderLine(p[0], p[4], color);
        RenderLine(p[1], p[5], color);
        RenderLine(p[2], p[6], color);
        RenderLine(p[3], p[7], color);

        if (directlyRender)
        {
            RenderLines();
        }
    }


    void RenderUtil::RenderWireframeSphere(float radius, const AZ::Transform& worldTM, const MCore::RGBAColor& color, bool directlyRender)
    {
        const float stepSize = AZ::Constants::TwoPi / s_wireframeSphereSegmentCount;

        AZ::Vector3 pos1, pos2;
        float x1, y1, x2, y2;
        const float endAngle = AZ::Constants::TwoPi + std::numeric_limits<float>::epsilon();
        for (float i = 0.0f; i < endAngle; i += stepSize)
        {
            x1 = radius * cosf(i);
            y1 = radius * sinf(i);
            x2 = radius * cosf(i + stepSize);
            y2 = radius * sinf(i + stepSize);

            pos1 = worldTM.TransformPoint(AZ::Vector3(x1, y1, 0.0f));
            pos2 = worldTM.TransformPoint(AZ::Vector3(x2, y2, 0.0f));
            RenderLine(pos1, pos2, color);

            pos1 = worldTM.TransformPoint(AZ::Vector3(x1, 0.0f, y1));
            pos2 = worldTM.TransformPoint(AZ::Vector3(x2, 0.0f, y2));
            RenderLine(pos1, pos2, color);

            pos1 = worldTM.TransformPoint(AZ::Vector3(0.0f, x1, y1));
            pos2 = worldTM.TransformPoint(AZ::Vector3(0.0f, x2, y2));
            RenderLine(pos1, pos2, color);
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }


    // The capsule caps (for one aligned vertically) are rendered as one horizontal full circle (around y) and two vertically aligned half circles around the x and z axes.
    // The end points of these half circles connect the bottom cap to the top cap (the cylinder part in the middle).
    void RenderUtil::RenderWireframeCapsule(float radius, float height, const AZ::Transform& worldTM, const MCore::RGBAColor& color, bool directlyRender)
    {
        float stepSize = AZ::Constants::TwoPi / s_wireframeSphereSegmentCount;
        const float cylinderHeight = height - 2.0f * radius;
        const float halfCylinderHeight = cylinderHeight * 0.5f;

        AZ::Vector3 pos1, pos2;
        float x1, y1, x2, y2;

        // Draw the full circles for both caps
        float startAngle = 0.0f;
        float endAngle = AZ::Constants::TwoPi + std::numeric_limits<float>::epsilon();
        for (float i = startAngle; i < endAngle; i += stepSize)
        {
            x1 = radius * cosf(i);
            y1 = radius * sinf(i);
            x2 = radius * cosf(i + stepSize);
            y2 = radius * sinf(i + stepSize);

            pos1 = worldTM.TransformPoint(AZ::Vector3(x1, y1, halfCylinderHeight));
            pos2 = worldTM.TransformPoint(AZ::Vector3(x2, y2,  halfCylinderHeight));
            RenderLine(pos1, pos2, color);

            pos2 = worldTM.TransformPoint(AZ::Vector3(x2, y2, -halfCylinderHeight));
            pos1 = worldTM.TransformPoint(AZ::Vector3(x1, y1, -halfCylinderHeight));
            RenderLine(pos1, pos2, color);
        }

        // Draw half circles for caps
        startAngle = 0.0f;
        endAngle = AZ::Constants::Pi - std::numeric_limits<float>::epsilon();
        for (float i = startAngle; i < endAngle; i += stepSize)
        {
            x1 = radius * cosf(i);
            y1 = radius * sinf(i);
            x2 = radius * cosf(i + stepSize);
            y2 = radius * sinf(i + stepSize);

            // Upper cap
            pos1 = worldTM.TransformPoint(AZ::Vector3(x1, 0.0f, y1 + halfCylinderHeight));
            pos2 = worldTM.TransformPoint(AZ::Vector3(x2, 0.0f, y2 + halfCylinderHeight));
            RenderLine(pos1, pos2, color);

            pos1 = worldTM.TransformPoint(AZ::Vector3(0.0f, x1, y1 + halfCylinderHeight));
            pos2 = worldTM.TransformPoint(AZ::Vector3(0.0f, x2, y2 + halfCylinderHeight));
            RenderLine(pos1, pos2, color);

            // Lower cap
            pos1 = worldTM.TransformPoint(AZ::Vector3(x1, 0.0f, -y1 - halfCylinderHeight));
            pos2 = worldTM.TransformPoint(AZ::Vector3(x2, 0.0f, -y2 - halfCylinderHeight));
            RenderLine(pos1, pos2, color);

            pos1 = worldTM.TransformPoint(AZ::Vector3(0.0f, x1, -y1 - halfCylinderHeight));
            pos2 = worldTM.TransformPoint(AZ::Vector3(0.0f, x2, -y2 - halfCylinderHeight));
            RenderLine(pos1, pos2, color);
        }

        // Draw cap connectors (cylinder height)
        startAngle = 0.0f;
        endAngle = AZ::Constants::TwoPi + std::numeric_limits<float>::epsilon();
        stepSize = AZ::Constants::Pi* 0.5f;
        for (float i = startAngle; i < endAngle; i += stepSize)
        {
            x1 = radius * cosf(i);
            y1 = radius * sinf(i);

            pos1 = worldTM.TransformPoint(AZ::Vector3(x1, y1,  halfCylinderHeight));
            pos2 = worldTM.TransformPoint(AZ::Vector3(x1, y1, -halfCylinderHeight));
            RenderLine(pos1, pos2, color);
        }

        if (directlyRender)
        {
            RenderLines();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Vector Font
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //   font info:
    //
    //Peter Holzmann, Octopus Enterprises
    //USPS: 19611 La Mar Court, Cupertino, CA 95014
    //UUCP: {hplabs!hpdsd,pyramid}!octopus!pete
    //Phone: 408/996-7746
    //
    //This distribution is made possible through the collective encouragement
    //of the Usenet Font Consortium, a mailing list that sprang to life to get
    //this accomplished and that will now most likely disappear into the mists
    //of time... Thanks are especially due to Jim Hurt, who provided the packed
    //font data for the distribution, along with a lot of other help.
    //
    //This file describes the Hershey Fonts in general, along with a description of
    //the other files in this distribution and a simple re-distribution restriction.
    //
    //USE RESTRICTION:
    //        This distribution of the Hershey Fonts may be used by anyone for
    //        any purpose, commercial or otherwise, providing that:
    //                1. The following acknowledgements must be distributed with
    //                        the font data:
    //                        - The Hershey Fonts were originally created by Dr.
    //                                A. V. Hershey while working at the U. S.
    //                                National Bureau of Standards.
    //                        - The format of the Font data in this distribution
    //                                was originally created by
    //                                        James Hurt
    //                                        Cognition, Inc.
    //                                        900 Technology Park Drive
    //                                        Billerica, MA 01821
    //                                        (mit-eddie!ci-dandelion!hurt)
    //                2. The font data in this distribution may be converted into
    //                        any other format *EXCEPT* the format distributed by
    //                        the U.S. NTIS (which organization holds the rights
    //                        to the distribution and use of the font data in that
    //                        particular format). Not that anybody would really
    //                        *want* to use their format... each point is described
    //                        in eight bytes as "xxx yyy:", where xxx and yyy are
    //                        the coordinate values as ASCII numbers.

#define FONT_VERSION 1

    unsigned char gFontData[6350] = {
        0x46, 0x4F, 0x4E, 0x54, 0x01, 0x00, 0x00, 0x00, 0x43, 0x01, 0x00, 0x00, 0x5E, 0x00, 0x00, 0x00, 0xC4, 0x06, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3C, 0x3D, 0x0A, 0x57, 0x3E, 0x0A, 0xD7, 0x23, 0x3C,
        0x28, 0x5C, 0x8F, 0x3D, 0x0A, 0xD7, 0x23, 0x3C, 0x08, 0xD7, 0xA3, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x08, 0xD7, 0x23, 0x3C, 0x0A, 0xD7, 0x23, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0xA3, 0x3C,
        0x08, 0xD7, 0x23, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x3D, 0x0A, 0x57, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x28, 0x5C, 0x0F, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0x3D, 0x0A, 0x57, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D,
        0x28, 0x5C, 0x0F, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0x00, 0x00, 0x80, 0x3E, 0x0C, 0xD7, 0x23, 0x3C, 0x29, 0x5C, 0x8F, 0xBD, 0x29, 0x5C, 0x0F, 0x3E, 0x00, 0x00, 0x80, 0x3E, 0x29, 0x5C, 0x8F, 0x3D,
        0x29, 0x5C, 0x8F, 0xBD, 0x0C, 0xD7, 0x23, 0x3C, 0x8F, 0xC2, 0xF5, 0x3D, 0x9A, 0x99, 0x19, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x8E, 0xC2, 0x75, 0x3D, 0x29, 0x5C, 0x0F, 0x3E,
        0x8E, 0xC2, 0x75, 0x3D, 0xCD, 0xCC, 0x4C, 0x3D, 0x00, 0x00, 0x80, 0x3E, 0xCD, 0xCC, 0x4C, 0x3D, 0x0A, 0xD7, 0x23, 0xBD, 0xEC, 0x51, 0xB8, 0x3D, 0x00, 0x00, 0x80, 0x3E, 0xEC, 0x51, 0xB8, 0x3D,
        0x0A, 0xD7, 0x23, 0xBD, 0x29, 0x5C, 0x0F, 0x3E, 0xEB, 0x51, 0x38, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0xCC, 0xCC, 0x4C, 0x3E, 0xEC, 0x51, 0xB8, 0x3D, 0x3D, 0x0A, 0x57, 0x3E, 0xCD, 0xCC, 0x4C, 0x3D,
        0x3D, 0x0A, 0x57, 0x3E, 0x0C, 0xD7, 0xA3, 0x3C, 0xCC, 0xCC, 0x4C, 0x3E, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x51, 0x38, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3E, 0x0C, 0xD7, 0x23, 0x3C,
        0x28, 0x5C, 0x0F, 0x3E, 0x0C, 0xD7, 0xA3, 0x3C, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0xCD, 0xCC, 0xCC, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D,
        0xEB, 0x51, 0xB8, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0x8E, 0xC2, 0xF5, 0x3C, 0x8F, 0xC2, 0xF5, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0xEC, 0x51, 0xB8, 0x3D,
        0x00, 0x00, 0x00, 0x00, 0xCD, 0xCC, 0x4C, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x8E, 0xC2, 0xF5, 0x3C, 0xEB, 0x51, 0x38, 0x3E, 0x3D, 0x0A, 0x57, 0x3E, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x28, 0x5C, 0x8F, 0x3D, 0x5C, 0x8F, 0x42, 0x3E, 0x28, 0x5C, 0x8F, 0x3D, 0x7A, 0x14, 0x2E, 0x3E, 0x8E, 0xC2, 0x75, 0x3D, 0x99, 0x99, 0x19, 0x3E, 0x0A, 0xD7, 0x23, 0x3D,
        0x28, 0x5C, 0x0F, 0x3E, 0x08, 0xD7, 0xA3, 0x3C, 0x28, 0x5C, 0x0F, 0x3E, 0x08, 0xD7, 0x23, 0x3C, 0xCC, 0xCC, 0x4C, 0x3E, 0x8E, 0xC2, 0xF5, 0x3C, 0x3D, 0x0A, 0x57, 0x3E, 0x28, 0x5C, 0x8F, 0x3D,
        0xCC, 0xCC, 0x4C, 0x3E, 0xCC, 0xCC, 0xCC, 0x3D, 0x5C, 0x8F, 0x42, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x5C, 0x8F, 0x42, 0x3E, 0x0A, 0xD7, 0x23, 0x3E, 0xCC, 0xCC, 0x4C, 0x3E, 0x28, 0x5C, 0x0F, 0x3E,
        0x28, 0x5C, 0x8F, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0x8E, 0xC2, 0x75, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x0A, 0xD7, 0x23, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x08, 0xD7, 0xA3, 0x3C, 0xB8, 0x1E, 0x05, 0x3E,
        0x00, 0x00, 0x00, 0x00, 0x99, 0x99, 0x19, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x14, 0x2E, 0x3E, 0x08, 0xD7, 0x23, 0x3C, 0xEB, 0x51, 0x38, 0x3E, 0x8E, 0xC2, 0xF5, 0x3C, 0xEB, 0x51, 0x38, 0x3E,
        0xCC, 0xCC, 0x4C, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0x28, 0x5C, 0x8F, 0x3D, 0xCC, 0xCC, 0x4C, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0xCC, 0xCC, 0x4C, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x5C, 0x8F, 0x42, 0x3E,
        0x28, 0x5C, 0x0F, 0x3E, 0xEB, 0x51, 0x38, 0x3E, 0x28, 0x5C, 0x0F, 0x3E, 0x7A, 0x14, 0x2E, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0x23, 0x3E, 0xAE, 0x47, 0xE1, 0x3D, 0x8E, 0xC2, 0xF5, 0x3D,
        0x8E, 0xC2, 0xF5, 0x3C, 0xCC, 0xCC, 0xCC, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0x0A, 0xD7, 0xA3, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x09, 0xD7, 0x23, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x0A, 0xD7, 0x23, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0x0A, 0xD7, 0xA3, 0x3D, 0x08, 0xD7, 0xA3, 0x3C, 0xEB, 0x51, 0xB8, 0x3D, 0xEB, 0x51, 0xB8, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0xCC, 0xCC, 0xCC, 0x3D,
        0x28, 0x5C, 0x0F, 0x3E, 0xAD, 0x47, 0xE1, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0xAD, 0x47, 0xE1, 0x3D, 0xEB, 0x51, 0x38, 0x3E, 0xCC, 0xCC, 0xCC, 0x3D, 0xCC, 0xCC, 0x4C, 0x3E, 0x8E, 0xC2, 0x75, 0x3D,
        0xCC, 0xCC, 0x4C, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0xEB, 0x51, 0x38, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0x8E, 0xC2, 0x75, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D,
        0xCC, 0xCC, 0xCC, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x8E, 0xC2, 0xF5, 0x3C, 0x99, 0x99, 0x19, 0x3E, 0x08, 0xD7, 0x23, 0x3C, 0x7A, 0x14, 0x2E, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3C,
        0x5C, 0x8F, 0x42, 0x3E, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0x4C, 0x3E, 0x0A, 0xD7, 0xA3, 0x3C, 0xEB, 0x51, 0x38, 0x3E, 0x0A, 0xD7, 0x23, 0x3C, 0x0A, 0xD7, 0x23, 0x3E, 0x00, 0x00, 0x00, 0x00,
        0x99, 0x99, 0x19, 0x3E, 0x29, 0x5C, 0x8F, 0x3D, 0x00, 0x00, 0x80, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0x1E, 0x85, 0x6B, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C, 0xCC, 0xCC, 0x4C, 0x3E, 0x00, 0x00, 0x00, 0x00,
        0xAE, 0x47, 0xE1, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x28, 0x5C, 0x8F, 0x3D, 0x8F, 0xC2, 0xF5, 0x3C, 0x0C, 0xD7, 0xA3, 0xBC, 0xCC, 0xCC, 0x4C, 0x3D, 0xCE, 0xCC, 0x4C, 0xBD, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x80, 0x3E, 0x0A, 0xD7, 0xA3, 0x3C, 0x1E, 0x85, 0x6B, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0xCC, 0xCC, 0x4C, 0x3E, 0x8F, 0xC2, 0x75, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0x29, 0x5C, 0x8F, 0x3D,
        0xAE, 0x47, 0xE1, 0x3D, 0x29, 0x5C, 0x8F, 0x3D, 0x28, 0x5C, 0x8F, 0x3D, 0x8F, 0xC2, 0x75, 0x3D, 0x08, 0xD7, 0xA3, 0x3C, 0x0A, 0xD7, 0x23, 0x3D, 0x0C, 0xD7, 0xA3, 0xBC, 0x0A, 0xD7, 0xA3, 0x3C,
        0xCE, 0xCC, 0x4C, 0xBD, 0x00, 0x00, 0x00, 0x00, 0x29, 0x5C, 0x8F, 0xBD, 0xCC, 0xCC, 0x4C, 0x3D, 0x99, 0x99, 0x19, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0x8E, 0xC2, 0xF5, 0x3C, 0x00, 0x00, 0x00, 0x00,
        0x8F, 0xC2, 0xF5, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0x8E, 0xC2, 0x75, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0xEB, 0x51, 0xB8, 0x3D, 0xEB, 0x51, 0x38, 0x3E, 0x00, 0x00, 0x00, 0x00,
        0xEB, 0x51, 0xB8, 0x3D, 0xEB, 0x51, 0x38, 0x3E, 0xEB, 0x51, 0xB8, 0x3D, 0x0A, 0xD7, 0xA3, 0x3C, 0x0A, 0xD7, 0x23, 0x3D, 0x0A, 0xD7, 0x23, 0x3C, 0x8E, 0xC2, 0xF5, 0x3C, 0x0A, 0xD7, 0x23, 0x3C,
        0xCC, 0xCC, 0x4C, 0x3D, 0x0A, 0xD7, 0xA3, 0x3C, 0x08, 0xD7, 0xA3, 0x3C, 0xEB, 0x51, 0x38, 0x3E, 0x00, 0x00, 0x80, 0x3E, 0x90, 0xC2, 0x75, 0x3D, 0x3D, 0x0A, 0x57, 0x3E, 0x0C, 0xD7, 0x23, 0x3C,
        0x7A, 0x14, 0x2E, 0x3E, 0x0C, 0xD7, 0x23, 0x3C, 0x0A, 0xD7, 0x23, 0x3D, 0x90, 0xC2, 0xF5, 0x3C, 0x08, 0xD7, 0x23, 0x3C, 0x90, 0xC2, 0x75, 0x3D, 0x00, 0x00, 0x00, 0x00, 0xAE, 0x47, 0xE1, 0x3D,
        0x08, 0xD7, 0x23, 0x3C, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0xEB, 0x51, 0xB8, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0xB8, 0x1E, 0x05, 0x3E,
        0x7A, 0x14, 0x2E, 0x3E, 0xAE, 0x47, 0xE1, 0x3D, 0xCC, 0xCC, 0x4C, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x7A, 0x14, 0x2E, 0x3E, 0x0C, 0xD7, 0xA3, 0x3C, 0x5C, 0x8F, 0x42, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D,
        0x5C, 0x8F, 0x42, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x99, 0x99, 0x19, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x29, 0x5C, 0x0F, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xD7, 0xA3, 0x3C,
        0x3D, 0x0A, 0x57, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x3D, 0x0A, 0x57, 0x3E, 0x29, 0x5C, 0x8F, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0xCD, 0xCC, 0xCC, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D,
        0x8F, 0xC2, 0xF5, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0xAE, 0x47, 0xE1, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0xCD, 0xCC, 0xCC, 0x3D, 0x3D, 0x0A, 0x57, 0x3E, 0x9A, 0x99, 0x19, 0x3E,
        0x28, 0x5C, 0x8F, 0x3D, 0xCD, 0xCC, 0xCC, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xC2, 0xF5, 0x3D, 0x3D, 0x0A, 0x57, 0x3E, 0xCD, 0xCC, 0x4C, 0x3D, 0x28, 0x5C, 0x0F, 0x3E, 0xAE, 0x47, 0xE1, 0x3D,
        0xB8, 0x1E, 0x05, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0xEB, 0x51, 0x38, 0x3E, 0x29, 0x5C, 0x8F, 0x3D, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x1E, 0x05, 0x3E, 0x8E, 0xC2, 0x75, 0x3D, 0xB8, 0x1E, 0x05, 0x3E,
        0x28, 0x5C, 0x8F, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0x8F, 0xC2, 0xF5, 0x3C, 0x8F, 0xC2, 0xF5, 0x3D, 0x0C, 0xD7, 0x23, 0x3C, 0xCC, 0xCC, 0xCC, 0x3D, 0x29, 0x5C, 0x0F, 0x3E,
        0x3D, 0x0A, 0x57, 0x3E, 0x0C, 0xD7, 0x23, 0x3C, 0xEB, 0x51, 0x38, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0xAE, 0x47, 0xE1, 0x3D,
        0xAE, 0x47, 0xE1, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0xEB, 0x51, 0xB8, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x08, 0xD7, 0xA3, 0x3C, 0x0C, 0xD7, 0x23, 0x3C,
        0xEB, 0x51, 0xB8, 0x3D, 0x90, 0xC2, 0xF5, 0x3C, 0xAE, 0x47, 0xE1, 0x3D, 0x90, 0xC2, 0x75, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0x28, 0x5C, 0x0F, 0x3E, 0xB8, 0x1E, 0x05, 0x3E,
        0x0A, 0xD7, 0x23, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0xEB, 0x51, 0x38, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x28, 0x5C, 0x0F, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0xCD, 0xCC, 0xCC, 0x3D,
        0xEB, 0x51, 0xB8, 0x3D, 0x29, 0x5C, 0x8F, 0x3D, 0x0A, 0xD7, 0xA3, 0x3D, 0x90, 0xC2, 0x75, 0x3D, 0x0A, 0xD7, 0xA3, 0x3D, 0x90, 0xC2, 0xF5, 0x3C, 0xEB, 0x51, 0xB8, 0x3D, 0x0C, 0xD7, 0x23, 0x3C,
        0xAE, 0x47, 0xE1, 0x3D, 0x29, 0x5C, 0x8F, 0x3D, 0x3D, 0x0A, 0x57, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0x0A, 0xD7, 0x23, 0x3D, 0x0A, 0xD7, 0xA3, 0x3C, 0xAE, 0x47, 0xE1, 0x3D, 0x0A, 0xD7, 0x23, 0x3E,
        0xEB, 0x51, 0x38, 0x3E, 0x0A, 0xD7, 0x23, 0x3E, 0x00, 0x00, 0x00, 0x00, 0xEB, 0x51, 0x38, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0xEB, 0x51, 0x38, 0x3E, 0x8E, 0xC2, 0x75, 0x3D, 0x0A, 0xD7, 0x23, 0x3E,
        0xEB, 0x51, 0xB8, 0x3D, 0x0A, 0xD7, 0x23, 0x3D, 0x3D, 0x0A, 0x57, 0x3E, 0xAE, 0x47, 0xE1, 0x3D, 0x5C, 0x8F, 0x42, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0x7A, 0x14, 0x2E, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D,
        0x99, 0x99, 0x19, 0x3E, 0x8F, 0xC2, 0x75, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0x8F, 0xC2, 0x75, 0x3D, 0x28, 0x5C, 0x8F, 0x3D, 0xCC, 0xCC, 0x4C, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0x29, 0x5C, 0x8F, 0x3D,
        0x08, 0xD7, 0x23, 0x3C, 0xEB, 0x51, 0xB8, 0x3D, 0x99, 0x99, 0x19, 0x3E, 0x28, 0x5C, 0x8F, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0x09, 0xD7, 0xA3, 0x3C,
        0x99, 0x99, 0x19, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0xA3, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0x8E, 0xC2, 0x75, 0x3D, 0x8E, 0xC2, 0xF5, 0x3C, 0xCC, 0xCC, 0x4C, 0x3D, 0x8E, 0xC2, 0x75, 0x3D,
        0xCC, 0xCC, 0x4C, 0x3D, 0x0A, 0xD7, 0xA3, 0x3D, 0x8E, 0xC2, 0x75, 0x3D, 0xEB, 0x51, 0xB8, 0x3D, 0x0A, 0xD7, 0xA3, 0x3D, 0x09, 0xD7, 0xA3, 0x3C, 0x8E, 0xC2, 0x75, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D,
        0x0A, 0xD7, 0x23, 0x3E, 0xEB, 0x51, 0xB8, 0x3D, 0x8E, 0xC2, 0x75, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0xCC, 0xCC, 0x4C, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0x0A, 0xD7, 0x23, 0x3E,
        0xCC, 0xCC, 0xCC, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0x99, 0x99, 0x19, 0x3E, 0x99, 0x99, 0x19, 0x3E, 0x28, 0x5C, 0x0F, 0x3E, 0x7A, 0x14, 0x2E, 0x3E, 0x90, 0xC2, 0xF5, 0x3C,
        0x28, 0x5C, 0x8F, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0x99, 0x99, 0x19, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0xEC, 0x51, 0xB8, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x9A, 0x99, 0x19, 0x3E,
        0x0A, 0xD7, 0x23, 0x3E, 0x00, 0x00, 0x00, 0x00, 0xB8, 0x1E, 0x05, 0x3E, 0x0C, 0xD7, 0xA3, 0x3C, 0x8E, 0xC2, 0xF5, 0x3C, 0x0A, 0xD7, 0x23, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0x9A, 0x99, 0x19, 0x3E,
        0xCC, 0xCC, 0x4C, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x9A, 0x99, 0x19, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0xCD, 0xCC, 0xCC, 0x3D,
        0x0A, 0xD7, 0xA3, 0x3D, 0x29, 0x5C, 0x0F, 0x3E, 0xAE, 0x47, 0xE1, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0xCC, 0xCC, 0x4C, 0x3D, 0xEB, 0x51, 0xB8, 0x3D, 0x08, 0xD7, 0xA3, 0x3C, 0x0A, 0xD7, 0xA3, 0x3D,
        0x08, 0xD7, 0x23, 0x3C, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0x4C, 0x3D, 0xCD, 0xCC, 0x4C, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0x8F, 0xC2, 0xF5, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3E,
        0x3D, 0x0A, 0x57, 0x3E, 0x0A, 0xD7, 0x23, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0x29, 0x5C, 0x0F, 0x3E, 0x28, 0x5C, 0x0F, 0x3E, 0xB8, 0x1E, 0x05, 0x3E,
        0x8F, 0xC2, 0xF5, 0x3D, 0xEC, 0x51, 0xB8, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x3D, 0xEB, 0x51, 0xB8, 0x3D, 0x0A, 0xD7, 0x23, 0x3D, 0x9A, 0x99, 0x19, 0x3E,
        0x0C, 0xD7, 0xA3, 0xBC, 0xCC, 0xCC, 0x4C, 0x3E, 0x3D, 0x0A, 0x57, 0x3E, 0x0A, 0xD7, 0x23, 0x3C, 0x00, 0x00, 0x80, 0x3E, 0x29, 0x5C, 0x0F, 0x3E, 0x90, 0xC2, 0xF5, 0xBC, 0x8F, 0xC2, 0x75, 0x3D,
        0x00, 0x00, 0x80, 0x3E, 0x8F, 0xC2, 0x75, 0x3D, 0x29, 0x5C, 0x8F, 0xBD, 0x0A, 0xD7, 0xA3, 0x3D, 0x1E, 0x85, 0x6B, 0x3E, 0xEB, 0x51, 0x38, 0x3E, 0x29, 0x5C, 0x8F, 0xBD, 0x0A, 0xD7, 0xA3, 0x3C,
        0x0A, 0xD7, 0x23, 0x3E, 0x0A, 0xD7, 0xA3, 0x3C, 0x8F, 0xC2, 0xF5, 0x3D, 0x0A, 0xD7, 0x23, 0x3C, 0xB8, 0x1E, 0x05, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C, 0xB8, 0x1E, 0x05, 0x3E, 0x29, 0x5C, 0x8F, 0x3D,
        0x28, 0x5C, 0x0F, 0x3E, 0x8F, 0xC2, 0xF5, 0x3D, 0x0A, 0xD7, 0xA3, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x8E, 0xC2, 0xF5, 0x3C, 0xEB, 0x51, 0xB8, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0xAE, 0x47, 0xE1, 0x3D,
        0x8F, 0xC2, 0xF5, 0x3D, 0x8F, 0xC2, 0xF5, 0x3C, 0x7A, 0x14, 0x2E, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xC2, 0xF5, 0x3D, 0x0C, 0xD7, 0xA3, 0xBC, 0xAE, 0x47, 0xE1, 0x3D,
        0xCE, 0xCC, 0x4C, 0xBD, 0xCC, 0xCC, 0xCC, 0x3D, 0x8E, 0xC2, 0x75, 0xBD, 0x0A, 0xD7, 0xA3, 0x3D, 0x29, 0x5C, 0x8F, 0xBD, 0xCC, 0xCC, 0x4C, 0x3D, 0x29, 0x5C, 0x8F, 0xBD, 0x8F, 0xC2, 0xF5, 0x3C,
        0x8E, 0xC2, 0x75, 0xBD, 0xAE, 0x47, 0xE1, 0x3D, 0xCC, 0xCC, 0xCC, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3C, 0xAE, 0x47, 0x61, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D,
        0xCC, 0xCC, 0x4C, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0xAE, 0x47, 0x61, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0x90, 0xC2, 0xF5, 0xBC, 0x0A, 0xD7, 0x23, 0x3D, 0x8E, 0xC2, 0x75, 0xBD, 0x0A, 0xD7, 0xA3, 0x3C,
        0x29, 0x5C, 0x8F, 0xBD, 0x0A, 0xD7, 0x23, 0x3D, 0x0A, 0xD7, 0xA3, 0x3D, 0x0A, 0xD7, 0x23, 0x3E, 0x28, 0x5C, 0x0F, 0x3E, 0x3D, 0x0A, 0x57, 0x3E, 0xB8, 0x1E, 0x05, 0x3E, 0xAE, 0x47, 0x61, 0x3E,
        0xCC, 0xCC, 0xCC, 0x3D, 0xAE, 0x47, 0x61, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xC2, 0xF5, 0x3D, 0x29, 0x5C, 0x8F, 0xBD, 0x8E, 0xC2, 0xF5, 0x3C, 0x0A, 0xD7, 0xA3, 0x3D, 0x0A, 0xD7, 0xA3, 0x3D,
        0x28, 0x5C, 0x8F, 0x3D, 0x08, 0xD7, 0x23, 0x3C, 0x08, 0xD7, 0x23, 0x3C, 0x8F, 0xC2, 0xF5, 0x3C, 0x0A, 0xD7, 0x23, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x28, 0x5C, 0x0F, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C,
        0x8E, 0xC2, 0x75, 0x3E, 0x0A, 0xD7, 0xA3, 0x3C, 0x7A, 0x14, 0x2E, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C, 0x0A, 0xD7, 0x23, 0x3E, 0x0A, 0xD7, 0xA3, 0x3C, 0xCC, 0xCC, 0xCC, 0x3D, 0x0A, 0xD7, 0xA3, 0x3C,
        0xAE, 0x47, 0x61, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C, 0xEB, 0x51, 0x38, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0x7A, 0x14, 0x2E, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D, 0xB8, 0x1E, 0x05, 0x3E, 0x0A, 0xD7, 0x23, 0x3D,
        0xAE, 0x47, 0xE1, 0x3D, 0x0A, 0xD7, 0x23, 0x3D, 0x28, 0x5C, 0x8F, 0x3D, 0xCC, 0xCC, 0x4C, 0x3D, 0xCC, 0xCC, 0x4C, 0x3D, 0x0A, 0xD7, 0xA3, 0x3C, 0x0C, 0xD7, 0xA3, 0xBC, 0x0A, 0xD7, 0xA3, 0x3C,
        0x0A, 0xD7, 0x23, 0xBD, 0x0A, 0xD7, 0xA3, 0x3C, 0x0A, 0xD7, 0xA3, 0x3D, 0x0A, 0xD7, 0x23, 0x3D, 0x8E, 0xC2, 0x75, 0x3D, 0x0A, 0xD7, 0xA3, 0x3C, 0x8E, 0xC2, 0x75, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C,
        0x1E, 0x85, 0x6B, 0x3E, 0x0A, 0xD7, 0x23, 0x3D, 0x5C, 0x8F, 0x42, 0x3E, 0x8F, 0xC2, 0xF5, 0x3C, 0xCC, 0xCC, 0xCC, 0x3D, 0x8F, 0xC2, 0xF5, 0x3C, 0xAE, 0x47, 0x61, 0x3E, 0xCC, 0xCC, 0x4C, 0x3D,
        0xEB, 0x51, 0xB8, 0x3D, 0x0A, 0xD7, 0xA3, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xC2, 0xF5, 0x3C, 0x0A, 0xD7, 0x23, 0xBD, 0x0A, 0xD7, 0xA3, 0x3C, 0x8E, 0xC2, 0x75, 0xBD, 0xAE, 0x47, 0xE1, 0x3D,
        0x0A, 0xD7, 0xA3, 0x3D, 0x7A, 0x14, 0x2E, 0x3E, 0x0A, 0xD7, 0xA3, 0x3D, 0xEB, 0x51, 0x38, 0x3E, 0xCC, 0xCC, 0xCC, 0x3D, 0xCC, 0xCC, 0x4C, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x28, 0x5C, 0x8F, 0x3D,
        0xCC, 0xCC, 0xCC, 0x3D, 0xAE, 0x47, 0xE1, 0x3D, 0x28, 0x5C, 0x8F, 0x3D, 0x99, 0x99, 0x19, 0x3E, 0x8E, 0xC2, 0x75, 0x3D, 0x7A, 0x14, 0x2E, 0x3E, 0x28, 0x5C, 0x8F, 0x3D, 0xAE, 0x47, 0xE1, 0x3D,
        0x3D, 0x0A, 0x57, 0x3E, 0x9A, 0x99, 0x19, 0x3E, 0x3D, 0x0A, 0x57, 0x3E, 0x21, 0x0A, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02, 0x00, 0x03, 0x00, 0x03, 0x00, 0x04, 0x00, 0x04, 0x00, 0x05, 0x00, 0x05,
        0x00, 0x02, 0x00, 0x22, 0x04, 0x00, 0x06, 0x00, 0x07, 0x00, 0x08, 0x00, 0x09, 0x00, 0x23, 0x08, 0x00, 0x0A, 0x00, 0x0B, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x0E, 0x00, 0x0F, 0x00, 0x10, 0x00, 0x11,
        0x00, 0x24, 0x2A, 0x00, 0x12, 0x00, 0x13, 0x00, 0x14, 0x00, 0x15, 0x00, 0x16, 0x00, 0x17, 0x00, 0x17, 0x00, 0x18, 0x00, 0x18, 0x00, 0x19, 0x00, 0x19, 0x00, 0x1A, 0x00, 0x1A, 0x00, 0x1B, 0x00,
        0x1B, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1D, 0x00, 0x1D, 0x00, 0x1E, 0x00, 0x1E, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x20, 0x00, 0x20, 0x00, 0x21, 0x00, 0x21, 0x00, 0x22, 0x00, 0x22, 0x00, 0x11, 0x00,
        0x11, 0x00, 0x23, 0x00, 0x23, 0x00, 0x24, 0x00, 0x24, 0x00, 0x25, 0x00, 0x25, 0x00, 0x26, 0x00, 0x26, 0x00, 0x05, 0x00, 0x05, 0x00, 0x27, 0x00, 0x25, 0x34, 0x00, 0x28, 0x00, 0x29, 0x00, 0x19,
        0x00, 0x2A, 0x00, 0x2A, 0x00, 0x2B, 0x00, 0x2B, 0x00, 0x2C, 0x00, 0x2C, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x2E, 0x00, 0x2E, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1B, 0x00, 0x1B, 0x00, 0x2F, 0x00, 0x2F,
        0x00, 0x30, 0x00, 0x30, 0x00, 0x19, 0x00, 0x19, 0x00, 0x31, 0x00, 0x31, 0x00, 0x32, 0x00, 0x32, 0x00, 0x33, 0x00, 0x33, 0x00, 0x34, 0x00, 0x34, 0x00, 0x28, 0x00, 0x35, 0x00, 0x36, 0x00, 0x36,
        0x00, 0x37, 0x00, 0x37, 0x00, 0x38, 0x00, 0x38, 0x00, 0x39, 0x00, 0x39, 0x00, 0x3A, 0x00, 0x3A, 0x00, 0x3B, 0x00, 0x3B, 0x00, 0x3C, 0x00, 0x3C, 0x00, 0x3D, 0x00, 0x3D, 0x00, 0x3E, 0x00, 0x3E,
        0x00, 0x35, 0x00, 0x26, 0x3C, 0x00, 0x3F, 0x00, 0x40, 0x00, 0x40, 0x00, 0x41, 0x00, 0x41, 0x00, 0x42, 0x00, 0x42, 0x00, 0x43, 0x00, 0x43, 0x00, 0x44, 0x00, 0x44, 0x00, 0x11, 0x00, 0x11, 0x00,
        0x45, 0x00, 0x45, 0x00, 0x46, 0x00, 0x46, 0x00, 0x47, 0x00, 0x47, 0x00, 0x48, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0x49, 0x00, 0x49, 0x00, 0x10, 0x00, 0x10, 0x00,
        0x4A, 0x00, 0x4A, 0x00, 0x4B, 0x00, 0x4B, 0x00, 0x4C, 0x00, 0x4C, 0x00, 0x4D, 0x00, 0x4D, 0x00, 0x4E, 0x00, 0x4E, 0x00, 0x4F, 0x00, 0x4F, 0x00, 0x50, 0x00, 0x50, 0x00, 0x08, 0x00, 0x08, 0x00,
        0x51, 0x00, 0x51, 0x00, 0x52, 0x00, 0x52, 0x00, 0x53, 0x00, 0x53, 0x00, 0x54, 0x00, 0x54, 0x00, 0x55, 0x00, 0x55, 0x00, 0x56, 0x00, 0x56, 0x00, 0x57, 0x00, 0x57, 0x00, 0x58, 0x00, 0x27, 0x0C,
        0x00, 0x59, 0x00, 0x5A, 0x00, 0x5A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x1A, 0x00, 0x5B, 0x00, 0x5B, 0x00, 0x5C, 0x00, 0x5C, 0x00, 0x5D, 0x00, 0x28, 0x12, 0x00, 0x5E, 0x00, 0x5F, 0x00,
        0x5F, 0x00, 0x60, 0x00, 0x60, 0x00, 0x5C, 0x00, 0x5C, 0x00, 0x61, 0x00, 0x61, 0x00, 0x62, 0x00, 0x62, 0x00, 0x02, 0x00, 0x02, 0x00, 0x63, 0x00, 0x63, 0x00, 0x64, 0x00, 0x64, 0x00, 0x0D, 0x00,
        0x29, 0x12, 0x00, 0x65, 0x00, 0x66, 0x00, 0x66, 0x00, 0x67, 0x00, 0x67, 0x00, 0x68, 0x00, 0x68, 0x00, 0x69, 0x00, 0x69, 0x00, 0x6A, 0x00, 0x6A, 0x00, 0x6B, 0x00, 0x6B, 0x00, 0x6C, 0x00, 0x6C,
        0x00, 0x6D, 0x00, 0x6D, 0x00, 0x6E, 0x00, 0x2A, 0x06, 0x00, 0x6F, 0x00, 0x70, 0x00, 0x71, 0x00, 0x72, 0x00, 0x73, 0x00, 0x10, 0x00, 0x2B, 0x04, 0x00, 0x74, 0x00, 0x25, 0x00, 0x75, 0x00, 0x76,
        0x00, 0x2C, 0x0C, 0x00, 0x77, 0x00, 0x78, 0x00, 0x78, 0x00, 0x49, 0x00, 0x49, 0x00, 0x79, 0x00, 0x79, 0x00, 0x77, 0x00, 0x77, 0x00, 0x7A, 0x00, 0x7A, 0x00, 0x29, 0x00, 0x2D, 0x02, 0x00, 0x75,
        0x00, 0x76, 0x00, 0x2E, 0x08, 0x00, 0x79, 0x00, 0x49, 0x00, 0x49, 0x00, 0x78, 0x00, 0x78, 0x00, 0x77, 0x00, 0x77, 0x00, 0x79, 0x00, 0x2F, 0x02, 0x00, 0x7B, 0x00, 0x6E, 0x00, 0x30, 0x20, 0x00,
        0x7C, 0x00, 0x60, 0x00, 0x60, 0x00, 0x7D, 0x00, 0x7D, 0x00, 0x71, 0x00, 0x71, 0x00, 0x75, 0x00, 0x75, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x80, 0x00, 0x80, 0x00, 0x47, 0x00,
        0x47, 0x00, 0x81, 0x00, 0x81, 0x00, 0x82, 0x00, 0x82, 0x00, 0x83, 0x00, 0x83, 0x00, 0x84, 0x00, 0x84, 0x00, 0x85, 0x00, 0x85, 0x00, 0x86, 0x00, 0x86, 0x00, 0x08, 0x00, 0x08, 0x00, 0x7C, 0x00,
        0x31, 0x06, 0x00, 0x87, 0x00, 0x5B, 0x00, 0x5B, 0x00, 0x19, 0x00, 0x19, 0x00, 0x26, 0x00, 0x32, 0x1A, 0x00, 0x5C, 0x00, 0x7D, 0x00, 0x7D, 0x00, 0x88, 0x00, 0x88, 0x00, 0x60, 0x00, 0x60, 0x00,
        0x19, 0x00, 0x19, 0x00, 0x18, 0x00, 0x18, 0x00, 0x86, 0x00, 0x86, 0x00, 0x89, 0x00, 0x89, 0x00, 0x85, 0x00, 0x85, 0x00, 0x8A, 0x00, 0x8A, 0x00, 0x8B, 0x00, 0x8B, 0x00, 0x20, 0x00, 0x20, 0x00,
        0x29, 0x00, 0x29, 0x00, 0x8C, 0x00, 0x33, 0x1C, 0x00, 0x8D, 0x00, 0x8E, 0x00, 0x8E, 0x00, 0x8F, 0x00, 0x8F, 0x00, 0x90, 0x00, 0x90, 0x00, 0x91, 0x00, 0x91, 0x00, 0x92, 0x00, 0x92, 0x00, 0x93,
        0x00, 0x93, 0x00, 0x11, 0x00, 0x11, 0x00, 0x56, 0x00, 0x56, 0x00, 0x81, 0x00, 0x81, 0x00, 0x47, 0x00, 0x47, 0x00, 0x26, 0x00, 0x26, 0x00, 0x05, 0x00, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0x49,
        0x00, 0x34, 0x06, 0x00, 0x94, 0x00, 0x62, 0x00, 0x62, 0x00, 0x95, 0x00, 0x94, 0x00, 0x96, 0x00, 0x35, 0x20, 0x00, 0x97, 0x00, 0x8D, 0x00, 0x8D, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x1E, 0x00, 0x1E,
        0x00, 0x98, 0x00, 0x98, 0x00, 0x09, 0x00, 0x09, 0x00, 0x99, 0x00, 0x99, 0x00, 0x92, 0x00, 0x92, 0x00, 0x93, 0x00, 0x93, 0x00, 0x11, 0x00, 0x11, 0x00, 0x56, 0x00, 0x56, 0x00, 0x81, 0x00, 0x81,
        0x00, 0x47, 0x00, 0x47, 0x00, 0x26, 0x00, 0x26, 0x00, 0x05, 0x00, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0x49, 0x00, 0x36, 0x2C, 0x00, 0x9A, 0x00, 0x86, 0x00, 0x86, 0x00, 0x08, 0x00, 0x08, 0x00,
        0x7C, 0x00, 0x7C, 0x00, 0x60, 0x00, 0x60, 0x00, 0x7D, 0x00, 0x7D, 0x00, 0x71, 0x00, 0x71, 0x00, 0x62, 0x00, 0x62, 0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x80, 0x00, 0x80, 0x00,
        0x9B, 0x00, 0x9B, 0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x45, 0x00, 0x9C, 0x00, 0x9C, 0x00, 0x9D, 0x00, 0x9D, 0x00, 0x9E, 0x00, 0x9E, 0x00, 0x73, 0x00, 0x73, 0x00, 0x8F, 0x00, 0x8F, 0x00,
        0x54, 0x00, 0x54, 0x00, 0x9F, 0x00, 0x9F, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0x62, 0x00, 0x37, 0x04, 0x00, 0xA1, 0x00, 0x48, 0x00, 0x06, 0x00, 0xA1, 0x00, 0x38, 0x38, 0x00, 0x19, 0x00, 0x1A, 0x00,
        0x1A, 0x00, 0xA2, 0x00, 0xA2, 0x00, 0x5C, 0x00, 0x5C, 0x00, 0x2E, 0x00, 0x2E, 0x00, 0xA3, 0x00, 0xA3, 0x00, 0xA4, 0x00, 0xA4, 0x00, 0xA5, 0x00, 0xA5, 0x00, 0xA6, 0x00, 0xA6, 0x00, 0x35, 0x00,
        0x35, 0x00, 0xA7, 0x00, 0xA7, 0x00, 0xA8, 0x00, 0xA8, 0x00, 0x24, 0x00, 0x24, 0x00, 0x25, 0x00, 0x25, 0x00, 0x26, 0x00, 0x26, 0x00, 0x05, 0x00, 0x05, 0x00, 0x02, 0x00, 0x02, 0x00, 0x49, 0x00,
        0x49, 0x00, 0x62, 0x00, 0x62, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0xAB, 0x00, 0xAB, 0x00, 0x90, 0x00, 0x90, 0x00, 0xAC, 0x00, 0xAC, 0x00, 0xAD, 0x00, 0xAD, 0x00, 0xAE, 0x00,
        0xAE, 0x00, 0x17, 0x00, 0x17, 0x00, 0x18, 0x00, 0x18, 0x00, 0x19, 0x00, 0x39, 0x2C, 0x00, 0xAF, 0x00, 0xB0, 0x00, 0xB0, 0x00, 0xB1, 0x00, 0xB1, 0x00, 0xB2, 0x00, 0xB2, 0x00, 0xB3, 0x00, 0xB3,
        0x00, 0xB4, 0x00, 0xB4, 0x00, 0xB5, 0x00, 0xB5, 0x00, 0x07, 0x00, 0x07, 0x00, 0x5D, 0x00, 0x5D, 0x00, 0xA2, 0x00, 0xA2, 0x00, 0x60, 0x00, 0x60, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0xB6, 0x00, 0xB6,
        0x00, 0x50, 0x00, 0x50, 0x00, 0x9A, 0x00, 0x9A, 0x00, 0xAF, 0x00, 0xAF, 0x00, 0xA6, 0x00, 0xA6, 0x00, 0xB7, 0x00, 0xB7, 0x00, 0x46, 0x00, 0x46, 0x00, 0x9B, 0x00, 0x9B, 0x00, 0x26, 0x00, 0x26,
        0x00, 0x05, 0x00, 0x05, 0x00, 0x78, 0x00, 0x3A, 0x10, 0x00, 0x0E, 0x00, 0x61, 0x00, 0x61, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xB8, 0x00, 0xB8, 0x00, 0x0E, 0x00, 0x79, 0x00, 0x49, 0x00, 0x49, 0x00,
        0x78, 0x00, 0x78, 0x00, 0x77, 0x00, 0x77, 0x00, 0x79, 0x00, 0x3B, 0x14, 0x00, 0x0E, 0x00, 0x61, 0x00, 0x61, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xB8, 0x00, 0xB8, 0x00, 0x0E, 0x00, 0x77, 0x00, 0x78,
        0x00, 0x78, 0x00, 0x49, 0x00, 0x49, 0x00, 0x79, 0x00, 0x79, 0x00, 0x77, 0x00, 0x77, 0x00, 0x7A, 0x00, 0x7A, 0x00, 0x29, 0x00, 0x3C, 0x04, 0x00, 0xB9, 0x00, 0x75, 0x00, 0x75, 0x00, 0xBA, 0x00,
        0x3D, 0x04, 0x00, 0x71, 0x00, 0xBB, 0x00, 0x10, 0x00, 0xBC, 0x00, 0x3E, 0x04, 0x00, 0x1B, 0x00, 0xBD, 0x00, 0xBD, 0x00, 0x29, 0x00, 0x3F, 0x22, 0x00, 0x1C, 0x00, 0x87, 0x00, 0x87, 0x00, 0x59,
        0x00, 0x59, 0x00, 0x1A, 0x00, 0x1A, 0x00, 0xBE, 0x00, 0xBE, 0x00, 0x08, 0x00, 0x08, 0x00, 0x50, 0x00, 0x50, 0x00, 0xBF, 0x00, 0xBF, 0x00, 0xC0, 0x00, 0xC0, 0x00, 0xC1, 0x00, 0xC1, 0x00, 0x99,
        0x00, 0x99, 0x00, 0x73, 0x00, 0x73, 0x00, 0xC2, 0x00, 0xC2, 0x00, 0xC3, 0x00, 0x6B, 0x00, 0xC4, 0x00, 0xC4, 0x00, 0x80, 0x00, 0x80, 0x00, 0xC5, 0x00, 0xC5, 0x00, 0x6B, 0x00, 0x40, 0x34, 0x00,
        0x90, 0x00, 0xC6, 0x00, 0xC6, 0x00, 0xC7, 0x00, 0xC7, 0x00, 0xC8, 0x00, 0xC8, 0x00, 0xC9, 0x00, 0xC9, 0x00, 0x1D, 0x00, 0x1D, 0x00, 0x61, 0x00, 0x61, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0xCB, 0x00,
        0xCB, 0x00, 0xCC, 0x00, 0xCC, 0x00, 0xCD, 0x00, 0xCD, 0x00, 0xCE, 0x00, 0xCE, 0x00, 0xCF, 0x00, 0xC8, 0x00, 0x2E, 0x00, 0x2E, 0x00, 0xB5, 0x00, 0xB5, 0x00, 0x4A, 0x00, 0x4A, 0x00, 0xD0, 0x00,
        0xD0, 0x00, 0xCC, 0x00, 0xD1, 0x00, 0xCF, 0x00, 0xCF, 0x00, 0xD2, 0x00, 0xD2, 0x00, 0xD3, 0x00, 0xD3, 0x00, 0xD4, 0x00, 0xD4, 0x00, 0x95, 0x00, 0x95, 0x00, 0xD5, 0x00, 0xD5, 0x00, 0xD6, 0x00,
        0xD6, 0x00, 0xD7, 0x00, 0xD7, 0x00, 0xD8, 0x00, 0x41, 0x06, 0x00, 0x08, 0x00, 0x29, 0x00, 0x08, 0x00, 0xBA, 0x00, 0xD9, 0x00, 0x9D, 0x00, 0x42, 0x24, 0x00, 0x06, 0x00, 0x29, 0x00, 0x06, 0x00,
        0x18, 0x00, 0x18, 0x00, 0x17, 0x00, 0x17, 0x00, 0x33, 0x00, 0x33, 0x00, 0xD8, 0x00, 0xD8, 0x00, 0xDA, 0x00, 0xDA, 0x00, 0xDB, 0x00, 0xDB, 0x00, 0x91, 0x00, 0x91, 0x00, 0xDC, 0x00, 0x61, 0x00,
        0xDC, 0x00, 0xDC, 0x00, 0x9E, 0x00, 0x9E, 0x00, 0xA6, 0x00, 0xA6, 0x00, 0x35, 0x00, 0x35, 0x00, 0xA7, 0x00, 0xA7, 0x00, 0xA8, 0x00, 0xA8, 0x00, 0x24, 0x00, 0x24, 0x00, 0x25, 0x00, 0x25, 0x00,
        0x29, 0x00, 0x43, 0x22, 0x00, 0xDD, 0x00, 0x16, 0x00, 0x16, 0x00, 0x17, 0x00, 0x17, 0x00, 0x94, 0x00, 0x94, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x67, 0x00, 0x67, 0x00, 0x5B, 0x00, 0x5B, 0x00, 0x5C,
        0x00, 0x5C, 0x00, 0xDE, 0x00, 0xDE, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x79, 0x00, 0x79, 0x00, 0xDF, 0x00, 0xDF, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x96, 0x00, 0x96, 0x00, 0x24,
        0x00, 0x24, 0x00, 0x23, 0x00, 0x23, 0x00, 0xE1, 0x00, 0x44, 0x18, 0x00, 0x06, 0x00, 0x29, 0x00, 0x06, 0x00, 0xB6, 0x00, 0xB6, 0x00, 0x50, 0x00, 0x50, 0x00, 0x9A, 0x00, 0x9A, 0x00, 0xAD, 0x00,
        0xAD, 0x00, 0xE2, 0x00, 0xE2, 0x00, 0x93, 0x00, 0x93, 0x00, 0xD4, 0x00, 0xD4, 0x00, 0x45, 0x00, 0x45, 0x00, 0x46, 0x00, 0x46, 0x00, 0x9B, 0x00, 0x9B, 0x00, 0x29, 0x00, 0x45, 0x08, 0x00, 0x06,
        0x00, 0x29, 0x00, 0x06, 0x00, 0x8E, 0x00, 0x61, 0x00, 0xE3, 0x00, 0x29, 0x00, 0x39, 0x00, 0x46, 0x06, 0x00, 0x06, 0x00, 0x29, 0x00, 0x06, 0x00, 0x8E, 0x00, 0x61, 0x00, 0xE3, 0x00, 0x47, 0x26,
        0x00, 0xDD, 0x00, 0x16, 0x00, 0x16, 0x00, 0x17, 0x00, 0x17, 0x00, 0x94, 0x00, 0x94, 0x00, 0x7C, 0x00, 0x7C, 0x00, 0x67, 0x00, 0x67, 0x00, 0x5B, 0x00, 0x5B, 0x00, 0x5C, 0x00, 0x5C, 0x00, 0xDE,
        0x00, 0xDE, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x79, 0x00, 0x79, 0x00, 0xDF, 0x00, 0xDF, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x96, 0x00, 0x96, 0x00, 0x24, 0x00, 0x24, 0x00, 0x23,
        0x00, 0x23, 0x00, 0xE1, 0x00, 0xE1, 0x00, 0xE4, 0x00, 0xE5, 0x00, 0xE4, 0x00, 0x48, 0x06, 0x00, 0x06, 0x00, 0x29, 0x00, 0xA1, 0x00, 0x8C, 0x00, 0x61, 0x00, 0xE6, 0x00, 0x49, 0x02, 0x00, 0x06,
        0x00, 0x29, 0x00, 0x4A, 0x12, 0x00, 0x94, 0x00, 0xE7, 0x00, 0xE7, 0x00, 0xE8, 0x00, 0xE8, 0x00, 0xE9, 0x00, 0xE9, 0x00, 0x80, 0x00, 0x80, 0x00, 0x48, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00,
        0x02, 0x00, 0x02, 0x00, 0xEA, 0x00, 0xEA, 0x00, 0x62, 0x00, 0x4B, 0x06, 0x00, 0x06, 0x00, 0x29, 0x00, 0xA1, 0x00, 0x62, 0x00, 0xEB, 0x00, 0x8C, 0x00, 0x4C, 0x04, 0x00, 0x06, 0x00, 0x29, 0x00,
        0x29, 0x00, 0xEC, 0x00, 0x4D, 0x08, 0x00, 0x06, 0x00, 0x29, 0x00, 0x06, 0x00, 0x47, 0x00, 0xED, 0x00, 0x47, 0x00, 0xED, 0x00, 0xBA, 0x00, 0x4E, 0x06, 0x00, 0x06, 0x00, 0x29, 0x00, 0x06, 0x00,
        0x8C, 0x00, 0xA1, 0x00, 0x8C, 0x00, 0x4F, 0x28, 0x00, 0x7C, 0x00, 0x67, 0x00, 0x67, 0x00, 0x5B, 0x00, 0x5B, 0x00, 0x5C, 0x00, 0x5C, 0x00, 0xDE, 0x00, 0xDE, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x79,
        0x00, 0x79, 0x00, 0xDF, 0x00, 0xDF, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x96, 0x00, 0x96, 0x00, 0x24, 0x00, 0x24, 0x00, 0x23, 0x00, 0x23, 0x00, 0xE1, 0x00, 0xE1, 0x00, 0xEE,
        0x00, 0xEE, 0x00, 0xEF, 0x00, 0xEF, 0x00, 0xDD, 0x00, 0xDD, 0x00, 0x16, 0x00, 0x16, 0x00, 0x17, 0x00, 0x17, 0x00, 0x94, 0x00, 0x94, 0x00, 0x7C, 0x00, 0x50, 0x14, 0x00, 0x06, 0x00, 0x29, 0x00,
        0x06, 0x00, 0x18, 0x00, 0x18, 0x00, 0x17, 0x00, 0x17, 0x00, 0x33, 0x00, 0x33, 0x00, 0xD8, 0x00, 0xD8, 0x00, 0xF0, 0x00, 0xF0, 0x00, 0xF1, 0x00, 0xF1, 0x00, 0xB0, 0x00, 0xB0, 0x00, 0xF2, 0x00,
        0xF2, 0x00, 0xF3, 0x00, 0x51, 0x2A, 0x00, 0x7C, 0x00, 0x67, 0x00, 0x67, 0x00, 0x5B, 0x00, 0x5B, 0x00, 0x5C, 0x00, 0x5C, 0x00, 0xDE, 0x00, 0xDE, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x79, 0x00, 0x79,
        0x00, 0xDF, 0x00, 0xDF, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x96, 0x00, 0x96, 0x00, 0x24, 0x00, 0x24, 0x00, 0x23, 0x00, 0x23, 0x00, 0xE1, 0x00, 0xE1, 0x00, 0xEE, 0x00, 0xEE,
        0x00, 0xEF, 0x00, 0xEF, 0x00, 0xDD, 0x00, 0xDD, 0x00, 0x16, 0x00, 0x16, 0x00, 0x17, 0x00, 0x17, 0x00, 0x94, 0x00, 0x94, 0x00, 0x7C, 0x00, 0xF4, 0x00, 0xF5, 0x00, 0x52, 0x16, 0x00, 0x06, 0x00,
        0x29, 0x00, 0x06, 0x00, 0x18, 0x00, 0x18, 0x00, 0x17, 0x00, 0x17, 0x00, 0x33, 0x00, 0x33, 0x00, 0xD8, 0x00, 0xD8, 0x00, 0xDA, 0x00, 0xDA, 0x00, 0xDB, 0x00, 0xDB, 0x00, 0x91, 0x00, 0x91, 0x00,
        0xDC, 0x00, 0xDC, 0x00, 0x61, 0x00, 0x69, 0x00, 0x8C, 0x00, 0x53, 0x26, 0x00, 0x16, 0x00, 0x17, 0x00, 0x17, 0x00, 0x18, 0x00, 0x18, 0x00, 0x19, 0x00, 0x19, 0x00, 0x1A, 0x00, 0x1A, 0x00, 0x1B,
        0x00, 0x1B, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1D, 0x00, 0x1D, 0x00, 0x1E, 0x00, 0x1E, 0x00, 0x1F, 0x00, 0x1F, 0x00, 0x20, 0x00, 0x20, 0x00, 0x21, 0x00, 0x21, 0x00, 0x22, 0x00, 0x22, 0x00, 0x11,
        0x00, 0x11, 0x00, 0x23, 0x00, 0x23, 0x00, 0x24, 0x00, 0x24, 0x00, 0x25, 0x00, 0x25, 0x00, 0x26, 0x00, 0x26, 0x00, 0x05, 0x00, 0x05, 0x00, 0x27, 0x00, 0x54, 0x04, 0x00, 0xB6, 0x00, 0x9B, 0x00,
        0x06, 0x00, 0xA1, 0x00, 0x55, 0x12, 0x00, 0x06, 0x00, 0x10, 0x00, 0x10, 0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x80, 0x00, 0x80, 0x00, 0x47, 0x00, 0x47, 0x00, 0x81, 0x00, 0x81,
        0x00, 0x56, 0x00, 0x56, 0x00, 0x11, 0x00, 0x11, 0x00, 0xA1, 0x00, 0x56, 0x04, 0x00, 0x06, 0x00, 0x47, 0x00, 0xED, 0x00, 0x47, 0x00, 0x57, 0x08, 0x00, 0x06, 0x00, 0x26, 0x00, 0x94, 0x00, 0x26,
        0x00, 0x94, 0x00, 0x3A, 0x00, 0xF6, 0x00, 0x3A, 0x00, 0x58, 0x04, 0x00, 0x06, 0x00, 0x8C, 0x00, 0xA1, 0x00, 0x29, 0x00, 0x59, 0x06, 0x00, 0x06, 0x00, 0xE3, 0x00, 0xE3, 0x00, 0x47, 0x00, 0xED,
        0x00, 0xE3, 0x00, 0x5A, 0x06, 0x00, 0xA1, 0x00, 0x29, 0x00, 0x06, 0x00, 0xA1, 0x00, 0x29, 0x00, 0x8C, 0x00, 0x5B, 0x08, 0x00, 0x65, 0x00, 0x6E, 0x00, 0xF7, 0x00, 0x0B, 0x00, 0x65, 0x00, 0x5E,
        0x00, 0x6E, 0x00, 0x0D, 0x00, 0x5C, 0x02, 0x00, 0x06, 0x00, 0xF8, 0x00, 0x5D, 0x08, 0x00, 0xF9, 0x00, 0xFA, 0x00, 0x5E, 0x00, 0x0D, 0x00, 0x65, 0x00, 0x5E, 0x00, 0x6E, 0x00, 0x0D, 0x00, 0x5E,
        0x04, 0x00, 0xFB, 0x00, 0x75, 0x00, 0xFB, 0x00, 0xBD, 0x00, 0x5F, 0x02, 0x00, 0x6E, 0x00, 0xFC, 0x00, 0x60, 0x0C, 0x00, 0xFD, 0x00, 0x07, 0x00, 0x07, 0x00, 0x71, 0x00, 0x71, 0x00, 0xB5, 0x00,
        0xB5, 0x00, 0xFE, 0x00, 0xFE, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x71, 0x00, 0x61, 0x1C, 0x00, 0xAC, 0x00, 0xEC, 0x00, 0xB0, 0x00, 0x90, 0x00, 0x90, 0x00, 0x09, 0x00, 0x09, 0x00, 0x98, 0x00, 0x98,
        0x00, 0x00, 0x01, 0x00, 0x01, 0xB5, 0x00, 0xB5, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x10, 0x00, 0x10, 0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x26, 0x00, 0x26, 0x00, 0x47, 0x00, 0x47,
        0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x62, 0x1C, 0x00, 0x06, 0x00, 0x29, 0x00, 0x61, 0x00, 0x1E, 0x00, 0x1E, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x01, 0x01, 0x01, 0x01, 0x4C, 0x00, 0x4C, 0x00,
        0xA5, 0x00, 0xA5, 0x00, 0x02, 0x01, 0x02, 0x01, 0x36, 0x00, 0x36, 0x00, 0x03, 0x01, 0x03, 0x01, 0x04, 0x01, 0x04, 0x01, 0x9B, 0x00, 0x9B, 0x00, 0x48, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00,
        0x27, 0x00, 0x63, 0x1A, 0x00, 0xB0, 0x00, 0x90, 0x00, 0x90, 0x00, 0x09, 0x00, 0x09, 0x00, 0x98, 0x00, 0x98, 0x00, 0x00, 0x01, 0x00, 0x01, 0xB5, 0x00, 0xB5, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x10,
        0x00, 0x10, 0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x26, 0x00, 0x26, 0x00, 0x47, 0x00, 0x47, 0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x64, 0x1C, 0x00, 0x97, 0x00, 0xEC, 0x00,
        0xB0, 0x00, 0x90, 0x00, 0x90, 0x00, 0x09, 0x00, 0x09, 0x00, 0x98, 0x00, 0x98, 0x00, 0x00, 0x01, 0x00, 0x01, 0xB5, 0x00, 0xB5, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x10, 0x00, 0x10, 0x00, 0x78, 0x00,
        0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x26, 0x00, 0x26, 0x00, 0x47, 0x00, 0x47, 0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x65, 0x20, 0x00, 0xCA, 0x00, 0x02, 0x01, 0x02, 0x01, 0x9E, 0x00, 0x9E,
        0x00, 0x05, 0x01, 0x05, 0x01, 0x90, 0x00, 0x90, 0x00, 0x09, 0x00, 0x09, 0x00, 0x98, 0x00, 0x98, 0x00, 0x00, 0x01, 0x00, 0x01, 0xB5, 0x00, 0xB5, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x10, 0x00, 0x10,
        0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x26, 0x00, 0x26, 0x00, 0x47, 0x00, 0x47, 0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x66, 0x0A, 0x00, 0x08, 0x00, 0x7C, 0x00, 0x7C, 0x00,
        0x67, 0x00, 0x67, 0x00, 0x06, 0x01, 0x06, 0x01, 0x07, 0x01, 0x07, 0x00, 0x01, 0x01, 0x67, 0x26, 0x00, 0xAC, 0x00, 0x08, 0x01, 0x08, 0x01, 0x09, 0x01, 0x09, 0x01, 0x0A, 0x01, 0x0A, 0x01, 0x0B,
        0x01, 0x0B, 0x01, 0x0C, 0x01, 0x0C, 0x01, 0x0D, 0x01, 0xB0, 0x00, 0x90, 0x00, 0x90, 0x00, 0x09, 0x00, 0x09, 0x00, 0x98, 0x00, 0x98, 0x00, 0x00, 0x01, 0x00, 0x01, 0xB5, 0x00, 0xB5, 0x00, 0xCA,
        0x00, 0xCA, 0x00, 0x10, 0x00, 0x10, 0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x26, 0x00, 0x26, 0x00, 0x47, 0x00, 0x47, 0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x68, 0x0E, 0x00,
        0x06, 0x00, 0x29, 0x00, 0xF3, 0x00, 0x00, 0x01, 0x00, 0x01, 0x98, 0x00, 0x98, 0x00, 0x09, 0x00, 0x09, 0x00, 0x90, 0x00, 0x90, 0x00, 0x0E, 0x01, 0x0E, 0x01, 0x0F, 0x01, 0x69, 0x0A, 0x00, 0x06,
        0x00, 0x2F, 0x00, 0x2F, 0x00, 0x8D, 0x00, 0x8D, 0x00, 0x10, 0x01, 0x10, 0x01, 0x06, 0x00, 0x1D, 0x00, 0x04, 0x00, 0x6A, 0x10, 0x00, 0xBE, 0x00, 0x11, 0x01, 0x11, 0x01, 0x7C, 0x00, 0x7C, 0x00,
        0x12, 0x01, 0x12, 0x01, 0xBE, 0x00, 0x98, 0x00, 0x13, 0x01, 0x13, 0x01, 0x14, 0x01, 0x14, 0x01, 0x15, 0x01, 0x15, 0x01, 0x6E, 0x00, 0x6B, 0x06, 0x00, 0x06, 0x00, 0x29, 0x00, 0x4D, 0x00, 0x49,
        0x00, 0x16, 0x01, 0x0F, 0x01, 0x6C, 0x02, 0x00, 0x06, 0x00, 0x29, 0x00, 0x6D, 0x1A, 0x00, 0x07, 0x00, 0x29, 0x00, 0xF3, 0x00, 0x00, 0x01, 0x00, 0x01, 0x98, 0x00, 0x98, 0x00, 0x09, 0x00, 0x09,
        0x00, 0x90, 0x00, 0x90, 0x00, 0x0E, 0x01, 0x0E, 0x01, 0x0F, 0x01, 0x0E, 0x01, 0xE2, 0x00, 0xE2, 0x00, 0x17, 0x01, 0x17, 0x01, 0x41, 0x00, 0x41, 0x00, 0x18, 0x01, 0x18, 0x01, 0x19, 0x01, 0x19,
        0x01, 0x1A, 0x01, 0x6E, 0x0E, 0x00, 0x07, 0x00, 0x29, 0x00, 0xF3, 0x00, 0x00, 0x01, 0x00, 0x01, 0x98, 0x00, 0x98, 0x00, 0x09, 0x00, 0x09, 0x00, 0x90, 0x00, 0x90, 0x00, 0x0E, 0x01, 0x0E, 0x01,
        0x0F, 0x01, 0x6F, 0x20, 0x00, 0x98, 0x00, 0x00, 0x01, 0x00, 0x01, 0xB5, 0x00, 0xB5, 0x00, 0xCA, 0x00, 0xCA, 0x00, 0x10, 0x00, 0x10, 0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x26,
        0x00, 0x26, 0x00, 0x47, 0x00, 0x47, 0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x45, 0x00, 0x9C, 0x00, 0x9C, 0x00, 0x22, 0x00, 0x22, 0x00, 0xB0, 0x00, 0xB0, 0x00, 0x90, 0x00, 0x90, 0x00, 0x09,
        0x00, 0x09, 0x00, 0x98, 0x00, 0x70, 0x1C, 0x00, 0x07, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x1E, 0x00, 0x1E, 0x00, 0x2D, 0x00, 0x2D, 0x00, 0x01, 0x01, 0x01, 0x01, 0x4C, 0x00, 0x4C, 0x00, 0xA5, 0x00,
        0xA5, 0x00, 0x02, 0x01, 0x02, 0x01, 0x36, 0x00, 0x36, 0x00, 0x03, 0x01, 0x03, 0x01, 0x04, 0x01, 0x04, 0x01, 0x9B, 0x00, 0x9B, 0x00, 0x48, 0x00, 0x48, 0x00, 0x05, 0x00, 0x05, 0x00, 0x27, 0x00,
        0x71, 0x1C, 0x00, 0xAC, 0x00, 0x1B, 0x01, 0xB0, 0x00, 0x90, 0x00, 0x90, 0x00, 0x09, 0x00, 0x09, 0x00, 0x98, 0x00, 0x98, 0x00, 0x00, 0x01, 0x00, 0x01, 0xB5, 0x00, 0xB5, 0x00, 0xCA, 0x00, 0xCA,
        0x00, 0x10, 0x00, 0x10, 0x00, 0x78, 0x00, 0x78, 0x00, 0x7F, 0x00, 0x7F, 0x00, 0x26, 0x00, 0x26, 0x00, 0x47, 0x00, 0x47, 0x00, 0x46, 0x00, 0x46, 0x00, 0x45, 0x00, 0x72, 0x0A, 0x00, 0x07, 0x00,
        0x29, 0x00, 0xCA, 0x00, 0xB5, 0x00, 0xB5, 0x00, 0x00, 0x01, 0x00, 0x01, 0x98, 0x00, 0x98, 0x00, 0x09, 0x00, 0x73, 0x20, 0x00, 0xA5, 0x00, 0x90, 0x00, 0x90, 0x00, 0x01, 0x01, 0x01, 0x01, 0x2D,
        0x00, 0x2D, 0x00, 0xFF, 0x00, 0xFF, 0x00, 0x61, 0x00, 0x61, 0x00, 0xA9, 0x00, 0xA9, 0x00, 0x1C, 0x01, 0x1C, 0x01, 0x1D, 0x01, 0x1D, 0x01, 0x72, 0x00, 0x72, 0x00, 0x37, 0x00, 0x37, 0x00, 0x03,
        0x01, 0x03, 0x01, 0x46, 0x00, 0x46, 0x00, 0x9B, 0x00, 0x9B, 0x00, 0x48, 0x00, 0x48, 0x00, 0x1E, 0x01, 0x1E, 0x01, 0x27, 0x00, 0x74, 0x0A, 0x00, 0x30, 0x00, 0x1F, 0x01, 0x1F, 0x01, 0xE0, 0x00,
        0xE0, 0x00, 0x80, 0x00, 0x80, 0x00, 0x47, 0x00, 0x07, 0x00, 0x01, 0x01, 0x75, 0x0E, 0x00, 0x07, 0x00, 0x49, 0x00, 0x49, 0x00, 0x1E, 0x01, 0x1E, 0x01, 0x07, 0x01, 0x07, 0x01, 0x80, 0x00, 0x80,
        0x00, 0xE9, 0x00, 0xE9, 0x00, 0x37, 0x00, 0x20, 0x01, 0x0F, 0x01, 0x76, 0x04, 0x00, 0x07, 0x00, 0x80, 0x00, 0xAC, 0x00, 0x80, 0x00, 0x77, 0x08, 0x00, 0x07, 0x00, 0x48, 0x00, 0x09, 0x00, 0x48,
        0x00, 0x09, 0x00, 0xEC, 0x00, 0x17, 0x01, 0xEC, 0x00, 0x78, 0x04, 0x00, 0x07, 0x00, 0x0F, 0x01, 0x20, 0x01, 0x29, 0x00, 0x79, 0x0C, 0x00, 0x1D, 0x00, 0x9B, 0x00, 0xAF, 0x00, 0x9B, 0x00, 0x9B,
        0x00, 0x13, 0x00, 0x13, 0x00, 0x0D, 0x01, 0x0D, 0x01, 0x0B, 0x00, 0x0B, 0x00, 0x6E, 0x00, 0x7A, 0x06, 0x00, 0x20, 0x01, 0x29, 0x00, 0x07, 0x00, 0x20, 0x01, 0x29, 0x00, 0x0F, 0x01, 0x7B, 0x34,
        0x00, 0x12, 0x00, 0x21, 0x01, 0x21, 0x01, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x00, 0x59, 0x00, 0x22, 0x01, 0x22, 0x01, 0x23, 0x01, 0x23, 0x01, 0x2D, 0x00, 0x2D, 0x00, 0x1F,
        0x00, 0x1F, 0x00, 0x24, 0x01, 0x21, 0x01, 0x25, 0x01, 0x25, 0x01, 0x1A, 0x00, 0x1A, 0x00, 0x26, 0x01, 0x26, 0x01, 0x27, 0x01, 0x27, 0x01, 0x6F, 0x00, 0x6F, 0x00, 0x28, 0x01, 0x28, 0x01, 0x29,
        0x01, 0x29, 0x01, 0x75, 0x00, 0x75, 0x00, 0x2A, 0x01, 0x2A, 0x01, 0x2B, 0x01, 0x2B, 0x01, 0x70, 0x00, 0x70, 0x00, 0xE0, 0x00, 0xE0, 0x00, 0x07, 0x01, 0x07, 0x01, 0x2C, 0x01, 0x2C, 0x01, 0x2D,
        0x01, 0x2D, 0x01, 0x0D, 0x01, 0x2E, 0x01, 0x2F, 0x01, 0x7C, 0x02, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x7D, 0x34, 0x00, 0x65, 0x00, 0x30, 0x01, 0x30, 0x01, 0x31, 0x01, 0x31, 0x01, 0xBE, 0x00, 0xBE,
        0x00, 0x32, 0x01, 0x32, 0x01, 0x06, 0x01, 0x06, 0x01, 0xFD, 0x00, 0xFD, 0x00, 0x1D, 0x00, 0x1D, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x33, 0x01, 0x30, 0x01, 0x34, 0x01, 0x34, 0x01, 0x60, 0x00, 0x60,
        0x00, 0x5B, 0x00, 0x5B, 0x00, 0x7D, 0x00, 0x7D, 0x00, 0x5D, 0x00, 0x5D, 0x00, 0xDE, 0x00, 0xDE, 0x00, 0xB5, 0x00, 0xB5, 0x00, 0x35, 0x01, 0x35, 0x01, 0x01, 0x00, 0x01, 0x00, 0xEA, 0x00, 0xEA,
        0x00, 0x27, 0x00, 0x27, 0x00, 0x1E, 0x01, 0x1E, 0x01, 0x36, 0x01, 0x36, 0x01, 0x63, 0x00, 0x63, 0x00, 0x37, 0x01, 0x37, 0x01, 0x38, 0x01, 0x1C, 0x01, 0xCB, 0x00, 0x7E, 0x28, 0x00, 0x10, 0x00,
        0xCA, 0x00, 0xCA, 0x00, 0xB5, 0x00, 0xB5, 0x00, 0x9F, 0x00, 0x9F, 0x00, 0xEB, 0x00, 0xEB, 0x00, 0x69, 0x00, 0x69, 0x00, 0x39, 0x01, 0x39, 0x01, 0x9D, 0x00, 0x9D, 0x00, 0x95, 0x00, 0x95, 0x00,
        0x3A, 0x01, 0x3A, 0x01, 0x3B, 0x01, 0xCA, 0x00, 0xA0, 0x00, 0xA0, 0x00, 0xAA, 0x00, 0xAA, 0x00, 0x3C, 0x01, 0x3C, 0x01, 0x3D, 0x01, 0x3D, 0x01, 0x3E, 0x01, 0x3E, 0x01, 0x9C, 0x00, 0x9C, 0x00,
        0x3F, 0x01, 0x3F, 0x01, 0x40, 0x01, 0x40, 0x01, 0x3B, 0x01, 0x3B, 0x01, 0xBB, 0x00
    };


    static inline const unsigned char*  getUShort(const unsigned char* data, unsigned short& v)
    {
        const unsigned short* src = (const unsigned short*)data;
        v = src[0];
        data += sizeof(unsigned short);
        return data;
    }


    static inline const unsigned char*  getUint(const unsigned char* data, uint32& v)
    {
        const uint32* src = (const uint32*)data;
        v = src[0];
        data += sizeof(uint32);
        return data;
    }


    RenderUtil::FontChar::FontChar()
    {
        m_indexCount = 0;
        m_indices    = 0;
        m_x1 = m_y1 = m_x2 = m_y2 = 0;
    }


    const unsigned char* RenderUtil::FontChar::Init(const unsigned char* data, const float* vertices)
    {
        data        = getUShort(data, m_indexCount);
        m_indices    = (const unsigned short*)data;
        data        += m_indexCount * sizeof(unsigned short);

        for (uint32 i = 0; i < m_indexCount; ++i)
        {
            uint32          index   = m_indices[i];
            const float*    vertex  = &vertices[index * 2];
            //assert( _finite(vertex[0]) );
            //assert( _finite(vertex[1]) );

            if (i == 0)
            {
                m_x1 = m_x2 = vertex[0];
                m_y1 = m_y2 = vertex[1];
            }
            else
            {
                if (vertex[0] < m_x1)
                {
                    m_x1 = vertex[0];
                }
                if (vertex[1] < m_y1)
                {
                    m_y1 = vertex[1];
                }
                if (vertex[0] > m_x2)
                {
                    m_x2 = vertex[0];
                }
                if (vertex[1] > m_y2)
                {
                    m_y2 = vertex[1];
                }
            }
        }

        return data;
    }


    void RenderUtil::FontChar::Render(const float* vertices, RenderUtil* renderUtil, float textScale, float& x, float& y, float posX, float posY, const MCore::RGBAColor& color)
    {
        if (m_indices)
        {
            const uint32    lineCount   = m_indexCount / 2;
            const float     spacing     = (m_x2 - m_x1) + 0.05f;

            AZ::Vector2 p1;
            AZ::Vector2 p2;
            for (uint32 i = 0; i < lineCount; ++i)
            {
                const float* v1 = &vertices[ m_indices[i * 2 + 0] * 2 ];
                const float* v2 = &vertices[ m_indices[i * 2 + 1] * 2 ];
                p1.SetX((v1[0] + x) * textScale + posX);
                p1.SetY((v1[1] + y) * textScale);
                p2.SetX((v2[0] + x) * textScale + posX);
                p2.SetY((v2[1] + y) * textScale);

                renderUtil->Render2DLine(p1.GetX(), -p1.GetY() + posY, p2.GetX(), -p2.GetY() + posY, color);
            }

            x += spacing;
        }
        else
        {
            x += 0.1f;
        }
    }


    RenderUtil::VectorFont::VectorFont(RenderUtil* renderUtil)
    {
        m_renderUtil = renderUtil;
        m_version    = 0;
        m_vcount     = 0;
        m_count      = 0;
        m_vertices   = NULL;

        Init(gFontData);
    }


    RenderUtil::VectorFont::~VectorFont()
    {
        Release();
    }


    void RenderUtil::VectorFont::Release()
    {
        m_version    = 0;
        m_vcount     = 0;
        m_count      = 0;
        m_vertices   = NULL;
    }


    void RenderUtil::VectorFont::Init(const unsigned char* fontData)
    {
        Release();
        if (fontData[0] == 'F' && fontData[1] == 'O' && fontData[2] == 'N' && fontData[3] == 'T')
        {
            fontData += 4;
            fontData = getUint(fontData, m_version);

            if (m_version == FONT_VERSION)
            {
                fontData = getUint(fontData, m_vcount);
                fontData = getUint(fontData, m_count);
                fontData = getUint(fontData, m_icount);

                uint32 vsize    = sizeof(float) * m_vcount * 2;
                m_vertices       = (float*)fontData;
                fontData        += vsize;

                for (uint32 i = 0; i < m_count; ++i)
                {
                    unsigned char c = *fontData++;
                    fontData = m_characters[c].Init(fontData, m_vertices);
                }
            }
        }
    }


    float RenderUtil::VectorFont::CalculateTextWidth(const char* text)
    {
        float textWidth = 0.0f;

        while (*text)
        {
            char codeUnit = *text++;
            textWidth += m_characters[(int)codeUnit].GetWidth();
        }

        return textWidth;
    }


    void RenderUtil::VectorFont::Render(float posX, float posY, float textScale, bool centered, const char* text, const MCore::RGBAColor& color)
    {
        float   x           = 0;
        float   y           = 0;
        float   fontScale   = textScale * 4.0f;// scale the text scale so that we are pixel perfect

        if (centered)
        {
            x = -CalculateTextWidth(text) * 0.5f;
        }

        while (*text)
        {
            const char c = *text++;
            m_characters[(int)c].Render(m_vertices, m_renderUtil, fontScale, x, y, posX, posY, color);
        }
    }
} // namespace MCommon
