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

#ifndef __RENDERGL_STANDARD_MATERIAL_H
#define __RENDERGL_STANDARD_MATERIAL_H

#include <AzCore/Math/Transform.h>
#include <EMotionFX/Source/StandardMaterial.h>
#include "Material.h"
#include "GLSLShader.h"
#include "TextureCache.h"


namespace RenderGL
{
    class RENDERGL_API StandardMaterial
        : public Material
    {
        MCORE_MEMORYOBJECTCATEGORY(StandardMaterial, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        StandardMaterial(GLActor* actor);
        virtual ~StandardMaterial();

        void Activate(uint32 flags) override;
        void Deactivate() override;

        bool Init(EMotionFX::Material* material);
        void Render(EMotionFX::ActorInstance* actorInstance, const Primitive* primitive) override;

        void SetAttribute(EAttribute attribute, bool enabled) override;

    protected:
        void UpdateShader();

        bool                            mAttributes[NUM_ATTRIBUTES];
        bool                            mAttributesUpdated;

        GLSLShader*                     mActiveShader;
        MCore::Array<GLSLShader*>       mShaders;
        AZ::Matrix4x4                   mBoneMatrices[200];
        EMotionFX::Material*            mMaterial;

        Texture*                        mDiffuseMap;
        Texture*                        mSpecularMap;
        Texture*                        mNormalMap;
    };
} // namespace RenderGL


#endif
