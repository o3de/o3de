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

        MCORE_INLINE EMotionFX::Actor* GetActor() { return mActor; }
        MCORE_INLINE const AZStd::string& GetTexturePath() const { return mTexturePath; }

        void Render(EMotionFX::ActorInstance* actorInstance, uint32 renderFlags = RENDER_LIGHTING | RENDER_TEXTURING);

        const MCore::RGBAColor& GetSkyColor() const                 { return mSkyColor; }
        const MCore::RGBAColor& GetGroundColor() const              { return mGroundColor; }
        void SetGroundColor(const MCore::RGBAColor& color)        { mGroundColor = color; }
        void SetSkyColor(const MCore::RGBAColor& color)           { mSkyColor = color; }

    private:
        struct RENDERGL_API MaterialPrimitives
        {
            Material*               mMaterial;
            AZStd::vector<Primitive> mPrimitives[3];

            MaterialPrimitives()              { mMaterial = nullptr; mPrimitives[0].reserve(64); mPrimitives[1].reserve(64); mPrimitives[2].reserve(64); }
            MaterialPrimitives(Material* mat) { mMaterial = mat; mPrimitives[0].reserve(64); mPrimitives[1].reserve(64); mPrimitives[2].reserve(64); }
        };

        AZStd::string                   mTexturePath;
        EMotionFX::Actor*               mActor;
        bool                            mEnableGPUSkinning;

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

        AZStd::vector< AZStd::vector<MaterialPrimitives*> >  mMaterials;
        MCore::Array2D<size_t>          mDynamicNodes;
        MCore::Array2D<Primitive>       mPrimitives[3];
        AZStd::vector<bool>              mHomoMaterials;
        AZStd::vector<VertexBuffer*>     mVertexBuffers[3];
        AZStd::vector<IndexBuffer*>      mIndexBuffers[3];
        MCore::RGBAColor                mGroundColor;
        MCore::RGBAColor                mSkyColor;

        GLActor();
        ~GLActor();

        void Delete() override;
    };
}

#endif
