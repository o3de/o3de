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

#include <Atom/RPI.Public/AuxGeom/AuxGeomDraw.h>
#include <Atom/RPI.Public/AuxGeom/AuxGeomFeatureProcessorInterface.h>
#include <Atom/RPI.Public/Culling.h>
#include <Atom/RPI.Public/Model/ModelLodUtils.h>
#include <Atom/RPI.Public/RPISystemInterface.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <Atom/RHI/CpuProfiler.h>

#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Casting/numeric_cast.h>

#include <AzCore/std/parallel/lock.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/EventTrace.h>
#include <AzCore/Debug/Timer.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/Job.h>

//Enables more inner-loop profiling scopes (can create high overhead in RadTelemetry if there are many-many objects in a scene)
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
            // since the underlying visScene is thread safe, but we don't want any threads
            // to call RegisterOrUpdateCullable between BeginCulling and EndCulling,
            // because that could cause a mis-match between the result of
            // IVisibilityScene::GetEntryCount and the number of cullables actually in the scene
            // so use soft_lock_shared here
            m_cullDataConcurrencyCheck.soft_lock_shared();
            m_visScene->InsertOrUpdateEntry(cullable.m_cullData.m_visibilityEntry);
            m_cullDataConcurrencyCheck.soft_unlock_shared();
        }

        void CullingScene::UnregisterCullable(Cullable& cullable)
        {
            // Multiple threads can call UnregisterCullable at the same time
            // since the underlying visScene is thread safe, but we don't want any threads
            // to call UnregisterCullable between BeginCulling and EndCulling,
            // because that could cause a mis-match between the result of
            // IVisibilityScene::GetEntryCount and the number of cullables actually in the scene
            // so use soft_lock_shared here
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

        private:
            CullingDebugContext* m_debugCtx;
            const Scene* m_scene;
            View* m_view;
            Frustum m_frustum;
            CullingScene::WorkListType m_worklist;

        public:
            AddObjectsToViewJob(CullingDebugContext& debugCtx, const Scene& scene, View& view, Frustum& frustum, CullingScene::WorkListType& worklist)
                : Job(true, nullptr)        //auto-deletes, no JobContext
                , m_debugCtx(&debugCtx)
                , m_scene(&scene)
                , m_view(&view)
                , m_frustum(frustum)                 //capture by value
                , m_worklist(AZStd::move(worklist))  //capture by value
            {
            }

            //work function
            void Process() override
            {
                AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);

                const View::UsageFlags viewFlags = m_view->GetUsageFlags();
                const RHI::DrawListMask drawListMask = m_view->GetDrawListMask();
                uint32_t numDrawPackets = 0;
                uint32_t numVisibleCullables = 0;

                for (const AzFramework::IVisibilityScene::NodeData& nodeData : m_worklist)
                {
                    //If a node is entirely contained within the frustum, then we can skip the fine grained culling.
                    bool nodeIsContainedInFrustum = ShapeIntersection::Contains(m_frustum, nodeData.m_bounds);

#ifdef AZ_CULL_PROFILE_VERBOSE
                    AZ_PROFILE_SCOPE_DYNAMIC(Debug::ProfileCategory::AzRender, "process node (view: %s, skip fine cull: %d",
                        m_view->GetName().GetCStr(), nodeIsContainedInFrustum ? 1 : 0);
#endif

                    if (nodeIsContainedInFrustum || !m_debugCtx->m_enableFrustumCulling)
                    {
                        //Add all objects within this node to the view, without any extra culling
                        for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                        {
                            if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                            {
                                Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);
                                if ((c->m_cullData.m_drawListMask & drawListMask).none() ||
                                    c->m_cullData.m_hideFlags & viewFlags ||
                                    c->m_cullData.m_scene != m_scene)       //[GFX_TODO][ATOM-13796] once the IVisibilitySystem supports multiple octree scenes, remove this
                                {
                                    continue;
                                }
                                numDrawPackets += AddLodDataToView(c->m_cullData.m_boundingSphere.GetCenter(), c->m_lodData, *m_view);
                                ++numVisibleCullables;
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
                                    c->m_cullData.m_scene != m_scene)       //[GFX_TODO][ATOM-13796] once the IVisibilitySystem supports multiple octree scenes, remove this
                                {
                                    continue;
                                }

                                IntersectResult res = ShapeIntersection::Classify(m_frustum, c->m_cullData.m_boundingSphere);
                                if (res == IntersectResult::Exterior)
                                {
                                    continue;
                                }
                                else if (res == IntersectResult::Interior || ShapeIntersection::Overlaps(m_frustum, c->m_cullData.m_boundingObb))
                                {
                                    numDrawPackets += AddLodDataToView(c->m_cullData.m_boundingSphere.GetCenter(), c->m_lodData, *m_view);
                                    ++numVisibleCullables;
                                }
                            }
                        }
                    }

                    if (m_debugCtx->m_debugDraw && (m_view->GetName() == m_debugCtx->m_currentViewSelectionName))
                    {
                        AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "debug draw culling");

                        AuxGeomDrawPtr auxGeomPtr = AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(m_scene);
                        if (auxGeomPtr)
                        {
                            //Draw the node bounds
                            // "Fully visible" nodes are nodes that are fully inside the frustum. "Partially visible" nodes intersect the edges of the frustum.
                            // Since the nodes of an octree have lots of overlapping boxes with coplanar edges, it's easier to view these separately, so
                            // we have a few debug booleans to toggle which ones to draw.
                            if (nodeIsContainedInFrustum && m_debugCtx->m_drawFullyVisibleNodes)
                            {
                                auxGeomPtr->DrawAabb(nodeData.m_bounds, Colors::Lime, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off);
                            }
                            else if (!nodeIsContainedInFrustum && m_debugCtx->m_drawPartiallyVisibleNodes)
                            {
                                auxGeomPtr->DrawAabb(nodeData.m_bounds, Colors::Yellow, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off);
                            }

                            //Draw bounds on individual objects
                            if (m_debugCtx->m_drawBoundingBoxes || m_debugCtx->m_drawBoundingSpheres || m_debugCtx->m_drawLodRadii)
                            {
                                for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                                {
                                    if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                                    {
                                        Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);
                                        if (m_debugCtx->m_drawBoundingBoxes)
                                        {
                                            auxGeomPtr->DrawObb(c->m_cullData.m_boundingObb, Matrix3x4::Identity(),
                                                nodeIsContainedInFrustum ? Colors::Lime : Colors::Yellow, AuxGeomDraw::DrawStyle::Line);
                                        }

                                        if (m_debugCtx->m_drawBoundingSpheres)
                                        {
                                            auxGeomPtr->DrawSphere(c->m_cullData.m_boundingSphere.GetCenter(), c->m_cullData.m_boundingSphere.GetRadius(),
                                                Color(0.5f, 0.5f, 0.5f, 0.3f), AuxGeomDraw::DrawStyle::Shaded);
                                        }

                                        if (m_debugCtx->m_drawLodRadii)
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

                if (m_debugCtx->m_enableStats)
                {
                    CullingDebugContext::CullStats& cullStats = m_debugCtx->GetCullStatsForView(m_view);

                    //no need for mutex here since these are all atomics
                    cullStats.m_numVisibleDrawPackets += numDrawPackets;
                    cullStats.m_numVisibleCullables += numVisibleCullables;
                    ++cullStats.m_numJobs;
                }
            }
        };

        void CullingScene::ProcessCullables(const Scene& scene, View& view, AZ::Job& parentJob)
        {
            AZ_PROFILE_SCOPE_DYNAMIC(Debug::ProfileCategory::AzRender, "CullingScene::ProcessCullables() - %s", view.GetName().GetCStr());

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

            WorkListType worklist;
            auto nodeVisitorLambda = [this, &scene, &view, &parentJob, &frustum, &worklist](const AzFramework::IVisibilityScene::NodeData& nodeData) -> void
            {
                AZ_PROFILE_SCOPE(Debug::ProfileCategory::AzRender, "nodeVisitorLambda()");
                AZ_Assert(nodeData.m_entries.size() > 0, "should not get called with 0 entries");
                AZ_Assert(worklist.size() < worklist.capacity(), "we should always have room to push a node on the queue");

                //Queue up a small list of work items (NodeData*) which will be pushed to a worker job (AddObjectsToViewJob) once the queue is full.
                //This reduces the number of jobs in flight, reducing job-system overhead.
                worklist.emplace_back(AZStd::move(nodeData));

                if (worklist.size() == worklist.capacity())
                {
                    //Kick off a job to process the (full) worklist
                    AddObjectsToViewJob* job = aznew AddObjectsToViewJob(m_debugCtx, scene, view, frustum, worklist); //pool allocated (cheap), auto-deletes when job finishes
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
                //Kick off a job to process any remaining workitems
                AddObjectsToViewJob* job = aznew AddObjectsToViewJob(m_debugCtx, scene, view, frustum, worklist); //pool allocated (cheap), auto-deletes when job finishes
                parentJob.SetContinuation(job);
                job->Start();
            }
        }

        uint32_t AddLodDataToView(const Vector3& pos, const Cullable::LodData& lodData, RPI::View& view)
        {
#ifdef AZ_CULL_PROFILE_DETAILED
            AZ_PROFILE_FUNCTION(Debug::ProfileCategory::AzRender);
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
                AZ_PROFILE_SCOPE_DYNAMIC(Debug::ProfileCategory::AzRender, "add draw packets: %zu", lod.m_drawPackets.size());
#endif
                numVisibleDrawPackets += static_cast<uint32_t>(lod.m_drawPackets.size());   //don't want to pay the cost of aznumeric_cast<> here so using static_cast<> instead
                for (const RHI::DrawPacket* drawPacket : lod.m_drawPackets)
                {
                    view.AddDrawPacket(drawPacket, pos);
                }
            };

            if (lodData.m_lodOverride == Cullable::NoLodOverride)
            {
                for (const Cullable::LodData::Lod& lod : lodData.m_lods)
                {
                    //Note that this supports overlapping lod ranges (to suport cross-fading lods, for example)
                    if (approxScreenPercentage >= lod.m_screenCoverageMin && approxScreenPercentage <= lod.m_screenCoverageMax)
                    {
                        addLodToDrawPacket(lod);
                    }
                }
            }
            else if(lodData.m_lodOverride < lodData.m_lods.size())
            {
                addLodToDrawPacket(lodData.m_lods.at(lodData.m_lodOverride));
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
            m_cullDataConcurrencyCheck.soft_lock();

            m_debugCtx.ResetCullStats();
            m_debugCtx.m_numCullablesInScene = GetNumCullables();

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
