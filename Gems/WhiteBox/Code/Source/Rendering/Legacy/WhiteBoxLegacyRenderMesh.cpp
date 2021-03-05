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

#include "WhiteBox_precompiled.h"

#include "Rendering/WhiteBoxRenderData.h"
#include "Viewport/WhiteBoxViewportConstants.h"
#include "WhiteBoxLegacyRenderMesh.h"

#include <CryCommon/IRenderer.h>
// IRenderMesh.h must be included after
#include <CryCommon/IRenderMesh.h>
#include <CryCommon/MathConversion.h>
#include <I3DEngine.h>
#include <IEntityRenderState.h>
#include <IIndexedMesh.h>

namespace WhiteBox
{
    //! White Box specific RenderNode to provide rendering support for the legacy renderer.
    class LegacyRenderNode : public IRenderNode
    {
    public:
        LegacyRenderNode();
        ~LegacyRenderNode();

        void Create(const WhiteBoxRenderData& renderData, const Matrix34& renderTransform);
        void Destroy();

        // IRenderNode ...
        void Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo) override;
        EERType GetRenderNodeType() override;
        const char* GetName() const override;
        const char* GetEntityClassName() const override;
        Vec3 GetPos(bool worldOnly = true) const override;
        const AABB GetBBox() const override;
        void SetBBox(const AABB& WSBBox) override;
        void OffsetPosition(const Vec3& delta) override;
        void SetMaterial(_smart_ptr<IMaterial> mat) override;
        _smart_ptr<IMaterial> GetMaterial(Vec3* hitPos = nullptr) override;
        _smart_ptr<IMaterial> GetMaterialOverride() override;
        IStatObj* GetEntityStatObj(
            unsigned int partId = 0, unsigned int subPartId = 0, Matrix34A* matrix = nullptr,
            bool returnOnlyVisible = false) override;
        _smart_ptr<IMaterial> GetEntitySlotMaterial(
            unsigned int partId, bool returnOnlyVisible = false, bool* drawNear = nullptr) override;
        float GetMaxViewDist() override;
        void GetMemoryUsage(class ICrySizer* sizer) const override;
        AZ::EntityId GetEntityId() override;

        void UpdateTransformAndBounds(const Matrix34& renderTransform);
        bool UpdateWhiteBoxMaterial(const WhiteBoxMaterial& whiteboxMaterial);
        bool GetMeshVisibility() const;
        void SetMeshVisibility(bool visibility);

    private:
        void SetShadowRenderFlags(bool shadows);
        bool LoadWhiteBoxMaterial(const WhiteBoxMaterial& whiteboxMaterial);
        void SetWhiteBoxMaterialProperties(_smart_ptr<IMaterial> material, const WhiteBoxMaterial& whiteboxMaterial);

        WhiteBoxMaterial m_material;
        AZStd::unique_ptr<IStatObj> m_statObj;
        Matrix34 m_renderTransform;
        AABB m_worldAabb;
        bool m_visible = true;
    };

    LegacyRenderNode::LegacyRenderNode() = default;

    LegacyRenderNode::~LegacyRenderNode()
    {
        Destroy();
    }

    bool LegacyRenderNode::LoadWhiteBoxMaterial(const WhiteBoxMaterial& whiteboxMaterial)
    {
        _smart_ptr<IMaterial> baseMaterial = whiteboxMaterial.m_useTexture
            ? gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("materials/checker_material")
            : gEnv->p3DEngine->GetMaterialManager()->LoadMaterial("materials/solid_material");

        _smart_ptr<IMaterial> material = gEnv->p3DEngine->GetMaterialManager()->CloneMaterial(baseMaterial);

        if (!material)
        {
            return false;
        }

        // customize the material according to the WhiteBoxMaterial properties
        SetWhiteBoxMaterialProperties(material, whiteboxMaterial);
        m_statObj->SetMaterial(material);
        return true;
    }

    bool LegacyRenderNode::UpdateWhiteBoxMaterial(const WhiteBoxMaterial& whiteboxMaterial)
    {
        if (m_material.m_useTexture != whiteboxMaterial.m_useTexture)
        {
            return LoadWhiteBoxMaterial(whiteboxMaterial);
        }
        else
        {
            SetWhiteBoxMaterialProperties(GetMaterial(), whiteboxMaterial);
            return true;
        }
    }

    void LegacyRenderNode::SetWhiteBoxMaterialProperties(
        _smart_ptr<IMaterial> material, const WhiteBoxMaterial& whiteboxMaterial)
    {
        m_material = whiteboxMaterial;

        // rather than checking if each material property differs from the current and new
        // material, simply go through and set each property to the new material properties
        // regardless of whether or not they differ

        // for now, there is only 'tint' but this will be expanded on in the future
        Vec3 tint = AZVec3ToLYVec3(m_material.m_tint);
        material->SetGetMaterialParamVec3("diffuse", tint, false);
    }

    void LegacyRenderNode::Create(const WhiteBoxRenderData& renderData, const Matrix34& renderTransform)
    {
        AZ_PROFILE_FUNCTION(AZ::Debug::ProfileCategory::AzToolsFramework);

        if (!m_statObj)
        {
            m_statObj = AZStd::unique_ptr<IStatObj>(gEnv->p3DEngine->CreateStatObj());

            if (m_statObj)
            {
                m_statObj->AddRef();
            }
        }

        IIndexedMesh* indexedMesh = m_statObj->GetIndexedMesh();
        if (!indexedMesh)
        {
            return;
        }

        // fill mesh
        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "Populate LegacyRenderNode Data");

            const WhiteBoxFaces culledFaceList = BuildCulledWhiteBoxFaces(renderData.m_faces);

            const size_t faceCount = culledFaceList.size();
            const size_t vertCount = faceCount * 3;

            indexedMesh->FreeStreams();
            indexedMesh->SetVertexCount(vertCount);
            indexedMesh->SetFaceCount(faceCount);
            indexedMesh->SetIndexCount(0);
            indexedMesh->SetTexCoordCount(vertCount);

            Vec3* const positions = indexedMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::POSITIONS);
            Vec3* const normals = indexedMesh->GetMesh()->GetStreamPtr<Vec3>(CMesh::NORMALS);
            SMeshTexCoord* const texcoords = indexedMesh->GetMesh()->GetStreamPtr<SMeshTexCoord>(CMesh::TEXCOORDS);
            SMeshFace* const faces = indexedMesh->GetMesh()->GetStreamPtr<SMeshFace>(CMesh::FACES);

            const auto copyFaceToStreams = [positions, normals, texcoords, &culledFaceList](size_t faceIndex)
            {
                size_t vertexIndex = faceIndex * 3;

                const auto copyVertexToStreams = [&vertexIndex, positions, normals, texcoords, &culledFaceList](
                                                     const WhiteBoxVertex& vertex, const AZ::Vector3& normal)
                {
                    positions[vertexIndex] = AZVec3ToLYVec3(vertex.m_position);
                    normals[vertexIndex] = AZVec3ToLYVec3(normal);
                    texcoords[vertexIndex] = SMeshTexCoord(vertex.m_uv.GetX(), vertex.m_uv.GetY());
                    vertexIndex++;
                };

                const WhiteBoxFace& face = culledFaceList[faceIndex];
                copyVertexToStreams(face.m_v1, face.m_normal);
                copyVertexToStreams(face.m_v2, face.m_normal);
                copyVertexToStreams(face.m_v3, face.m_normal);
            };

            for (size_t faceIndex = 0; faceIndex < faceCount; faceIndex++)
            {
                copyFaceToStreams(faceIndex);
            }

            size_t faceIndex = 0;
            for (size_t f = 0; f < faceCount; ++f)
            {
                SMeshFace smeshFace = {}; // zero initialize SMeshFace
                for (size_t v = 0; v < 3; ++v)
                {
                    smeshFace.v[v] = faceIndex++;
                }

                faces[f] = smeshFace;
            }
        }

        indexedMesh->SetSubSetCount(1);
        indexedMesh->SetSubsetMaterialId(0, 0);

        indexedMesh->CalcBBox();
        ///////////////////////////////////////////////

        if (!LoadWhiteBoxMaterial(renderData.m_material))
        {
            return;
        }

        SetShadowRenderFlags(true);

        {
            AZ_PROFILE_SCOPE(AZ::Debug::ProfileCategory::AzToolsFramework, "Optimize IndexedMesh");

#if defined(AZ_PLATFORM_WINDOWS)
            // required to generate the mesh using CMeshCompiler
            indexedMesh->Optimize();
#endif
        }

        m_statObj->Invalidate(false);

        const int statObjFlags = m_statObj->GetFlags();
        m_statObj->SetFlags(statObjFlags | STATIC_OBJECT_DYNAMIC);

        m_renderTransform = renderTransform;

        SetBBox(AABB::CreateTransformedAABB(renderTransform, m_statObj->GetAABB()));

        gEnv->p3DEngine->RegisterEntity(this);
    }

    void LegacyRenderNode::Destroy()
    {
        gEnv->p3DEngine->FreeRenderNodeState(this);
    }

    /*IRenderNode*/
    void LegacyRenderNode::Render(const struct SRendParams& inRenderParams, const struct SRenderingPassInfo& passInfo)
    {
        SRendParams rParams(inRenderParams);
        rParams.pInstance = this;
        rParams.fAlpha = 1.0f;

        rParams.pMatrix = &m_renderTransform;
        rParams.bForceDrawStatic = true;

        if (rParams.pMatrix->IsValid())
        {
            m_statObj->Render(rParams, passInfo);
        }
    }

    /*IRenderNode*/
    EERType LegacyRenderNode::GetRenderNodeType()
    {
        return eERType_StaticMeshRenderComponent;
    }

    /*IRenderNode*/
    const char* LegacyRenderNode::GetName() const
    {
        return "LegacyRenderNode";
    }

    /*IRenderNode*/
    const char* LegacyRenderNode::GetEntityClassName() const
    {
        return "LegacyRenderNode";
    }

    /*IRenderNode*/
    Vec3 LegacyRenderNode::GetPos(bool /*worldOnly = true*/) const
    {
        return m_renderTransform.GetTranslation();
    }

    /*IRenderNode*/
    const AABB LegacyRenderNode::GetBBox() const
    {
        return m_worldAabb;
    }

    /*IRenderNode*/
    void LegacyRenderNode::SetBBox(const AABB& WSBBox)
    {
        m_worldAabb = WSBBox;
    }

    /*IRenderNode*/
    void LegacyRenderNode::OffsetPosition(const Vec3& /*delta*/)
    {
        // todo
    }

    /*IRenderNode*/
    void LegacyRenderNode::SetMaterial(_smart_ptr<IMaterial> /*mat*/)
    {
        // todo
    }

    /*IRenderNode*/
    _smart_ptr<IMaterial> LegacyRenderNode::GetMaterial(Vec3* /*hitPos = nullptr*/)
    {
        if (m_statObj)
        {
            return m_statObj->GetMaterial();
        }

        return nullptr;
    }

    /*IRenderNode*/
    _smart_ptr<IMaterial> LegacyRenderNode::GetMaterialOverride()
    {
        return nullptr;
    }

    /*IRenderNode*/
    float LegacyRenderNode::GetMaxViewDist()
    {
        return FLT_MAX;
    }

    /*IRenderNode*/
    IStatObj* LegacyRenderNode::GetEntityStatObj(
        unsigned int partId, [[maybe_unused]] unsigned int subPartId, Matrix34A* matrix, bool /*returnOnlyVisible*/)
    {
        if (partId == 0)
        {
            if (matrix)
            {
                *matrix = m_renderTransform;
            }

            return m_statObj.get();
        }

        return nullptr;
    }

    /*IRenderNode*/
    _smart_ptr<IMaterial> LegacyRenderNode::GetEntitySlotMaterial(
        unsigned int /*partId*/, bool /*returnOnlyVisible*/, bool* /*drawNear*/)
    {
        return nullptr;
    }

    /*IRenderNode*/
    void LegacyRenderNode::GetMemoryUsage(class ICrySizer* sizer) const
    {
        sizer->AddObjectSize(this);
    }

    /*IRenderNode*/
    AZ::EntityId LegacyRenderNode::GetEntityId()
    {
        // note: may need to store entityId in future
        return AZ::EntityId();
    }

    void LegacyRenderNode::UpdateTransformAndBounds(const Matrix34& renderTransform)
    {
        m_renderTransform = renderTransform;
        SetBBox(AABB::CreateTransformedAABB(renderTransform, m_statObj->GetAABB()));
    }

    void LegacyRenderNode::SetShadowRenderFlags(bool shadows)
    {
        auto flags = GetRndFlags();

        if (shadows)
        {
            // Enable shadows
            flags |= (ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS);
        }
        else
        {
            // Disable shadows
            flags ^= (ERF_CASTSHADOWMAPS | ERF_HAS_CASTSHADOWMAPS);
        }

        SetRndFlags(flags);
    }

    bool LegacyRenderNode::GetMeshVisibility() const
    {
        return m_visible;
    }

    void LegacyRenderNode::SetMeshVisibility(const bool visibility)
    {
        if (visibility != m_visible)
        {
            float opacity = visibility ? 1.0f : 0.0f;
            auto mat = GetMaterial();
            mat->SetGetMaterialParamFloat("opacity", opacity, false);
            SetShadowRenderFlags(visibility);
            m_visible = visibility;
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////

    LegacyRenderMesh::LegacyRenderMesh() = default;

    LegacyRenderMesh::~LegacyRenderMesh()
    {
        if (m_renderNode)
        {
            m_renderNode->Destroy();
            m_renderNode.reset();
        }
    }

    void LegacyRenderMesh::BuildMesh(
        const WhiteBoxRenderData& renderData, const AZ::Transform& worldFromLocal,
        [[maybe_unused]] AZ::EntityId entityId)
    {
        m_renderNode = AZStd::make_unique<LegacyRenderNode>();
        m_renderNode->Create(renderData, AZTransformToLYTransform(worldFromLocal));
    }

    void LegacyRenderMesh::UpdateTransform(const AZ::Transform& worldFromLocal)
    {
        if (m_renderNode)
        {
            m_renderNode->UpdateTransformAndBounds(AZTransformToLYTransform(worldFromLocal));
        }
    }

    void LegacyRenderMesh::UpdateMaterial(const WhiteBoxMaterial& material)
    {
        if (m_renderNode)
        {
            m_renderNode->UpdateWhiteBoxMaterial(material);
        }
    }

    bool LegacyRenderMesh::IsVisible() const
    {
        if (m_renderNode)
        {
            m_renderNode->GetMeshVisibility();
        }

        return false;
    }

    void LegacyRenderMesh::SetVisiblity(bool visibility)
    {
        if (m_renderNode)
        {
            m_renderNode->SetMeshVisibility(visibility);
        }
    }
} // namespace WhiteBox
