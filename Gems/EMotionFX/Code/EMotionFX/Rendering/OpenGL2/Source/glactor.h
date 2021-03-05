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

namespace RenderGL
{
    class RENDERGL_API GLActor
        : public MCore::MemoryObject
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
            MCore::Array<Primitive> mPrimitives[3];

            MaterialPrimitives()              { mMaterial = nullptr; mPrimitives[0].Reserve(64); mPrimitives[1].Reserve(64); mPrimitives[2].Reserve(64); }
            MaterialPrimitives(Material* mat) { mMaterial = mat; mPrimitives[0].Reserve(64); mPrimitives[1].Reserve(64); mPrimitives[2].Reserve(64); }
        };

        AZStd::string                   mTexturePath;
        EMotionFX::Actor*               mActor;
        bool                            mEnableGPUSkinning;

        void Cleanup();

        void RenderMeshes(EMotionFX::ActorInstance* actorInstance, EMotionFX::Mesh::EMeshType meshType, uint32 renderFlags);
        void RenderShadowMap(EMotionFX::Mesh::EMeshType meshType);
        void InitMaterials(uint32 lodLevel);
        Material* InitMaterial(EMotionFX::Material* emfxMaterial);
        void FillIndexBuffers(uint32 lodLevel);
        void FillStaticVertexBuffers(uint32 lodLevel);
        void FillGPUSkinnedVertexBuffers(uint32 lodLevel);

        void UpdateDynamicVertices(EMotionFX::ActorInstance* actorInstance);

        EMotionFX::Mesh::EMeshType ClassifyMeshType(EMotionFX::Node* node, EMotionFX::Mesh* mesh, uint32 lodLevel);

        MCore::Array< MCore::Array<MaterialPrimitives*> >  mMaterials;
        MCore::Array2D<uint32>          mDynamicNodes;
        MCore::Array2D<Primitive>       mPrimitives[3];
        MCore::Array<bool>              mHomoMaterials;
        MCore::Array<VertexBuffer*>     mVertexBuffers[3];
        MCore::Array<IndexBuffer*>      mIndexBuffers[3];
        MCore::RGBAColor                mGroundColor;
        MCore::RGBAColor                mSkyColor;

        GLActor();
        ~GLActor();

        void Delete() override;
    };
}

#endif
