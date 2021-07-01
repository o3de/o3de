/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Vector2.h>

#include <AzCore/Debug/Timer.h>
#include <EMotionFX/Source/SkinningInfoVertexAttributeLayer.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Mesh.h>
#include <EMotionFX/Source/SubMesh.h>
#include <EMotionFX/Source/MeshDeformerStack.h>
#include "GraphicsManager.h"
#include "glactor.h"
#include "StandardMaterial.h"

namespace RenderGL
{
    // constructor
    GLActor::GLActor()
    {
        mEnableGPUSkinning  = true;
        mActor              = nullptr;
        mEnableGPUSkinning  = true;

        mMaterials.SetMemoryCategory(MEMCATEGORY_RENDERING);

        mHomoMaterials.SetMemoryCategory(MEMCATEGORY_RENDERING);

        for (uint32 i = 0; i < 3; i++)
        {
            mIndexBuffers[i].SetMemoryCategory(MEMCATEGORY_RENDERING);
        }

        mSkyColor    = MCore::RGBAColor(0.55f, 0.55f, 0.55f);
        mGroundColor = MCore::RGBAColor(0.117f, 0.015f, 0.07f);
    }


    // destructor
    GLActor::~GLActor()
    {
        Cleanup();
    }


    //
    GLActor* GLActor::Create()
    {
        return new GLActor();
    }


    // delete the actor
    void GLActor::Delete()
    {
        delete this;
    }


    // get rid of the allocated memory
    void GLActor::Cleanup()
    {
        uint32 i;

        // get rid of all index and vertex buffers
        for (uint32 a = 0; a < 3; ++a)
        {
            // get rid of the given vertex buffers
            const uint32 numVertexBuffers = mVertexBuffers[a].GetLength();
            for (i = 0; i < numVertexBuffers; ++i)
            {
                delete mVertexBuffers[a][i];
            }

            // get rid of the given index buffers
            const uint32 numIndexBuffers = mIndexBuffers[a].GetLength();
            for (i = 0; i < numIndexBuffers; ++i)
            {
                delete mIndexBuffers[a][i];
            }
        }

        // delete all materials
        const uint32 numLOD = mMaterials.GetLength();
        for (uint32 l = 0; l < numLOD; l++)
        {
            const uint32 numMaterials = mMaterials[l].GetLength();
            for (uint32 n = 0; n < numMaterials; n++)
            {
                delete mMaterials[l][n]->mMaterial;
                delete mMaterials[l][n];
            }
        }
    }


    // customize the classify mesh type function
    EMotionFX::Mesh::EMeshType GLActor::ClassifyMeshType(EMotionFX::Node* node, EMotionFX::Mesh* mesh, uint32 lodLevel)
    {
        MCORE_ASSERT(node && mesh);
        return mesh->ClassifyMeshType(lodLevel, mActor, node->GetNodeIndex(), !mEnableGPUSkinning, 4, 200);
    }


    // initialize based on an EMotion FX Actor
    bool GLActor::Init(EMotionFX::Actor* actor, const char* texturePath, bool gpuSkinning, bool removeGPUSkinnedMeshes)
    {
        if (QOpenGLContext::currentContext())
        {
            initializeOpenGLFunctions();
        }
        AZ::Debug::Timer initTimer;
        initTimer.Stamp();

        mActor              = actor;
        mEnableGPUSkinning  = gpuSkinning;
        mTexturePath        = texturePath;

        // get the number of nodes and geometry LOD levels
        const uint32 numGeometryLODLevels   = actor->GetNumLODLevels();
        const uint32 numNodes               = actor->GetNumNodes();

        // set the pre-allocation amount for the number of materials
        mMaterials.Resize(numGeometryLODLevels);

        // resize the vertex and index buffers
        for (uint32 a = 0; a < 3; ++a)
        {
            mVertexBuffers[a].Resize(numGeometryLODLevels);
            mIndexBuffers[a].Resize(numGeometryLODLevels);
            mPrimitives[a].Resize(numGeometryLODLevels);

            // reset the vertex and index buffers
            for (uint32 n = 0; n < numGeometryLODLevels; ++n)
            {
                mVertexBuffers[a][n] = nullptr;
                mIndexBuffers [a][n] = nullptr;
            }
        }

        mHomoMaterials.Resize(numGeometryLODLevels);
        mDynamicNodes.Resize (numGeometryLODLevels);

        EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

        // iterate through the lod levels
        for (uint32 lodLevel = 0; lodLevel < numGeometryLODLevels; ++lodLevel)
        {
            InitMaterials(lodLevel);

            // the total number of vertices
            uint32 totalNumVerts[3]     = { 0, 0, 0 };
            uint32 totalNumIndices[3]   = { 0, 0, 0 };

            // iterate through all nodes
            for (uint32 n = 0; n < numNodes; ++n)
            {
                // get the current node
                EMotionFX::Node* node = skeleton->GetNode(n);

                // get the mesh for the node, if there is any
                EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, n);
                if (mesh == nullptr)
                {
                    continue;
                }

                // skip collision meshes
                if (mesh->GetIsCollisionMesh())
                {
                    continue;
                }

                // classify the mesh type
                EMotionFX::Mesh::EMeshType meshType = ClassifyMeshType(node, mesh, lodLevel);

                // get the number of submeshes and iterate through them
                const uint32 numSubMeshes = mesh->GetNumSubMeshes();
                for (uint32 s = 0; s < numSubMeshes; ++s)
                {
                    // get the current submesh
                    EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(s);

                    // create and add the primitive
                    Primitive newPrimitive;
                    newPrimitive.mNodeIndex     = n;
                    newPrimitive.mNumVertices   = subMesh->GetNumVertices();
                    newPrimitive.mNumTriangles  = subMesh->CalcNumTriangles();  // subMesh->GetNumIndices() / 3;
                    newPrimitive.mIndexOffset   = totalNumIndices[ meshType ];
                    newPrimitive.mVertexOffset  = totalNumVerts[ meshType ];
                    newPrimitive.mMaterialIndex = 0;                            // Since GL actor only uses the default material, we should only pass in 0.

                    // copy over the used bones from the submesh
                    if (subMesh->GetNumBones() > 0)
                    {
                        newPrimitive.mBoneNodeIndices = subMesh->GetBonesArray();
                    }

                    // add to primitive list
                    mPrimitives[meshType].Add(lodLevel, newPrimitive);

                    // add to material list
                    MaterialPrimitives* materialPrims = mMaterials[lodLevel][newPrimitive.mMaterialIndex];
                    materialPrims->mPrimitives[meshType].Add(newPrimitive);

                    totalNumIndices[meshType] += newPrimitive.mNumTriangles * 3;
                    totalNumVerts[meshType] += subMesh->GetNumVertices();
                }


                // add dynamic meshes to the dynamic node list
                if (meshType == EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED)
                {
                    mDynamicNodes.Add(lodLevel, node->GetNodeIndex());
                }
            }

            // create the dynamic vertex buffers
            const uint32 numDynamicBytes = sizeof(StandardVertex) * totalNumVerts[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED];
            if (numDynamicBytes > 0)
            {
                mVertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] = new VertexBuffer();
                mIndexBuffers [EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] = new IndexBuffer();

                const bool vbSuccess = mVertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Init(sizeof(StandardVertex), totalNumVerts[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED], USAGE_DYNAMIC);
                const bool ibSuccess = mIndexBuffers [EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Init(IndexBuffer::INDEXSIZE_32BIT, totalNumIndices[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED], USAGE_STATIC);

                // check if the vertex and index buffers are valid
                if (vbSuccess == false || ibSuccess == false)
                {
                    Cleanup();
                    return false;
                }
            }

            // create the static vertex buffers
            const uint32 numStaticBytes = sizeof(StandardVertex) * totalNumVerts[EMotionFX::Mesh::MESHTYPE_STATIC];
            if (numStaticBytes > 0)
            {
                mVertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] = new VertexBuffer();
                mIndexBuffers [EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] = new IndexBuffer();

                const bool vbSuccess = mVertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Init(sizeof(StandardVertex), totalNumVerts[EMotionFX::Mesh::MESHTYPE_STATIC], USAGE_STATIC);
                const bool ibSuccess = mIndexBuffers [EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Init(IndexBuffer::INDEXSIZE_32BIT, totalNumIndices[EMotionFX::Mesh::MESHTYPE_STATIC], USAGE_STATIC);

                // check if the vertex and index buffers are valid
                if (vbSuccess == false || ibSuccess == false)
                {
                    Cleanup();
                    return false;
                }
            }

            // create the skinned vertex buffers
            const uint32 numSkinnedBytes = sizeof(SkinnedVertex) * totalNumVerts[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED];
            if (numSkinnedBytes > 0)
            {
                mVertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] = new VertexBuffer();
                mIndexBuffers [EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] = new IndexBuffer();

                const bool vbSuccess = mVertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Init(sizeof(SkinnedVertex), totalNumVerts[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED], USAGE_STATIC);
                const bool ibSuccess = mIndexBuffers [EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Init(IndexBuffer::INDEXSIZE_32BIT, totalNumIndices[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED], USAGE_STATIC);

                // check if the vertex and index buffers are valid
                if (vbSuccess == false || ibSuccess == false)
                {
                    Cleanup();
                    return false;
                }
            }

            // fill the vertex and index buffers
            FillIndexBuffers(lodLevel);
            FillStaticVertexBuffers(lodLevel);
            FillGPUSkinnedVertexBuffers(lodLevel);
        }

        // remove the meshes that are skinned by the GPU in case the flag is set
        if (gpuSkinning)
        {
            // iterate through all geometry LOD levels
            for (uint32 lodLevel = 0; lodLevel < numGeometryLODLevels; ++lodLevel)
            {
                // iterate through all nodes
                for (uint32 n = 0; n < numNodes; ++n)
                {
                    // get the current node
                    EMotionFX::Node* node = skeleton->GetNode(n);

                    // get the mesh for the node, if there is any
                    EMotionFX::Mesh* mesh = actor->GetMesh(lodLevel, n);
                    if (mesh == nullptr)
                    {
                        continue;
                    }

                    // skip collision meshes
                    if (mesh->GetIsCollisionMesh())
                    {
                        continue;
                    }

                    // classify the mesh type
                    EMotionFX::Mesh::EMeshType meshType = ClassifyMeshType(node, mesh, lodLevel);

                    // remove the meshes
                    if (meshType != EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED)
                    {
                        if (removeGPUSkinnedMeshes)
                        {
                            actor->RemoveNodeMeshForLOD(lodLevel, n);
                        }
                        else
                        {
                            // Disable all deformers for this mesh, rather than deleting it.
                            EMotionFX::MeshDeformerStack* stack = actor->GetMeshDeformerStack(lodLevel, n);
                            if (stack)
                            {
                                const uint32 numDeformers = stack->GetNumDeformers();
                                for (uint32 d=0; d<numDeformers; ++d)
                                {
                                    EMotionFX::MeshDeformer* deformer = stack->GetDeformer(d);
                                    deformer->SetIsEnabled(false);
                                }
                            } // if stack
                        }
                    } // if mesh is cpu deformed
                }
            }
        }

        const float initTime = initTimer.GetDeltaTimeInSeconds();
        MCore::LogInfo("[OpenGL] Initializing the OpenGL actor took %.2f ms.", initTime * 1000.0f);

        return true;
    }


    // init material
    Material* GLActor::InitMaterial(EMotionFX::Material* emfxMaterial)
    {
        switch (emfxMaterial->GetType())
        {
        case EMotionFX::Material::TYPE_ID:
        case EMotionFX::StandardMaterial::TYPE_ID:
        {
            StandardMaterial* material = new RenderGL::StandardMaterial(this);
            material->Init(emfxMaterial);
            return material;
        }

        default:
            // fallback to standard material
            MCore::LogWarning("[OpenGL] Cannot initialize OpenGL material for material '%s'. Falling back to default material.", emfxMaterial->GetName());
            StandardMaterial* material = new RenderGL::StandardMaterial(this);
            material->Init((EMotionFX::StandardMaterial*)mActor->GetMaterial(0, 0));
            return material;
        }
    }


    // initialize materials
    void GLActor::InitMaterials(uint32 lodLevel)
    {
        // get the number of materials and iterate through them
        const uint32 numMaterials = mActor->GetNumMaterials(lodLevel);
        for (uint32 m = 0; m < numMaterials; ++m)
        {
            EMotionFX::Material* emfxMaterial = mActor->GetMaterial(lodLevel, m);
            Material* material = InitMaterial(emfxMaterial);
            mMaterials[lodLevel].Add( new MaterialPrimitives(material) );
        }
    }


    // render the given actor instance
    void GLActor::Render(EMotionFX::ActorInstance* actorInstance, uint32 renderFlags)
    {
        if (!mActor->IsReady())
        {
            return;
        }

        // make sure our actor instance is valid and that we initialized the gl actor
        assert(mActor && actorInstance);

        // update the dynamic vertices (copy dynamic vertices from system memory into the vertex buffer)
        UpdateDynamicVertices(actorInstance);

        // solid mesh rendering
        //glEnable( GL_CULL_FACE );
        //glCullFace( GL_FRONT );

        // render actor meshes
        //  glPushAttrib( GL_ALL_ATTRIB_BITS );
        glPushAttrib(GL_TEXTURE_BIT);
        RenderMeshes(actorInstance, EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED, renderFlags);
        RenderMeshes(actorInstance, EMotionFX::Mesh::MESHTYPE_STATIC, renderFlags);
        RenderMeshes(actorInstance, EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED, renderFlags);
        glPopAttrib();

        //glDisable(GL_CULL_FACE);
    }


    // render meshes of the given type
    void GLActor::RenderMeshes(EMotionFX::ActorInstance* actorInstance, EMotionFX::Mesh::EMeshType meshType, uint32 renderFlags)
    {
        const uint32 lodLevel     = actorInstance->GetLODLevel();
        const uint32 numMaterials = mMaterials[lodLevel].GetLength();

        if (numMaterials == 0)
        {
            return;
        }

        if (mVertexBuffers[meshType][lodLevel] == nullptr || mIndexBuffers[meshType][lodLevel] == nullptr)
        {
            return;
        }

        if (mVertexBuffers[meshType][lodLevel]->GetBufferID() == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // activate vertex and index buffers
        mVertexBuffers[meshType][lodLevel]->Activate();
        mIndexBuffers[meshType][lodLevel]->Activate();

        // render all the primitives in each material
        for (uint32 n = 0; n < numMaterials; n++)
        {
            const MaterialPrimitives* materialPrims = mMaterials[lodLevel][n];
            const uint32 numPrimitives = materialPrims->mPrimitives[meshType].GetLength();
            if (numPrimitives == 0)
            {
                continue;
            }

            Material* material = materialPrims->mMaterial;
            if (material == nullptr)
            {
                continue;
            }

            material->SetAttribute(Material::LIGHTING,      (renderFlags & RENDER_LIGHTING) != 0);
            material->SetAttribute(Material::TEXTURING,     (renderFlags & RENDER_TEXTURING) != 0);
            material->SetAttribute(Material::SKINNING,      (meshType == EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED));
            material->SetAttribute(Material::SHADOWS,       false);     // self-shadowing disabled

            // if the materials are homogenous, activate globally
            uint32 activationFlags = Material::GLOBAL | Material::LOCAL;
            material->Activate(activationFlags);

            // render all primitives
            for (uint32 i = 0; i < numPrimitives; ++i)
            {
                material->Render(actorInstance, &materialPrims->mPrimitives[meshType][i]);
            }

            material->Deactivate();
        }
    }


    // update the dynamic vertices
    void GLActor::UpdateDynamicVertices(EMotionFX::ActorInstance* actorInstance)
    {
        // get the number of dynamic nodes
        const uint32 lodLevel = actorInstance->GetLODLevel();
        const size_t numNodes = mDynamicNodes.GetNumElements(lodLevel);
        if (numNodes == 0)
        {
            return;
        }

        if (mVertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] == nullptr)
        {
            return;
        }

        // lock the dynamic vertex buffer
        StandardVertex* dynamicVertices = (StandardVertex*)mVertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
        if (dynamicVertices == nullptr)
        {
            return;
        }

        // copy over all vertex data of all dynamic node meshes
        uint32 globalVert = 0;

        // iterate through the nodes
        for (size_t n = 0; n < numNodes; ++n)
        {
            // get the node and its mesh
            const size_t        nodeIndex   = mDynamicNodes.GetElement(lodLevel, n);
            EMotionFX::Mesh*    mesh        = mActor->GetMesh(lodLevel, aznumeric_cast<uint32>(nodeIndex));

            // is the mesh valid?
            if (mesh == nullptr)
            {
                continue;
            }

            // get the mesh data buffers
            const AZ::Vector3* positions   = static_cast<const AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            const AZ::Vector3* normals     = static_cast<const AZ::Vector3*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
            const AZ::Vector4* tangents    = static_cast<const AZ::Vector4*>(mesh->FindVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
            const AZ::Vector2* uvsA        = static_cast<const AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));  // first UV set

            const uint32 numVertices = mesh->GetNumVertices();
            if (uvsA)
            {
                for (uint32 v = 0; v < numVertices; ++v)
                {
                    dynamicVertices[globalVert].mPosition   = positions[v];
                    dynamicVertices[globalVert].mNormal     = normals[v];
                    dynamicVertices[globalVert].mUV         = uvsA[v];
                    dynamicVertices[globalVert].mTangent    = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
            else
            {
                for (uint32 v = 0; v < numVertices; ++v)
                {
                    dynamicVertices[globalVert].mPosition   = positions[v];
                    dynamicVertices[globalVert].mNormal     = normals[v];
                    dynamicVertices[globalVert].mUV         = AZ::Vector2(0.0f, 0.0f);
                    dynamicVertices[globalVert].mTangent    = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
        }

        // unlock the vertex buffer
        mVertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Unlock();
    }


    // fill the index buffers with data
    void GLActor::FillIndexBuffers(uint32 lodLevel)
    {
        // initialize the index buffers
        uint32* staticIndices   = nullptr;
        uint32* dynamicIndices  = nullptr;
        uint32* skinnedIndices  = nullptr;

        // lock the index buffers
        if (mIndexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel])
        {
            staticIndices = (uint32*)mIndexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Lock(LOCK_WRITEONLY);
        }
        if (mIndexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel])
        {
            dynamicIndices = (uint32*)mIndexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
        }
        if (mIndexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel])
        {
            skinnedIndices = (uint32*)mIndexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
        }

        //if (staticIndices == nullptr || dynamicIndices == nullptr || skinnedIndices == nullptr)
        if ((mIndexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] && staticIndices == nullptr) ||
            (mIndexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] && dynamicIndices == nullptr) ||
            (mIndexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] && skinnedIndices == nullptr))
        {
            MCore::LogWarning("[OpenGL] Cannot lock index buffers in GLActor::FillIndexBuffers.");
            return;
        }

        // the total number of vertices
        uint32 totalNumStaticIndices    = 0;
        uint32 totalNumDynamicIndices   = 0;
        uint32 totalNumSkinnedIndices   = 0;
        uint32 dynamicOffset            = 0;
        uint32 staticOffset             = 0;
        uint32 gpuSkinnedOffset         = 0;

        EMotionFX::Skeleton* skeleton = mActor->GetSkeleton();

        // get the number of nodes and iterate through them
        const uint32 numNodes = mActor->GetNumNodes();
        for (uint32 n = 0; n < numNodes; ++n)
        {
            // get the current node
            EMotionFX::Node* node = skeleton->GetNode(n);

            // get the mesh for the node, if there is any
            EMotionFX::Mesh* mesh = mActor->GetMesh(lodLevel, n);
            if (mesh == nullptr)
            {
                continue;
            }

            // skip collision meshes
            if (mesh->GetIsCollisionMesh())
            {
                continue;
            }

            // get the mesh type and the indices
            //const uint32              numIndices  = mesh->GetNumIndices();
            uint32*                     indices     = mesh->GetIndices();
            uint8*                      vertCounts  = mesh->GetPolygonVertexCounts();
            EMotionFX::Mesh::EMeshType  meshType    = ClassifyMeshType(node, mesh, lodLevel);

            // NOTE: this method triangulates polygons when needed internally
            switch (meshType)
            {
            case EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED:
            {
                uint32 polyStartIndex = 0;
                const uint32 numPolys = mesh->GetNumPolygons();
                for (uint32 p = 0; p < numPolys; ++p)
                {
                    const uint32 numPolyVerts = vertCounts[p];
                    for (uint32 v = 2; v < numPolyVerts; v++)
                    {
                        dynamicIndices[totalNumDynamicIndices++] = indices[polyStartIndex]          + dynamicOffset;
                        dynamicIndices[totalNumDynamicIndices++] = indices[polyStartIndex + v - 1]      + dynamicOffset;
                        dynamicIndices[totalNumDynamicIndices++] = indices[polyStartIndex + v]  + dynamicOffset;
                    }
                    polyStartIndex += numPolyVerts;
                }

                //for (uint32 i=0; i<numIndices; ++i)
                //dynamicIndices[totalNumDynamicIndices++] = indices[i] + dynamicOffset;

                dynamicOffset += mesh->GetNumVertices();
                break;
            }

            case EMotionFX::Mesh::MESHTYPE_STATIC:
            {
                uint32 polyStartIndex = 0;
                const uint32 numPolys = mesh->GetNumPolygons();
                for (uint32 p = 0; p < numPolys; ++p)
                {
                    const uint32 numPolyVerts = vertCounts[p];
                    for (uint32 v = 2; v < numPolyVerts; v++)
                    {
                        staticIndices[totalNumStaticIndices++] = indices[polyStartIndex]            + staticOffset;
                        staticIndices[totalNumStaticIndices++] = indices[polyStartIndex + v - 1]        + staticOffset;
                        staticIndices[totalNumStaticIndices++] = indices[polyStartIndex + v]    + staticOffset;
                    }
                    polyStartIndex += numPolyVerts;
                }

                // fill in static index buffers
                //for (uint32 i=0; i<numIndices; ++i)
                //staticIndices[totalNumStaticIndices++] = indices[i] + staticOffset;

                staticOffset += mesh->GetNumVertices();
                break;
            }

            case EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED:
            {
                uint32 polyStartIndex = 0;
                const uint32 numPolys = mesh->GetNumPolygons();
                for (uint32 p = 0; p < numPolys; ++p)
                {
                    const uint32 numPolyVerts = vertCounts[p];
                    for (uint32 v = 2; v < numPolyVerts; v++)
                    {
                        skinnedIndices[totalNumSkinnedIndices++] = indices[polyStartIndex]          + gpuSkinnedOffset;
                        skinnedIndices[totalNumSkinnedIndices++] = indices[polyStartIndex + v - 1]      + gpuSkinnedOffset;
                        skinnedIndices[totalNumSkinnedIndices++] = indices[polyStartIndex + v]  + gpuSkinnedOffset;
                    }
                    polyStartIndex += numPolyVerts;
                }

                // fill in gpu skinned index buffers
                //for (uint32 i=0; i<numIndices; ++i)
                //skinnedIndices[totalNumSkinnedIndices++] = indices[i] + gpuSkinnedOffset;

                gpuSkinnedOffset += mesh->GetNumVertices();
                break;
            }
            }
        }

        // unlock the buffers
        if (staticIndices)
        {
            mIndexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Unlock();
        }
        if (dynamicIndices)
        {
            mIndexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Unlock();
        }
        if (skinnedIndices)
        {
            mIndexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Unlock();
        }
    }


    // fill the static vertex buffer
    void GLActor::FillStaticVertexBuffers(uint32 lodLevel)
    {
        if (mVertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] == nullptr)
        {
            return;
        }

        // get the number of nodes
        const uint32 numNodes = mActor->GetNumNodes();
        if (numNodes == 0)
        {
            return;
        }

        EMotionFX::Skeleton* skeleton = mActor->GetSkeleton();

        // lock the static vertex buffer
        StandardVertex* staticVertices = (StandardVertex*)mVertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Lock(LOCK_WRITEONLY);
        if (staticVertices == nullptr)
        {
            return;
        }

        // copy over all vertex data of all dynamic node meshes
        uint32 globalVert = 0;

        // iterate through all nodes
        for (uint32 n = 0; n < numNodes; ++n)
        {
            // get the current node
            EMotionFX::Node* node = skeleton->GetNode(n);

            // get the mesh for the node, if there is any
            EMotionFX::Mesh* mesh = mActor->GetMesh(lodLevel, n);
            if (mesh == nullptr)
            {
                continue;
            }

            // skip collision meshes
            if (mesh->GetIsCollisionMesh())
            {
                continue;
            }

            // get the mesh type and only do static buffers
            EMotionFX::Mesh::EMeshType meshType = ClassifyMeshType(node, mesh, lodLevel);
            if (meshType != EMotionFX::Mesh::MESHTYPE_STATIC)
            {
                continue;
            }

            // get the mesh data buffers
            const AZ::Vector3* positions   = static_cast<const AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            const AZ::Vector3* normals     = static_cast<const AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
            const AZ::Vector4* tangents    = static_cast<const AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
            const AZ::Vector2* uvsA        = static_cast<const AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));  // first UV set

            if (uvsA)
            {
                const uint32 numVerts = mesh->GetNumVertices();
                for (uint32 v = 0; v < numVerts; ++v)
                {
                    staticVertices[globalVert].mPosition    = positions[v];
                    staticVertices[globalVert].mNormal      = normals[v];
                    staticVertices[globalVert].mUV          = uvsA[v];
                    staticVertices[globalVert].mTangent     = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
            else
            {
                const uint32 numVerts = mesh->GetNumVertices();
                for (uint32 v = 0; v < numVerts; ++v)
                {
                    staticVertices[globalVert].mPosition    = positions[v];
                    staticVertices[globalVert].mNormal      = normals[v];
                    staticVertices[globalVert].mUV          = AZ::Vector2(0.0f, 0.0f);
                    staticVertices[globalVert].mTangent     = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
        }

        // unlock the vertex buffer
        mVertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Unlock();
    }


    // fill the GPU skinned vertex buffer
    void GLActor::FillGPUSkinnedVertexBuffers(uint32 lodLevel)
    {
        if (mVertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] == nullptr)
        {
            return;
        }

        // get the number of dynamic nodes
        const uint32 numNodes = mActor->GetNumNodes();
        if (numNodes == 0)
        {
            return;
        }

        EMotionFX::Skeleton* skeleton = mActor->GetSkeleton();

        // lock the GPU skinned vertex buffer
        SkinnedVertex* skinnedVertices = (SkinnedVertex*)mVertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
        if (skinnedVertices == nullptr)
        {
            return;
        }

        // copy over all vertex data of all dynamic node meshes
        uint32 globalVert = 0;

        // iterate through all nodes
        for (uint32 n = 0; n < numNodes; ++n)
        {
            // get the current node
            EMotionFX::Node* node = skeleton->GetNode(n);

            // get the mesh for the node, if there is any
            EMotionFX::Mesh* mesh = mActor->GetMesh(lodLevel, n);
            if (mesh == nullptr)
            {
                continue;
            }

            // skip collision meshes
            if (mesh->GetIsCollisionMesh())
            {
                continue;
            }

            // get the mesh type and only do gpu skinned buffers
            EMotionFX::Mesh::EMeshType meshType = ClassifyMeshType(node, mesh, lodLevel);
            if (meshType != EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED)
            {
                continue;
            }

            // get the mesh data buffers
            const AZ::Vector3* positions   = static_cast<const AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_POSITIONS));
            const AZ::Vector3* normals     = static_cast<const AZ::Vector3*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_NORMALS));
            const AZ::Vector4* tangents    = static_cast<const AZ::Vector4*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_TANGENTS));
            const AZ::Vector2* uvsA        = static_cast<const AZ::Vector2*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_UVCOORDS, 0));  // first UV set
            const AZ::u32*     orgVerts    = static_cast<const AZ::u32*>(mesh->FindOriginalVertexData(EMotionFX::Mesh::ATTRIB_ORGVTXNUMBERS));

            // find the skinning layer
            EMotionFX::SkinningInfoVertexAttributeLayer* skinningInfo = (EMotionFX::SkinningInfoVertexAttributeLayer*)mesh->FindSharedVertexAttributeLayer(EMotionFX::SkinningInfoVertexAttributeLayer::TYPE_ID);
            assert(skinningInfo);

            // get the number of submeshes and iterate through them
            const uint32 numSubMeshes = mesh->GetNumSubMeshes();
            for (uint32 s = 0; s < numSubMeshes; ++s)
            {
                // get the current submesh and the start vertex
                EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(s);
                const uint32 startVertex = subMesh->GetStartVertex();

                // get the number of vertices and iterate through them
                const uint32 numVertices = subMesh->GetNumVertices();
                for (uint32 v = 0; v < numVertices; ++v)
                {
                    const uint32 meshVertexNr = v + startVertex;
                    const uint32 orgVertex = orgVerts[meshVertexNr];

                    // copy position and normal
                    skinnedVertices[globalVert].mPosition   = positions[meshVertexNr];
                    skinnedVertices[globalVert].mNormal     = normals[meshVertexNr];
                    skinnedVertices[globalVert].mTangent    = (tangents) ? tangents[meshVertexNr] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    skinnedVertices[globalVert].mUV         = (uvsA == nullptr) ? AZ::Vector2(0.0f, 0.0f) : uvsA[meshVertexNr];

                    // get the number of influences and iterate through them
                    const size_t numInfluences = skinningInfo->GetNumInfluences(orgVertex);
                    size_t i;
                    for (i = 0; i < numInfluences; ++i)
                    {
                        // get the influence and its weight and set the indices
                        EMotionFX::SkinInfluence* influence                 = skinningInfo->GetInfluence(orgVertex, i);
                        skinnedVertices[globalVert].mWeights[i]     = influence->GetWeight();
                        const uint32 boneIndex                      = subMesh->FindBoneIndex(influence->GetNodeNr());
                        skinnedVertices[globalVert].mBoneIndices[i] = static_cast<float>(boneIndex);
                        MCORE_ASSERT(boneIndex != MCORE_INVALIDINDEX32);
                    }

                    // reset remaining weights and offsets
                    for (size_t a = i; a < 4; ++a)
                    {
                        skinnedVertices[globalVert].mWeights[a]     = 0.0f;
                        skinnedVertices[globalVert].mBoneIndices[a] = 0;
                    }

                    globalVert++;
                }
            }
        }

        // unlock the vertex buffer
        mVertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Unlock();
    }
}
