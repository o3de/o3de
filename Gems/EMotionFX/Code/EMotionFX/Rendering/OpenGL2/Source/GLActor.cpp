/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        m_enableGpuSkinning  = true;
        m_actor              = nullptr;
        m_enableGpuSkinning  = true;

        m_skyColor    = MCore::RGBAColor(0.55f, 0.55f, 0.55f);
        m_groundColor = MCore::RGBAColor(0.117f, 0.015f, 0.07f);
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
        // get rid of all index and vertex buffers
        for (AZStd::vector<VertexBuffer*>& vertexBuffers : m_vertexBuffers)
        {
            for (VertexBuffer* vertexBuffer : vertexBuffers)
            {
                delete vertexBuffer;
            }
        }
        for (AZStd::vector<IndexBuffer*>& indexBuffers : m_indexBuffers)
        {
            for (IndexBuffer* indexBuffer : indexBuffers)
            {
                delete indexBuffer;
            }
        }

        // delete all materials
        for (AZStd::vector<MaterialPrimitives*>& materialsPerLod : m_materials)
        {
            for (MaterialPrimitives* materialPrimitives : materialsPerLod)
            {
                delete materialPrimitives->m_material;
                delete materialPrimitives;
            }
        }
    }


    // customize the classify mesh type function
    EMotionFX::Mesh::EMeshType GLActor::ClassifyMeshType(EMotionFX::Node* node, EMotionFX::Mesh* mesh, size_t lodLevel)
    {
        MCORE_ASSERT(node && mesh);
        return mesh->ClassifyMeshType(lodLevel, m_actor, node->GetNodeIndex(), !m_enableGpuSkinning, 4, 200);
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

        m_actor              = actor;
        m_enableGpuSkinning  = gpuSkinning;
        m_texturePath        = texturePath;

        // get the number of nodes and geometry LOD levels
        const size_t numGeometryLODLevels   = actor->GetNumLODLevels();
        const size_t numNodes               = actor->GetNumNodes();

        // set the pre-allocation amount for the number of materials
        m_materials.resize(numGeometryLODLevels);

        // resize the vertex and index buffers
        for (AZStd::vector<VertexBuffer*>& vertexBuffers : m_vertexBuffers)
        {
            vertexBuffers.resize(numGeometryLODLevels);
            AZStd::fill(begin(vertexBuffers), end(vertexBuffers), nullptr);
        }
        for (AZStd::vector<IndexBuffer*>& indexBuffers : m_indexBuffers)
        {
            indexBuffers.resize(numGeometryLODLevels);
            AZStd::fill(begin(indexBuffers), end(indexBuffers), nullptr);
        }
        for (MCore::Array2D<Primitive>& primitives : m_primitives)
        {
            primitives.Resize(numGeometryLODLevels);
        }

        m_homoMaterials.resize(numGeometryLODLevels);
        m_dynamicNodes.Resize (numGeometryLODLevels);

        EMotionFX::Skeleton* skeleton = actor->GetSkeleton();

        // iterate through the lod levels
        for (size_t lodLevel = 0; lodLevel < numGeometryLODLevels; ++lodLevel)
        {
            InitMaterials(lodLevel);

            // the total number of vertices
            uint32 totalNumVerts[3]     = { 0, 0, 0 };
            uint32 totalNumIndices[3]   = { 0, 0, 0 };

            // iterate through all nodes
            for (size_t n = 0; n < numNodes; ++n)
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
                const size_t numSubMeshes = mesh->GetNumSubMeshes();
                for (size_t s = 0; s < numSubMeshes; ++s)
                {
                    // get the current submesh
                    EMotionFX::SubMesh* subMesh = mesh->GetSubMesh(s);

                    // create and add the primitive
                    Primitive newPrimitive;
                    newPrimitive.m_nodeIndex     = n;
                    newPrimitive.m_numVertices   = subMesh->GetNumVertices();
                    newPrimitive.m_numTriangles  = subMesh->CalcNumTriangles();  // subMesh->GetNumIndices() / 3;
                    newPrimitive.m_indexOffset   = totalNumIndices[ meshType ];
                    newPrimitive.m_vertexOffset  = totalNumVerts[ meshType ];
                    newPrimitive.m_materialIndex = 0;                            // Since GL actor only uses the default material, we should only pass in 0.

                    // copy over the used bones from the submesh
                    if (subMesh->GetNumBones() > 0)
                    {
                        newPrimitive.m_boneNodeIndices = subMesh->GetBonesArray();
                    }

                    // add to primitive list
                    m_primitives[meshType].Add(lodLevel, newPrimitive);

                    // add to material list
                    MaterialPrimitives* materialPrims = m_materials[lodLevel][newPrimitive.m_materialIndex];
                    materialPrims->m_primitives[meshType].emplace_back(newPrimitive);

                    totalNumIndices[meshType] += newPrimitive.m_numTriangles * 3;
                    totalNumVerts[meshType] += subMesh->GetNumVertices();
                }


                // add dynamic meshes to the dynamic node list
                if (meshType == EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED)
                {
                    m_dynamicNodes.Add(lodLevel, node->GetNodeIndex());
                }
            }

            // create the dynamic vertex buffers
            const size_t numDynamicBytes = sizeof(StandardVertex) * totalNumVerts[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED];
            if (numDynamicBytes > 0)
            {
                m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] = new VertexBuffer();
                m_indexBuffers [EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] = new IndexBuffer();

                const bool vbSuccess = m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Init(sizeof(StandardVertex), totalNumVerts[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED], USAGE_DYNAMIC);
                const bool ibSuccess = m_indexBuffers [EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Init(IndexBuffer::INDEXSIZE_32BIT, totalNumIndices[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED], USAGE_STATIC);

                // check if the vertex and index buffers are valid
                if (vbSuccess == false || ibSuccess == false)
                {
                    Cleanup();
                    return false;
                }
            }

            // create the static vertex buffers
            const size_t numStaticBytes = sizeof(StandardVertex) * totalNumVerts[EMotionFX::Mesh::MESHTYPE_STATIC];
            if (numStaticBytes > 0)
            {
                m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] = new VertexBuffer();
                m_indexBuffers [EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] = new IndexBuffer();

                const bool vbSuccess = m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Init(sizeof(StandardVertex), totalNumVerts[EMotionFX::Mesh::MESHTYPE_STATIC], USAGE_STATIC);
                const bool ibSuccess = m_indexBuffers [EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Init(IndexBuffer::INDEXSIZE_32BIT, totalNumIndices[EMotionFX::Mesh::MESHTYPE_STATIC], USAGE_STATIC);

                // check if the vertex and index buffers are valid
                if (vbSuccess == false || ibSuccess == false)
                {
                    Cleanup();
                    return false;
                }
            }

            // create the skinned vertex buffers
            const size_t numSkinnedBytes = sizeof(SkinnedVertex) * totalNumVerts[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED];
            if (numSkinnedBytes > 0)
            {
                m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] = new VertexBuffer();
                m_indexBuffers [EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] = new IndexBuffer();

                const bool vbSuccess = m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Init(sizeof(SkinnedVertex), totalNumVerts[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED], USAGE_STATIC);
                const bool ibSuccess = m_indexBuffers [EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Init(IndexBuffer::INDEXSIZE_32BIT, totalNumIndices[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED], USAGE_STATIC);

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
            for (size_t lodLevel = 0; lodLevel < numGeometryLODLevels; ++lodLevel)
            {
                // iterate through all nodes
                for (size_t n = 0; n < numNodes; ++n)
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
                                const size_t numDeformers = stack->GetNumDeformers();
                                for (size_t d=0; d<numDeformers; ++d)
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
            material->Init((EMotionFX::StandardMaterial*)m_actor->GetMaterial(0, 0));
            return material;
        }
    }


    // initialize materials
    void GLActor::InitMaterials(size_t lodLevel)
    {
        // get the number of materials and iterate through them
        const size_t numMaterials = m_actor->GetNumMaterials(lodLevel);
        for (size_t m = 0; m < numMaterials; ++m)
        {
            EMotionFX::Material* emfxMaterial = m_actor->GetMaterial(lodLevel, m);
            Material* material = InitMaterial(emfxMaterial);
            m_materials[lodLevel].emplace_back( new MaterialPrimitives(material) );
        }
    }


    // render the given actor instance
    void GLActor::Render(EMotionFX::ActorInstance* actorInstance, uint32 renderFlags)
    {
        if (!m_actor->IsReady())
        {
            return;
        }

        // make sure our actor instance is valid and that we initialized the gl actor
        assert(m_actor && actorInstance);

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
        const size_t lodLevel     = actorInstance->GetLODLevel();
        const size_t numMaterials = m_materials[lodLevel].size();

        if (numMaterials == 0)
        {
            return;
        }

        if (m_vertexBuffers[meshType][lodLevel] == nullptr || m_indexBuffers[meshType][lodLevel] == nullptr)
        {
            return;
        }

        if (m_vertexBuffers[meshType][lodLevel]->GetBufferID() == MCORE_INVALIDINDEX32)
        {
            return;
        }

        // activate vertex and index buffers
        m_vertexBuffers[meshType][lodLevel]->Activate();
        m_indexBuffers[meshType][lodLevel]->Activate();

        // render all the primitives in each material
        for (const MaterialPrimitives* materialPrims : m_materials[lodLevel])
        {
            if (materialPrims->m_primitives[meshType].empty())
            {
                continue;
            }

            Material* material = materialPrims->m_material;
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
            for (const Primitive& primitive : materialPrims->m_primitives[meshType])
            {
                material->Render(actorInstance, &primitive);
            }

            material->Deactivate();
        }
    }


    // update the dynamic vertices
    void GLActor::UpdateDynamicVertices(EMotionFX::ActorInstance* actorInstance)
    {
        // get the number of dynamic nodes
        const size_t lodLevel = actorInstance->GetLODLevel();
        const size_t numNodes = m_dynamicNodes.GetNumElements(lodLevel);
        if (numNodes == 0)
        {
            return;
        }

        if (m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] == nullptr)
        {
            return;
        }

        // lock the dynamic vertex buffer
        StandardVertex* dynamicVertices = (StandardVertex*)m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
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
            const size_t        nodeIndex   = m_dynamicNodes.GetElement(lodLevel, n);
            EMotionFX::Mesh*    mesh        = m_actor->GetMesh(lodLevel, nodeIndex);

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
                    dynamicVertices[globalVert].m_position   = positions[v];
                    dynamicVertices[globalVert].m_normal     = normals[v];
                    dynamicVertices[globalVert].m_uv         = uvsA[v];
                    dynamicVertices[globalVert].m_tangent    = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
            else
            {
                for (uint32 v = 0; v < numVertices; ++v)
                {
                    dynamicVertices[globalVert].m_position   = positions[v];
                    dynamicVertices[globalVert].m_normal     = normals[v];
                    dynamicVertices[globalVert].m_uv         = AZ::Vector2(0.0f, 0.0f);
                    dynamicVertices[globalVert].m_tangent    = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
        }

        // unlock the vertex buffer
        m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Unlock();
    }


    // fill the index buffers with data
    void GLActor::FillIndexBuffers(size_t lodLevel)
    {
        // initialize the index buffers
        uint32* staticIndices   = nullptr;
        uint32* dynamicIndices  = nullptr;
        uint32* skinnedIndices  = nullptr;

        // lock the index buffers
        if (m_indexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel])
        {
            staticIndices = (uint32*)m_indexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Lock(LOCK_WRITEONLY);
        }
        if (m_indexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel])
        {
            dynamicIndices = (uint32*)m_indexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
        }
        if (m_indexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel])
        {
            skinnedIndices = (uint32*)m_indexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
        }

        //if (staticIndices == nullptr || dynamicIndices == nullptr || skinnedIndices == nullptr)
        if ((m_indexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] && staticIndices == nullptr) ||
            (m_indexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel] && dynamicIndices == nullptr) ||
            (m_indexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] && skinnedIndices == nullptr))
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

        EMotionFX::Skeleton* skeleton = m_actor->GetSkeleton();

        // get the number of nodes and iterate through them
        const size_t numNodes = m_actor->GetNumNodes();
        for (size_t n = 0; n < numNodes; ++n)
        {
            // get the current node
            EMotionFX::Node* node = skeleton->GetNode(n);

            // get the mesh for the node, if there is any
            EMotionFX::Mesh* mesh = m_actor->GetMesh(lodLevel, n);
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

                gpuSkinnedOffset += mesh->GetNumVertices();
                break;
            }
            }
        }

        // unlock the buffers
        if (staticIndices)
        {
            m_indexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Unlock();
        }
        if (dynamicIndices)
        {
            m_indexBuffers[EMotionFX::Mesh::MESHTYPE_CPU_DEFORMED][lodLevel]->Unlock();
        }
        if (skinnedIndices)
        {
            m_indexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Unlock();
        }
    }


    // fill the static vertex buffer
    void GLActor::FillStaticVertexBuffers(size_t lodLevel)
    {
        if (m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel] == nullptr)
        {
            return;
        }

        // get the number of nodes
        const size_t numNodes = m_actor->GetNumNodes();
        if (numNodes == 0)
        {
            return;
        }

        EMotionFX::Skeleton* skeleton = m_actor->GetSkeleton();

        // lock the static vertex buffer
        StandardVertex* staticVertices = (StandardVertex*)m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Lock(LOCK_WRITEONLY);
        if (staticVertices == nullptr)
        {
            return;
        }

        // copy over all vertex data of all dynamic node meshes
        uint32 globalVert = 0;

        // iterate through all nodes
        for (size_t n = 0; n < numNodes; ++n)
        {
            // get the current node
            EMotionFX::Node* node = skeleton->GetNode(n);

            // get the mesh for the node, if there is any
            EMotionFX::Mesh* mesh = m_actor->GetMesh(lodLevel, n);
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
                    staticVertices[globalVert].m_position    = positions[v];
                    staticVertices[globalVert].m_normal      = normals[v];
                    staticVertices[globalVert].m_uv          = uvsA[v];
                    staticVertices[globalVert].m_tangent     = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
            else
            {
                const uint32 numVerts = mesh->GetNumVertices();
                for (uint32 v = 0; v < numVerts; ++v)
                {
                    staticVertices[globalVert].m_position    = positions[v];
                    staticVertices[globalVert].m_normal      = normals[v];
                    staticVertices[globalVert].m_uv          = AZ::Vector2(0.0f, 0.0f);
                    staticVertices[globalVert].m_tangent     = (tangents) ? tangents[v] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    globalVert++;
                }
            }
        }

        // unlock the vertex buffer
        m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_STATIC][lodLevel]->Unlock();
    }


    // fill the GPU skinned vertex buffer
    void GLActor::FillGPUSkinnedVertexBuffers(size_t lodLevel)
    {
        if (m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel] == nullptr)
        {
            return;
        }

        // get the number of dynamic nodes
        const size_t numNodes = m_actor->GetNumNodes();
        if (numNodes == 0)
        {
            return;
        }

        EMotionFX::Skeleton* skeleton = m_actor->GetSkeleton();

        // lock the GPU skinned vertex buffer
        SkinnedVertex* skinnedVertices = (SkinnedVertex*)m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Lock(LOCK_WRITEONLY);
        if (skinnedVertices == nullptr)
        {
            return;
        }

        // copy over all vertex data of all dynamic node meshes
        uint32 globalVert = 0;

        // iterate through all nodes
        for (size_t n = 0; n < numNodes; ++n)
        {
            // get the current node
            EMotionFX::Node* node = skeleton->GetNode(n);

            // get the mesh for the node, if there is any
            EMotionFX::Mesh* mesh = m_actor->GetMesh(lodLevel, n);
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
            const size_t numSubMeshes = mesh->GetNumSubMeshes();
            for (size_t s = 0; s < numSubMeshes; ++s)
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
                    skinnedVertices[globalVert].m_position   = positions[meshVertexNr];
                    skinnedVertices[globalVert].m_normal     = normals[meshVertexNr];
                    skinnedVertices[globalVert].m_tangent    = (tangents) ? tangents[meshVertexNr] : AZ::Vector4(0.0f, 0.0f, 1.0f, 1.0f);
                    skinnedVertices[globalVert].m_uv         = (uvsA == nullptr) ? AZ::Vector2(0.0f, 0.0f) : uvsA[meshVertexNr];

                    // get the number of influences and iterate through them
                    const size_t numInfluences = skinningInfo->GetNumInfluences(orgVertex);
                    size_t i;
                    for (i = 0; i < numInfluences; ++i)
                    {
                        // get the influence and its weight and set the indices
                        EMotionFX::SkinInfluence* influence                 = skinningInfo->GetInfluence(orgVertex, i);
                        skinnedVertices[globalVert].m_weights[i]     = influence->GetWeight();
                        const size_t boneIndex                      = subMesh->FindBoneIndex(influence->GetNodeNr());
                        skinnedVertices[globalVert].m_boneIndices[i] = static_cast<float>(boneIndex);
                        MCORE_ASSERT(boneIndex != InvalidIndex);
                    }

                    // reset remaining weights and offsets
                    for (size_t a = i; a < 4; ++a)
                    {
                        skinnedVertices[globalVert].m_weights[a]     = 0.0f;
                        skinnedVertices[globalVert].m_boneIndices[a] = 0;
                    }

                    globalVert++;
                }
            }
        }

        // unlock the vertex buffer
        m_vertexBuffers[EMotionFX::Mesh::MESHTYPE_GPU_DEFORMED][lodLevel]->Unlock();
    }
}
