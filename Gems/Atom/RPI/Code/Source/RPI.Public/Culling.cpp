/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/Timer.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/Job.h>
#include <Atom_RPI_Traits_Platform.h>

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
#include <MaskedOcclusionCulling/MaskedOcclusionCulling.h>
#endif

//Enables more inner-loop profiling scopes (can create high overhead in telemetry if there are many-many objects in a scene)
//#define AZ_CULL_PROFILE_DETAILED

//Enables more detailed profiling descriptions within the culling system, but adds some performance overhead.
//Enable this to more easily see which jobs are associated with which view.
//#define AZ_CULL_PROFILE_VERBOSE   

namespace AZ
{
    namespace RPI
    {
        AZ_CVAR(bool, r_CullInParallel, true, nullptr, ConsoleFunctorFlags::Null, "");
        AZ_CVAR(uint32_t, r_CullWorkPerBatch, 500, nullptr, ConsoleFunctorFlags::Null, "");

        void DebugDrawWorldCoordinateAxes(AuxGeomDraw* auxGeom)
        {
            auxGeom->DrawCylinder(Vector3(.5, .0, .0), Vector3(1, 0, 0), 0.02f, 1.0f, Colors::Red, AuxGeomDraw::DrawStyle::Solid, AuxGeomDraw::DepthTest::Off);
            auxGeom->DrawCylinder(Vector3(.0, .5, .0), Vector3(0, 1, 0), 0.02f, 1.0f, Colors::Green, AuxGeomDraw::DrawStyle::Solid, AuxGeomDraw::DepthTest::Off);
            auxGeom->DrawCylinder(Vector3(.0, .0, .5), Vector3(0, 0, 1), 0.02f, 1.0f, Colors::Blue, AuxGeomDraw::DrawStyle::Solid, AuxGeomDraw::DepthTest::Off);

            Vector3 axisVerts[] =
            {
                Vector3(0.f, 0.f , 0.f), Vector3(10000.f, 0.f, 0.f),
                Vector3(0.f, 0.f , 0.f), Vector3(0.f, 10000.f, 0.f),
                Vector3(0.f, 0.f , 0.f), Vector3(0.f, 0.f, 10000.f)
            };
            Color colors[] =
            {
                Colors::Red, Colors::Red,
                Colors::Green, Colors::Green,
                Colors::Blue, Colors::Blue
            };
            AuxGeomDraw::AuxGeomDynamicDrawArguments lineArgs;
            lineArgs.m_verts = axisVerts;
            lineArgs.m_vertCount = 6;
            lineArgs.m_colors = colors;
            lineArgs.m_colorCount = lineArgs.m_vertCount;
            lineArgs.m_depthTest = AuxGeomDraw::DepthTest::Off;
            auxGeom->DrawLines(lineArgs);
        }

        void DebugDrawFrustum(const AZ::Frustum& f, AuxGeomDraw* auxGeom, const AZ::Color color, [[maybe_unused]] AZ::u8 lineWidth = 1)
        {
            using namespace ShapeIntersection;

            enum CornerIndices {
                NearTopLeft, NearTopRight, NearBottomLeft, NearBottomRight,
                FarTopLeft, FarTopRight, FarBottomLeft, FarBottomRight
            };
            Vector3 corners[8];

            if (IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Left), corners[NearTopLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Right), corners[NearTopRight]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Left), corners[NearBottomLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Near), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Right), corners[NearBottomRight]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Left), corners[FarTopLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Top), f.GetPlane(Frustum::PlaneId::Right), corners[FarTopRight]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Left), corners[FarBottomLeft]) &&
                IntersectThreePlanes(f.GetPlane(Frustum::PlaneId::Far), f.GetPlane(Frustum::PlaneId::Bottom), f.GetPlane(Frustum::PlaneId::Right), corners[FarBottomRight]))
            {

                uint32_t lineIndices[24]{
                    //near plane
                    NearTopLeft, NearTopRight,
                    NearTopRight, NearBottomRight,
                    NearBottomRight, NearBottomLeft,
                    NearBottomLeft, NearTopLeft,

                    //Far plane
                    FarTopLeft, FarTopRight,
                    FarTopRight, FarBottomRight,
                    FarBottomRight, FarBottomLeft,
                    FarBottomLeft, FarTopLeft,

                    //Near-to-Far connecting lines
                    NearTopLeft, FarTopLeft,
                    NearTopRight, FarTopRight,
                    NearBottomLeft, FarBottomLeft,
                    NearBottomRight, FarBottomRight
                };
                AuxGeomDraw::AuxGeomDynamicIndexedDrawArguments drawArgs;
                drawArgs.m_verts = corners;
                drawArgs.m_vertCount = 8;
                drawArgs.m_indices = lineIndices;
                drawArgs.m_indexCount = 24;
                drawArgs.m_colors = &color;
                drawArgs.m_colorCount = 1;
                auxGeom->DrawLines(drawArgs);

                uint32_t triangleIndices[36]{
                    //near
                    NearBottomLeft, NearTopLeft, NearTopRight,
                    NearBottomLeft, NearTopRight, NearBottomRight,

                    //far
                    FarBottomRight, FarTopRight, FarTopLeft,
                    FarBottomRight, FarTopLeft, FarBottomLeft,

                    //left
                    FarBottomLeft, NearBottomLeft, NearTopLeft,
                    FarBottomLeft, NearTopLeft, FarTopLeft,

                    //right
                    NearBottomRight, NearTopRight, FarTopRight,
                    NearBottomRight, FarTopRight, FarBottomRight,

                    //bottom
                    FarBottomLeft, NearBottomLeft, NearBottomRight,
                    FarBottomLeft, NearBottomRight, FarBottomRight,

                    //top
                    NearTopLeft, FarTopLeft, FarTopRight,
                    NearTopLeft, FarTopRight, NearTopRight
                };
                Color transparentColor(color.GetR(), color.GetG(), color.GetB(), color.GetA() * 0.3f);
                drawArgs.m_indices = triangleIndices;
                drawArgs.m_indexCount = 36;
                drawArgs.m_colors = &transparentColor;
                auxGeom->DrawTriangles(drawArgs);

                // plane normals
                Vector3 planeNormals[] =
                {
                    //near
                    0.25f * (corners[NearBottomLeft] + corners[NearBottomRight] + corners[NearTopLeft] + corners[NearTopRight]),
                    0.25f * (corners[NearBottomLeft] + corners[NearBottomRight] + corners[NearTopLeft] + corners[NearTopRight]) + f.GetPlane(Frustum::PlaneId::Near).GetNormal(),

                    //far
                    0.25f * (corners[FarBottomLeft] + corners[FarBottomRight] + corners[FarTopLeft] + corners[FarTopRight]),
                    0.25f * (corners[FarBottomLeft] + corners[FarBottomRight] + corners[FarTopLeft] + corners[FarTopRight]) + f.GetPlane(Frustum::PlaneId::Far).GetNormal(),

                    //left
                    0.5f * (corners[NearBottomLeft] + corners[NearTopLeft]),
                    0.5f * (corners[NearBottomLeft] + corners[NearTopLeft]) + f.GetPlane(Frustum::PlaneId::Left).GetNormal(),

                    //right
                    0.5f * (corners[NearBottomRight] + corners[NearTopRight]),
                    0.5f * (corners[NearBottomRight] + corners[NearTopRight]) + f.GetPlane(Frustum::PlaneId::Right).GetNormal(),

                    //bottom
                    0.5f * (corners[NearBottomLeft] + corners[NearBottomRight]),
                    0.5f * (corners[NearBottomLeft] + corners[NearBottomRight]) + f.GetPlane(Frustum::PlaneId::Bottom).GetNormal(),

                    //top
                    0.5f * (corners[NearTopLeft] + corners[NearTopRight]),
                    0.5f * (corners[NearTopLeft] + corners[NearTopRight]) + f.GetPlane(Frustum::PlaneId::Top).GetNormal(),
                };
                Color planeNormalColors[] =
                {
                    Colors::Red, Colors::Red,       //near
                    Colors::Green, Colors::Green,   //far
                    Colors::Blue, Colors::Blue,     //left
                    Colors::Orange, Colors::Orange, //right
                    Colors::Pink, Colors::Pink,     //bottom
                    Colors::MediumPurple, Colors::MediumPurple, //top
                };
                AuxGeomDraw::AuxGeomDynamicDrawArguments planeNormalLineArgs;
                planeNormalLineArgs.m_verts = planeNormals;
                planeNormalLineArgs.m_vertCount = 12;
                planeNormalLineArgs.m_colors = planeNormalColors;
                planeNormalLineArgs.m_colorCount = planeNormalLineArgs.m_vertCount;
                planeNormalLineArgs.m_depthTest = AuxGeomDraw::DepthTest::Off;
                auxGeom->DrawLines(planeNormalLineArgs);
            }
            else
            {
                AZ_Assert(false, "invalid frustum, cannot draw");
            }
        }

        CullingDebugContext::~CullingDebugContext()
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_perViewCullStatsMutex);
            for (auto& iter : m_perViewCullStats)
            {
                delete iter.second;
                iter.second = nullptr;
            }
        }

        CullingDebugContext::CullStats& CullingDebugContext::GetCullStatsForView(View* view)
        {
            AZStd::lock_guard<AZStd::mutex> lock(m_perViewCullStatsMutex);
            auto iter = m_perViewCullStats.find(view);
            if (iter != m_perViewCullStats.end())
            {
                AZ_Assert(iter->second->m_name == view->GetName(), "stored view name does not match");
                return *iter->second;
            }
            else
            {
                m_perViewCullStats[view] = aznew CullStats(view->GetName());
                return *m_perViewCullStats[view];
            }
        }

        void CullingDebugContext::ResetCullStats()
        {
            m_numCullablesInScene = 0;

            AZStd::lock_guard<AZStd::mutex> lockCullStats(m_perViewCullStatsMutex);
            for (auto& cullStatsPair : m_perViewCullStats)
            {
                cullStatsPair.second->Reset();
            }
        }

        void CullingScene::RegisterOrUpdateCullable(Cullable& cullable)
        {
            // Multiple threads can call RegisterOrUpdateCullable at the same time
            // since the underlying visScene is thread safe, but if you're inserting or
            // updating between BeginCulling and EndCulling, you'll get non-deterministic
            // results depending on a race condition if you happen to update before or after
            // the culling system starts Enumerating, so use soft_lock_shared here
            m_cullDataConcurrencyCheck.soft_lock_shared();
            m_visScene->InsertOrUpdateEntry(cullable.m_cullData.m_visibilityEntry);
            m_cullDataConcurrencyCheck.soft_unlock_shared();
        }

        void CullingScene::UnregisterCullable(Cullable& cullable)
        {
            // Multiple threads can call RegisterOrUpdateCullable at the same time
            // since the underlying visScene is thread safe, but if you're inserting or
            // updating between BeginCulling and EndCulling, you'll get non-deterministic
            // results depending on a race condition if you happen to update before or after
            // the culling system starts Enumerating, so use soft_lock_shared here
            m_cullDataConcurrencyCheck.soft_lock_shared();
            m_visScene->RemoveEntry(cullable.m_cullData.m_visibilityEntry);
            m_cullDataConcurrencyCheck.soft_unlock_shared();
        }

        uint32_t CullingScene::GetNumCullables() const
        {
            return m_visScene->GetEntryCount();
        }

        class AddObjectsToViewJob final
            : public Job
        {
        public:
            AZ_CLASS_ALLOCATOR(AddObjectsToViewJob, ThreadPoolAllocator, 0);

            struct JobData
            {
                CullingDebugContext* m_debugCtx = nullptr;
                const Scene* m_scene = nullptr;
                View* m_view = nullptr;
                Frustum m_frustum;
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
                MaskedOcclusionCulling* m_maskedOcclusionCulling = nullptr;
#endif
            };

        private:
            const AZStd::shared_ptr<JobData> m_jobData;
            CullingScene::WorkListType m_worklist;

        public:
            AddObjectsToViewJob(const AZStd::shared_ptr<AddObjectsToViewJob::JobData>& jobData, CullingScene::WorkListType& worklist)
                : Job(true, nullptr)        //auto-deletes, no JobContext
                , m_jobData(jobData)
                , m_worklist(worklist)
            {
            }

            //work function
            void Process() override
            {
                AZ_PROFILE_SCOPE(RPI, "AddObjectsToViewJob: Process");

                const View::UsageFlags viewFlags = m_jobData->m_view->GetUsageFlags();
                const RHI::DrawListMask drawListMask = m_jobData->m_view->GetDrawListMask();
                uint32_t numDrawPackets = 0;
                uint32_t numVisibleCullables = 0;

                for (const AzFramework::IVisibilityScene::NodeData& nodeData : m_worklist)
                {
                    //If a node is entirely contained within the frustum, then we can skip the fine grained culling.
                    bool nodeIsContainedInFrustum = ShapeIntersection::Contains(m_jobData->m_frustum, nodeData.m_bounds);

#ifdef AZ_CULL_PROFILE_VERBOSE
                    AZ_PROFILE_SCOPE(RPI, "process node (view: %s, skip fine cull: %d",
                        m_view->GetName().GetCStr(), nodeIsContainedInFrustum ? 1 : 0);
#endif

                    if (nodeIsContainedInFrustum || !m_jobData->m_debugCtx->m_enableFrustumCulling)
                    {
                        //Add all objects within this node to the view, without any extra culling
                        for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                        {
                            {
                                if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                                {
                                    Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);

                                    if ((c->m_cullData.m_drawListMask & drawListMask).none() ||
                                        c->m_cullData.m_hideFlags & viewFlags ||
                                        c->m_cullData.m_scene != m_jobData->m_scene ||       //[GFX_TODO][ATOM-13796] once the IVisibilitySystem supports multiple octree scenes, remove this
                                        c->m_isHidden)
                                    {
                                        continue;
                                    }

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
                                    if (TestOcclusionCulling(visibleEntry) == MaskedOcclusionCulling::CullingResult::VISIBLE)
#endif
                                    {
                                        numDrawPackets += AddLodDataToView(c->m_cullData.m_boundingSphere.GetCenter(), c->m_lodData, *m_jobData->m_view);
                                        ++numVisibleCullables;
                                        c->m_isVisible = true;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        //Do fine-grained culling before adding objects to the view
                        for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                        {
                            if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                            {
                                Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);

                                if ((c->m_cullData.m_drawListMask & drawListMask).none() ||
                                    c->m_cullData.m_hideFlags & viewFlags ||
                                    c->m_cullData.m_scene != m_jobData->m_scene ||       //[GFX_TODO][ATOM-13796] once the IVisibilitySystem supports multiple octree scenes, remove this
                                    c->m_isHidden)
                                {
                                    continue;
                                }

                                IntersectResult res = ShapeIntersection::Classify(m_jobData->m_frustum, c->m_cullData.m_boundingSphere);
                                if (res == IntersectResult::Exterior)
                                {
                                    continue;
                                }
                                else if (res == IntersectResult::Interior || ShapeIntersection::Overlaps(m_jobData->m_frustum, c->m_cullData.m_boundingObb))
                                {
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
                                    if (TestOcclusionCulling(visibleEntry) == MaskedOcclusionCulling::CullingResult::VISIBLE)
#endif
                                    {
                                        numDrawPackets += AddLodDataToView(c->m_cullData.m_boundingSphere.GetCenter(), c->m_lodData, *m_jobData->m_view);
                                        ++numVisibleCullables;
                                        c->m_isVisible = true;
                                    }
                                }
                            }
                        }
                    }

                    if (m_jobData->m_debugCtx->m_debugDraw && (m_jobData->m_view->GetName() == m_jobData->m_debugCtx->m_currentViewSelectionName))
                    {
                        AZ_PROFILE_SCOPE(RPI, "debug draw culling");

                        AuxGeomDrawPtr auxGeomPtr = AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(m_jobData->m_scene);
                        if (auxGeomPtr)
                        {
                            //Draw the node bounds
                            // "Fully visible" nodes are nodes that are fully inside the frustum. "Partially visible" nodes intersect the edges of the frustum.
                            // Since the nodes of an octree have lots of overlapping boxes with coplanar edges, it's easier to view these separately, so
                            // we have a few debug booleans to toggle which ones to draw.
                            if (nodeIsContainedInFrustum && m_jobData->m_debugCtx->m_drawFullyVisibleNodes)
                            {
                                auxGeomPtr->DrawAabb(nodeData.m_bounds, Colors::Lime, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off);
                            }
                            else if (!nodeIsContainedInFrustum && m_jobData->m_debugCtx->m_drawPartiallyVisibleNodes)
                            {
                                auxGeomPtr->DrawAabb(nodeData.m_bounds, Colors::Yellow, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off);
                            }

                            //Draw bounds on individual objects
                            if (m_jobData->m_debugCtx->m_drawBoundingBoxes || m_jobData->m_debugCtx->m_drawBoundingSpheres || m_jobData->m_debugCtx->m_drawLodRadii)
                            {
                                for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                                {
                                    if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                                    {
                                        Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);
                                        if (m_jobData->m_debugCtx->m_drawBoundingBoxes)
                                        {
                                            auxGeomPtr->DrawObb(c->m_cullData.m_boundingObb, Matrix3x4::Identity(),
                                                nodeIsContainedInFrustum ? Colors::Lime : Colors::Yellow, AuxGeomDraw::DrawStyle::Line);
                                        }

                                        if (m_jobData->m_debugCtx->m_drawBoundingSpheres)
                                        {
                                            auxGeomPtr->DrawSphere(c->m_cullData.m_boundingSphere.GetCenter(), c->m_cullData.m_boundingSphere.GetRadius(),
                                                Color(0.5f, 0.5f, 0.5f, 0.3f), AuxGeomDraw::DrawStyle::Shaded);
                                        }

                                        if (m_jobData->m_debugCtx->m_drawLodRadii)
                                        {
                                            auxGeomPtr->DrawSphere(c->m_cullData.m_boundingSphere.GetCenter(),
                                                c->m_lodData.m_lodSelectionRadius,
                                                Color(1.0f, 0.5f, 0.0f, 0.3f), RPI::AuxGeomDraw::DrawStyle::Shaded);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                if (m_jobData->m_debugCtx->m_enableStats)
                {
                    CullingDebugContext::CullStats& cullStats = m_jobData->m_debugCtx->GetCullStatsForView(m_jobData->m_view);

                    //no need for mutex here since these are all atomics
                    cullStats.m_numVisibleDrawPackets += numDrawPackets;
                    cullStats.m_numVisibleCullables += numVisibleCullables;
                    ++cullStats.m_numJobs;
                }
            }

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            MaskedOcclusionCulling::CullingResult TestOcclusionCulling(AzFramework::VisibilityEntry* visibleEntry)
            {
                if (!m_jobData->m_maskedOcclusionCulling)
                {
                    return MaskedOcclusionCulling::CullingResult::VISIBLE;
                }

                if (visibleEntry->m_boundingVolume.Contains(m_jobData->m_view->GetCameraTransform().GetTranslation()))
                {
                    // camera is inside bounding volume
                    return MaskedOcclusionCulling::CullingResult::VISIBLE;
                }

                const Vector3& minBound = visibleEntry->m_boundingVolume.GetMin();
                const Vector3& maxBound = visibleEntry->m_boundingVolume.GetMax();

                // compute bounding volume corners
                Vector4 corners[8];
                corners[0] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), minBound.GetY(), minBound.GetZ(), 1.0f);
                corners[1] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), minBound.GetY(), maxBound.GetZ(), 1.0f);
                corners[2] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), minBound.GetY(), maxBound.GetZ(), 1.0f);
                corners[3] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), minBound.GetY(), minBound.GetZ(), 1.0f);
                corners[4] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), maxBound.GetY(), minBound.GetZ(), 1.0f);
                corners[5] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), maxBound.GetY(), maxBound.GetZ(), 1.0f);
                corners[6] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), maxBound.GetY(), maxBound.GetZ(), 1.0f);
                corners[7] = m_jobData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), maxBound.GetY(), minBound.GetZ(), 1.0f);

                // find min clip-space depth and NDC min/max
                float minDepth = FLT_MAX;
                float ndcMinX = FLT_MAX;
                float ndcMinY = FLT_MAX;
                float ndcMaxX = -FLT_MAX;
                float ndcMaxY = -FLT_MAX;
                for (uint32_t index = 0; index < 8; ++index)
                {
                    minDepth = AZStd::min(minDepth, corners[index].GetW());

                    // convert to NDC
                    corners[index] /= corners[index].GetW();

                    ndcMinX = AZStd::min(ndcMinX, corners[index].GetX());
                    ndcMinY = AZStd::min(ndcMinY, corners[index].GetY());
                    ndcMaxX = AZStd::max(ndcMaxX, corners[index].GetX());
                    ndcMaxY = AZStd::max(ndcMaxY, corners[index].GetY());
                }

                if (minDepth < 0.00000001f)
                {
                    return MaskedOcclusionCulling::VISIBLE;
                }

                // test against the occlusion buffer, which contains only the manually placed occlusion planes
                return m_jobData->m_maskedOcclusionCulling->TestRect(ndcMinX, ndcMinY, ndcMaxX, ndcMaxY, minDepth);
            }
#endif
        };

        void CullingScene::ProcessCullables(const Scene& scene, View& view, AZ::Job& parentJob)
        {
            AZ_PROFILE_SCOPE(RPI, "CullingScene::ProcessCullables() - %s", view.GetName().GetCStr());

            const Matrix4x4& worldToClip = view.GetWorldToClipMatrix();
            Frustum frustum = Frustum::CreateFromMatrixColumnMajor(worldToClip);
            if (m_debugCtx.m_freezeFrustums)
            {
                AZStd::lock_guard<AZStd::mutex> lock(m_debugCtx.m_frozenFrustumsMutex);
                auto iter = m_debugCtx.m_frozenFrustums.find(&view);
                if (iter != m_debugCtx.m_frozenFrustums.end())
                {
                    frustum = iter->second;
                }
            }

            if (m_debugCtx.m_debugDraw && m_debugCtx.m_drawViewFrustum && view.GetName() == m_debugCtx.m_currentViewSelectionName)
            {
                AuxGeomDrawPtr auxGeomPtr = AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(&scene);
                if (auxGeomPtr)
                {
                    DebugDrawFrustum(frustum, auxGeomPtr.get(), AZ::Colors::White);
                }
            }

            if (m_debugCtx.m_enableStats)
            {
                CullingDebugContext::CullStats& cullStats = m_debugCtx.GetCullStatsForView(&view);
                cullStats.m_cameraViewToWorld = view.GetViewToWorldMatrix();
            }

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            // setup occlusion culling, if necessary
            MaskedOcclusionCulling* maskedOcclusionCulling = m_occlusionPlanes.empty() ? nullptr : view.GetMaskedOcclusionCulling();
            if (maskedOcclusionCulling)
            {
                // frustum cull occlusion planes
                using VisibleOcclusionPlane = AZStd::pair<OcclusionPlane, float>;
                AZStd::vector<VisibleOcclusionPlane> visibleOccluders;
                for (const auto& occlusionPlane : m_occlusionPlanes)
                {
                    if (ShapeIntersection::Overlaps(frustum, occlusionPlane.m_aabb))
                    {
                        // occluder is visible, compute view space distance and add to list
                        float depth = (view.GetWorldToViewMatrix() * occlusionPlane.m_aabb.GetMin()).GetZ();
                        depth = AZStd::min(depth, (view.GetWorldToViewMatrix() * occlusionPlane.m_aabb.GetMax()).GetZ());

                        visibleOccluders.push_back(AZStd::make_pair(occlusionPlane, depth));
                    }
                }

                // sort the occlusion planes by view space distance, front-to-back
                AZStd::sort(visibleOccluders.begin(), visibleOccluders.end(), [](const VisibleOcclusionPlane& LHS, const VisibleOcclusionPlane& RHS)
                {
                    return LHS.second > RHS.second;
                });

                for (const VisibleOcclusionPlane& occlusionPlane: visibleOccluders)
                {
                    // convert to clip-space
                    Vector4 projectedBL = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerBL);
                    Vector4 projectedTL = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerTL);
                    Vector4 projectedTR = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerTR);
                    Vector4 projectedBR = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerBR);

                    // store to float array
                    float verts[16];
                    projectedBL.StoreToFloat4(&verts[0]);
                    projectedTL.StoreToFloat4(&verts[4]);
                    projectedTR.StoreToFloat4(&verts[8]);
                    projectedBR.StoreToFloat4(&verts[12]);

                    static uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

                    // render into the occlusion buffer, specifying BACKFACE_NONE so it functions as a double-sided occluder
                    maskedOcclusionCulling->RenderTriangles((float*)verts, indices, 2, nullptr, MaskedOcclusionCulling::BACKFACE_NONE);
                }
            }
#endif

            WorkListType worklist;

            AZStd::shared_ptr<AddObjectsToViewJob::JobData> jobData = AZStd::make_shared<AddObjectsToViewJob::JobData>();
            jobData->m_debugCtx = &m_debugCtx;
            jobData->m_scene = &scene;
            jobData->m_view = &view;
            jobData->m_frustum = frustum;
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            jobData->m_maskedOcclusionCulling = maskedOcclusionCulling;
#endif

            auto nodeVisitorLambda = [jobData, &parentJob, &worklist](const AzFramework::IVisibilityScene::NodeData& nodeData) -> void
            {
                AZ_PROFILE_SCOPE(RPI, "nodeVisitorLambda()");
                AZ_Assert(nodeData.m_entries.size() > 0, "should not get called with 0 entries");
                AZ_Assert(worklist.size() < worklist.capacity(), "we should always have room to push a node on the queue");

                //Queue up a small list of work items (NodeData*) which will be pushed to a worker job (AddObjectsToViewJob) once the queue is full.
                //This reduces the number of jobs in flight, reducing job-system overhead.
                worklist.emplace_back(AZStd::move(nodeData));

                if (worklist.size() == worklist.capacity())
                {
                    //Kick off a job to process the (full) worklist
                    AddObjectsToViewJob* job = aznew AddObjectsToViewJob(jobData, worklist); //pool allocated (cheap), auto-deletes when job finishes
                    worklist.clear();
                    parentJob.SetContinuation(job);
                    job->Start();
                }
            };

            if (m_debugCtx.m_enableFrustumCulling)
            {
                m_visScene->Enumerate(frustum, nodeVisitorLambda);                    
            }
            else
            {
                m_visScene->EnumerateNoCull(nodeVisitorLambda);
            }

            if (worklist.size() > 0)
            {
                AZStd::shared_ptr<AddObjectsToViewJob::JobData> remainingJobData = AZStd::make_shared<AddObjectsToViewJob::JobData>();
                remainingJobData->m_debugCtx = &m_debugCtx;
                remainingJobData->m_scene = &scene;
                remainingJobData->m_view = &view;
                remainingJobData->m_frustum = frustum;
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
                remainingJobData->m_maskedOcclusionCulling = maskedOcclusionCulling;
#endif
                //Kick off a job to process any remaining workitems
                AddObjectsToViewJob* job = aznew AddObjectsToViewJob(remainingJobData, worklist); //pool allocated (cheap), auto-deletes when job finishes
                parentJob.SetContinuation(job);
                job->Start();
            }
        }

        uint32_t AddLodDataToView(const Vector3& pos, const Cullable::LodData& lodData, RPI::View& view)
        {
#ifdef AZ_CULL_PROFILE_DETAILED
            AZ_PROFILE_SCOPE(RPI, "AddLodDataToView");
#endif

            const Matrix4x4& viewToClip = view.GetViewToClipMatrix();
            //the [1][1] element of a perspective projection matrix stores cot(FovY/2) (equal to 2*nearPlaneDistance/nearPlaneHeight),
            //which is used to determine the (vertical) projected size in screen space
            const float yScale = viewToClip.GetElement(1, 1);
            const bool isPerspective = viewToClip.GetElement(3, 3) == 0.f;
            const Vector3 cameraPos = view.GetViewToWorldMatrix().GetTranslation();

            const float approxScreenPercentage = ModelLodUtils::ApproxScreenPercentage(
                pos, lodData.m_lodSelectionRadius, cameraPos, yScale, isPerspective);

            uint32_t numVisibleDrawPackets = 0;

            auto addLodToDrawPacket = [&](const Cullable::LodData::Lod& lod)
            {
#ifdef AZ_CULL_PROFILE_VERBOSE
                AZ_PROFILE_SCOPE(RPI, "add draw packets: %zu", lod.m_drawPackets.size());
#endif
                numVisibleDrawPackets += static_cast<uint32_t>(lod.m_drawPackets.size());   //don't want to pay the cost of aznumeric_cast<> here so using static_cast<> instead
                for (const RHI::DrawPacket* drawPacket : lod.m_drawPackets)
                {
                    view.AddDrawPacket(drawPacket, pos);
                }
            };

            switch (lodData.m_lodConfiguration.m_lodType)
            {
                case Cullable::LodType::SpecificLod:
                    if (lodData.m_lodConfiguration.m_lodOverride < lodData.m_lods.size())
                    {
                        addLodToDrawPacket(lodData.m_lods.at(lodData.m_lodConfiguration.m_lodOverride));
                    }
                    break;
                case Cullable::LodType::ScreenCoverage:
                default:
                    for (const Cullable::LodData::Lod& lod : lodData.m_lods)
                    {
                        // Note that this supports overlapping lod ranges (to suport cross-fading lods, for example)
                        if (approxScreenPercentage >= lod.m_screenCoverageMin && approxScreenPercentage <= lod.m_screenCoverageMax)
                        {
                            addLodToDrawPacket(lod);
                        }
                    }
                    break;
            }

            return numVisibleDrawPackets;
        }

        void CullingScene::Activate(const Scene* parentScene)
        {
            m_parentScene = parentScene;

            AZ_Assert(m_visScene == nullptr, "IVisibilityScene already created for this RPI::Scene");
            char sceneIdBuf[40] = "";
            m_parentScene->GetId().ToString(sceneIdBuf);
            AZ::Name visSceneName(AZStd::string::format("RenderCullScene[%s]", sceneIdBuf));
            m_visScene = AZ::Interface<AzFramework::IVisibilitySystem>::Get()->CreateVisibilityScene(visSceneName);

#ifdef AZ_CULL_DEBUG_ENABLED
            AZ_Assert(CountObjectsInScene() == 0, "The culling system should start with 0 entries in this scene.");
#endif
        }

        void CullingScene::Deactivate()
        {
#ifdef AZ_CULL_DEBUG_ENABLED
            AZ_Assert(CountObjectsInScene() == 0, "All culling entries must be removed from the scene before shutdown.");
#endif
            if (m_visScene)
            {
                AZ::Interface<AzFramework::IVisibilitySystem>::Get()->DestroyVisibilityScene(m_visScene);
                m_visScene = nullptr;
            }
        }

        void CullingScene::BeginCulling(const AZStd::vector<ViewPtr>& views)
        {
            AZ_PROFILE_SCOPE(RPI, "CullingScene: BeginCulling");
            m_cullDataConcurrencyCheck.soft_lock();

            m_debugCtx.ResetCullStats();
            m_debugCtx.m_numCullablesInScene = GetNumCullables();
            AZ::JobCompletion beginCullingCompletion;

            for (auto& view : views)
            {
                const auto cullingLambda = [&view]()
                {
                    view->BeginCulling();
                };

                AZ::Job* cullingJob = AZ::CreateJobFunction(AZStd::move(cullingLambda), true, nullptr);
                cullingJob->SetDependent(&beginCullingCompletion);
                cullingJob->Start();
            }

            beginCullingCompletion.StartAndWaitForCompletion();

            AuxGeomDrawPtr auxGeom;
            if (m_debugCtx.m_debugDraw)
            {
                auxGeom = AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(m_parentScene);
                AZ_Assert(auxGeom, "Invalid AuxGeomFeatureProcessorInterface");

                if (m_debugCtx.m_drawWorldCoordinateAxes)
                {
                    DebugDrawWorldCoordinateAxes(auxGeom.get());
                }
            }

            {
                AZStd::lock_guard<AZStd::mutex> lockFrozenFrustums(m_debugCtx.m_frozenFrustumsMutex);
                if (m_debugCtx.m_freezeFrustums)
                {
                    for (const ViewPtr& viewPtr : views)
                    {
                        auto iter = m_debugCtx.m_frozenFrustums.find(viewPtr.get());
                        if (iter == m_debugCtx.m_frozenFrustums.end())
                        {
                            const Matrix4x4& worldToClip = viewPtr->GetWorldToClipMatrix();
                            Frustum frustum = Frustum::CreateFromMatrixColumnMajor(worldToClip, Frustum::ReverseDepth::True);
                            m_debugCtx.m_frozenFrustums.insert({ viewPtr.get(), frustum });                            
                        }
                    }
                }
                else if(m_debugCtx.m_frozenFrustums.size() > 0)
                {
                    m_debugCtx.m_frozenFrustums.clear();
                }
            }
        }

        void CullingScene::EndCulling()
        {            
            m_cullDataConcurrencyCheck.soft_unlock();
        }

        size_t CullingScene::CountObjectsInScene()
        {
            size_t numObjects = 0;
            m_visScene->EnumerateNoCull(
                [this, &numObjects](const AzFramework::IVisibilityScene::NodeData& nodeData)
                {
                    for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                    {
                        if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                        {
                            Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);
                            if (c->m_cullData.m_scene == m_parentScene)       //[GFX_TODO][ATOM-13796] once the IVisibilitySystem supports multiple octree scenes, remove this
                            {
                                ++numObjects;
                            }
                        }
                    }
                }
            );

            return numObjects;
        }

    } // namespace RPI
} // namespace AZ
