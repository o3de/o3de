/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            m_vertexOffset = 0;
            m_indexOffset  = 0;
            m_numTriangles = 0;
            m_numVertices  = 0;

            m_nodeIndex     = InvalidIndex;
            m_materialIndex = MCORE_INVALIDINDEX32;
        }

        size_t                  m_nodeIndex;     /**< The index of the node to which this primitive belongs to. */
        uint32                  m_vertexOffset;
        uint32                  m_indexOffset;   /**< The starting index. */
        uint32                  m_numTriangles;  /**< The number of triangles in the primitive. */
        uint32                  m_numVertices;   /**< The number of vertices in the primitive. */
        uint32                  m_materialIndex; /**< The material index which is mapped to the primitive. */

        AZStd::vector<size_t>    m_boneNodeIndices;/**< Mapping from local bones 0-50 to nodes. */
    };


    // StandardVertex
    struct RENDERGL_API StandardVertex
    {
        AZ::Vector3 m_position;
        AZ::Vector3 m_normal;
        AZ::Vector4 m_tangent;
        AZ::Vector2 m_uv;
    };


    // SkinnedVertex
    struct RENDERGL_API SkinnedVertex
    {
        AZ::Vector3  m_position;
        AZ::Vector3  m_normal;
        AZ::Vector4 m_tangent;
        AZ::Vector2 m_uv;
        float           m_weights[4];
        float           m_boneIndices[4];
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

        GLActor* m_actor;
    };
}

#endif
