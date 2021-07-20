/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __RENDERGL_MATERIAL_H
#define __RENDERGL_MATERIAL_H

#include <AzCore/Math/Vector2.h>
#include <MCore/Source/StandardHeaders.h>
#include <EMotionFX/Source/ActorInstance.h>
#include "TextureCache.h"


namespace RenderGL
{
    // forward declaration
    class GLActor;

    struct RENDERGL_API Primitive
    {
        Primitive()
        {
            mVertexOffset = 0;
            mIndexOffset  = 0;
            mNumTriangles = 0;
            mNumVertices  = 0;

            mNodeIndex     = MCORE_INVALIDINDEX32;
            mMaterialIndex = MCORE_INVALIDINDEX32;
        }

        uint32                  mNodeIndex;     /**< The index of the node to which this primitive belongs to. */
        uint32                  mVertexOffset;
        uint32                  mIndexOffset;   /**< The starting index. */
        uint32                  mNumTriangles;  /**< The number of triangles in the primitive. */
        uint32                  mNumVertices;   /**< The number of vertices in the primitive. */
        uint32                  mMaterialIndex; /**< The material index which is mapped to the primitive. */

        MCore::Array<uint32>    mBoneNodeIndices;/**< Mapping from local bones 0-50 to nodes. */
    };


    // StandardVertex
    struct RENDERGL_API StandardVertex
    {
        AZ::Vector3 mPosition;
        AZ::Vector3 mNormal;
        AZ::Vector4 mTangent;
        AZ::Vector2 mUV;
    };


    // SkinnedVertex
    struct RENDERGL_API SkinnedVertex
    {
        AZ::Vector3  mPosition;
        AZ::Vector3  mNormal;
        AZ::Vector4 mTangent;
        AZ::Vector2 mUV;
        float           mWeights[4];
        float           mBoneIndices[4];
    };



    /**
     * Material
     */
    class RENDERGL_API Material
    {
        MCORE_MEMORYOBJECTCATEGORY(Material, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_RENDERING);

    public:
        enum EAttribute
        {
            LIGHTING,
            SKINNING,
            SHADOWS,
            TEXTURING,
            NUM_ATTRIBUTES
        };

        enum EActivationFlags
        {
            GLOBAL  = 1 << 1,
            LOCAL   = 1 << 2
        };

        Material(GLActor* actor);
        virtual ~Material();

        virtual void Activate(uint32 flags) = 0;
        virtual void Deactivate() = 0;
        virtual void Render(EMotionFX::ActorInstance* actorInstance, const Primitive* primitive) = 0;
        virtual void SetAttribute(EAttribute attribute, bool enabled)   { MCORE_UNUSED(attribute); MCORE_UNUSED(enabled); }

    protected:
        Texture* LoadTexture(const char* fileName, bool genMipmaps);
        Texture* LoadTexture(const char* fileName);
        const char* AttributeToString(const EAttribute attribute);

        GLActor* mActor;
    };
}

#endif
