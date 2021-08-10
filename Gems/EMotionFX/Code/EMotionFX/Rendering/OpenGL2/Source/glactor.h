/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_GLACTOR_H
#define __RENDERGL_GLACTOR_H

#include "RenderGLConfig.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "RenderTexture.h"
#include "Shader.h"
#include "Material.h"
#include <MCore/Source/Array2D.h>
#include <MCore/Source/MemoryObject.h>
#include <EMotionFX/Source/Mesh.h>

#include <AzCore/RTTI/TypeInfo.h>

#include <AzCore/PlatformIncl.h>
#include <QOpenGLExtraFunctions>

namespace RenderGL
{
    class RENDERGL_API GLActor
        : public MCore::MemoryObject
        , protected QOpenGLExtraFunctions
    {
        MCORE_MEMORYOBJECTCATEGORY(GLActor, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

        AZ_TYPE_INFO(GLActor, "{D59A4DF3-CB73-425A-9234-F547BFF9117E}");

    public:
        enum ERenderFlags
        {
            RENDER_LIGHTING = 1 << 0,
            RENDER_TEXTURING = 1 << 1,
            RENDER_SHADOWS = 1 << 2,
            RENDER_SKINNING = 1 << 3
        };

        static GLActor* Create();

        bool Init(EMotionFX::Actor* actor, const char* texturePath, bool gpuSkinning = true, bool removeGPUSkinnedMeshes = true);

        MCORE_INLINE EMotionFX::Actor* GetActor() { return m_actor; }
        MCORE_INLINE const AZStd::string& GetTexturePath() const { return m_texturePath; }

        void Render(EMotionFX::ActorInstance* actorInstance, uint32 renderFlags = RENDER_LIGHTING | RENDER_TEXTURING);

        const MCore::RGBAColor& GetSkyColor() const                 { return m_skyColor; }
        const MCore::RGBAColor& GetGroundColor() const              { return m_groundColor; }
        void SetGroundColor(const MCore::RGBAColor& color)        { m_groundColor = color; }
        void SetSkyColor(const MCore::RGBAColor& color)           { m_skyColor = color; }

    private:
        struct RENDERGL_API MaterialPrimitives
        {
            Material*               m_material;
            AZStd::vector<Primitive> m_primitives[3];

            MaterialPrimitives()              { m_material = nullptr; m_primitives[0].reserve(64); m_primitives[1].reserve(64); m_primitives[2].reserve(64); }
            MaterialPrimitives(Material* mat) { m_material = mat; m_primitives[0].reserve(64); m_primitives[1].reserve(64); m_primitives[2].reserve(64); }
        };

        AZStd::string                   m_texturePath;
        EMotionFX::Actor*               m_actor;
        bool                            m_enableGpuSkinning;

        void Cleanup();

        void RenderMeshes(EMotionFX::ActorInstance* actorInstance, EMotionFX::Mesh::EMeshType meshType, uint32 renderFlags);
        void RenderShadowMap(EMotionFX::Mesh::EMeshType meshType);
        void InitMaterials(size_t lodLevel);
        Material* InitMaterial(EMotionFX::Material* emfxMaterial);
        void FillIndexBuffers(size_t lodLevel);
        void FillStaticVertexBuffers(size_t lodLevel);
        void FillGPUSkinnedVertexBuffers(size_t lodLevel);

        void UpdateDynamicVertices(EMotionFX::ActorInstance* actorInstance);

        EMotionFX::Mesh::EMeshType ClassifyMeshType(EMotionFX::Node* node, EMotionFX::Mesh* mesh, size_t lodLevel);

        AZStd::vector< AZStd::vector<MaterialPrimitives*> >  m_materials;
        MCore::Array2D<size_t>          m_dynamicNodes;
        MCore::Array2D<Primitive>       m_primitives[3];
        AZStd::vector<bool>              m_homoMaterials;
        AZStd::vector<VertexBuffer*>     m_vertexBuffers[3];
        AZStd::vector<IndexBuffer*>      m_indexBuffers[3];
        MCore::RGBAColor                m_groundColor;
        MCore::RGBAColor                m_skyColor;

        GLActor();
        ~GLActor();

        void Delete() override;
    };
}

#endif
