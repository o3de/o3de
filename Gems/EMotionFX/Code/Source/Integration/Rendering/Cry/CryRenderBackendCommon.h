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

#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/std/parallel/atomic.h>

#include <AzFramework/Asset/SimpleAsset.h>

#include <EMotionFX/Source/SubMesh.h>

#include <IEntityRenderState.h>
#include <IIndexedMesh.h>

struct IStatObj; // Cry mesh wrapper.
struct SSkinningData;

namespace EMotionFX
{
    namespace Integration
    {
        struct Primitive
        {
            AZStd::vector<SMeshBoneMapping_uint16>  m_vertexBoneMappings;
            _smart_ptr<IRenderMesh> m_renderMesh;
            CMesh*                  m_mesh = nullptr;      // Non-null only until asset is finalized.
            bool                    m_isDynamic = false; // Indicates if the mesh is dynamic (e.g. has morph targets)
            bool                    m_useUniqueMesh = false;
            EMotionFX::SubMesh*     m_subMesh = nullptr;

            Primitive() = default;
            ~Primitive()
            {
                delete m_mesh;
            }
        };

        /// Holds render representation for a single LOD.
        struct MeshLOD
        {
            AZStd::vector<Primitive>    m_primitives;
            AZStd::atomic_bool          m_isReady{ false };
            bool                        m_hasDynamicMeshes;

            MeshLOD()
                : m_hasDynamicMeshes(false)
            {
                m_isReady.store(false);
            }

            MeshLOD(MeshLOD&& rhs)
            {
                m_primitives = AZStd::move(rhs.m_primitives);
                m_hasDynamicMeshes = rhs.m_hasDynamicMeshes;
                m_isReady.store(rhs.m_isReady.load());
            }

            ~MeshLOD() = default;
        };
    } // namespace Integration
} // namespace EMotionFX
