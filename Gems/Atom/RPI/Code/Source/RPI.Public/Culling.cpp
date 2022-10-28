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
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
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

        // Entry work lists
        AZ_CVAR(bool, r_useEntryWorkListsForCulling, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Use entity work lists instead of node work lists for job distribution");
        AZ_CVAR(uint32_t, r_numEntriesPerCullingJob, 750, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls amount of entries to collect for jobs when using entry work lists");

        // Node work lists using entry count
        AZ_CVAR(bool, r_useEntryCountForNodeJobs, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Use entity count instead of node count when checking whether to spawn job for node work list");
        AZ_CVAR(uint32_t, r_maxNodesWhenUsingEntryCount, 100, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls max amount of nodes to collect when using entry count");

        // Node work lists using node count
        AZ_CVAR(uint32_t, r_numNodesPerCullingJob, 25, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls amount of nodes to collect for jobs when not using the entry count");

#ifdef AZ_CULL_DEBUG_ENABLED
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
#endif //AZ_CULL_DEBUG_ENABLED

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


        struct WorklistData
        {
            CullingDebugContext* m_debugCtx = nullptr;
            const Scene* m_scene = nullptr;
            View* m_view = nullptr;
            Frustum m_frustum;
            AZ::Job* m_parentJob = nullptr;
            AZ::TaskGraphEvent* m_taskGraphEvent = nullptr;
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            MaskedOcclusionCulling* m_maskedOcclusionCulling = nullptr;
#endif
#ifdef AZ_CULL_DEBUG_ENABLED
            AuxGeomDrawPtr GetAuxGeomPtr()
            {
                if (m_debugCtx->m_debugDraw && (m_view->GetName() == m_debugCtx->m_currentViewSelectionName))
                {
                    AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(m_scene);
                }
                return nullptr;
            }
#endif
        };

        static AZStd::shared_ptr<WorklistData> MakeWorklistData(
            CullingDebugContext& debugCtx,
            const Scene& scene,
            View& view,
            Frustum& frustum,
            [[maybe_unused]] void* maskedOcclusionCulling,
            AZ::Job* parentJob,
            AZ::TaskGraphEvent* taskGraphEvent)
        {
            AZStd::shared_ptr<WorklistData> worklistData = AZStd::make_shared<WorklistData>();
            worklistData->m_debugCtx = &debugCtx;
            worklistData->m_scene = &scene;
            worklistData->m_view = &view;
            worklistData->m_frustum = frustum;
            worklistData->m_parentJob = parentJob;
            worklistData->m_taskGraphEvent = taskGraphEvent;
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            worklistData->m_maskedOcclusionCulling = static_cast<MaskedOcclusionCulling*>(maskedOcclusionCulling);
#endif
            return worklistData;
        }

        // Used to accumulate NodeData into lists to be handed off to jobs for processing
        struct WorkListType
        {
            void Init()
            {
                m_entryCount = 0;
                u32 reserveCount = r_useEntryCountForNodeJobs ? r_maxNodesWhenUsingEntryCount : r_numNodesPerCullingJob;
                m_nodes.reserve(reserveCount);
            }

            u32 m_entryCount = 0;
            AZStd::vector<AzFramework::IVisibilityScene::NodeData> m_nodes;
        };

        // Used to accumulate VisibilityEntry into lists to be handed off to jobs for processing
        struct EntryListType
        {
            AZStd::vector<AzFramework::VisibilityEntry*> m_entries;
        };

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
        static MaskedOcclusionCulling::CullingResult TestOcclusionCulling(
                    const AZStd::shared_ptr<WorklistData>& worklistData,
                    AzFramework::VisibilityEntry* visibleEntry);
#endif

        static void ProcessEntrylist(const AZStd::shared_ptr<WorklistData>& worklistData, const AZStd::vector<AzFramework::VisibilityEntry*>& entries, bool parentNodeContainedInFrustum = false, s32 startIdx = 0, s32 endIdx = -1)
        {
#ifdef AZ_CULL_DEBUG_ENABLED
            // These variable are only used for the gathering of debug information.
            uint32_t numDrawPackets = 0;
            uint32_t numVisibleCullables = 0;
#endif
            endIdx = (endIdx == -1) ? s32(entries.size()) : endIdx;

            for (s32 i = startIdx; i < endIdx; ++i)
            {
                AzFramework::VisibilityEntry* visibleEntry = entries[i];

                if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                {
                    Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);

                    if ((c->m_cullData.m_drawListMask & worklistData->m_view->GetDrawListMask()).none() ||
                        c->m_cullData.m_hideFlags & worklistData->m_view->GetUsageFlags() ||
                        c->m_isHidden)
                    {
                        continue;
                    }

                    if (!parentNodeContainedInFrustum)
                    {
                        IntersectResult res = ShapeIntersection::Classify(worklistData->m_frustum, c->m_cullData.m_boundingSphere);
                        bool entryInFrustum = (res != IntersectResult::Exterior) && (res == IntersectResult::Interior || ShapeIntersection::Overlaps(worklistData->m_frustum, c->m_cullData.m_boundingObb));
                        if (!entryInFrustum)
                        {
                            continue;
                        }
                    }

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
                    if (TestOcclusionCulling(worklistData, visibleEntry) == MaskedOcclusionCulling::CullingResult::VISIBLE)
#endif
                    {
                        // There are ways to write this without [[maybe_unused]], but they are brittle.
                        // For example, using #else could cause a bug where the function's parameter
                        // is changed in #ifdef but not in #else.
                        [[maybe_unused]]
                        const uint32_t drawPacketCount = AddLodDataToView(c->m_cullData.m_boundingSphere.GetCenter(), c->m_lodData, *worklistData->m_view);
                        c->m_isVisible = true;

#ifdef AZ_CULL_DEBUG_ENABLED
                        ++numVisibleCullables;
                        numDrawPackets += drawPacketCount;
#endif
                    }
                }
            }

#ifdef AZ_CULL_DEBUG_ENABLED
            AuxGeomDrawPtr auxGeomPtr = worklistData->GetAuxGeomPtr();
            if (auxGeomPtr)
            {
                //Draw bounds on individual objects
                if (worklistData->m_debugCtx->m_drawBoundingBoxes || worklistData->m_debugCtx->m_drawBoundingSpheres || worklistData->m_debugCtx->m_drawLodRadii)
                {
                    for (AzFramework::VisibilityEntry* visibleEntry : entries)
                    {
                        if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                        {
                            Cullable* c = static_cast<Cullable*>(visibleEntry->m_userData);
                            if (worklistData->m_debugCtx->m_drawBoundingBoxes)
                            {
                                auxGeomPtr->DrawObb(c->m_cullData.m_boundingObb, Matrix3x4::Identity(),
                                    parentNodeContainedInFrustum ? Colors::Lime : Colors::Yellow, AuxGeomDraw::DrawStyle::Line);
                            }

                            if (worklistData->m_debugCtx->m_drawBoundingSpheres)
                            {
                                auxGeomPtr->DrawSphere(c->m_cullData.m_boundingSphere.GetCenter(), c->m_cullData.m_boundingSphere.GetRadius(),
                                    Color(0.5f, 0.5f, 0.5f, 0.3f), AuxGeomDraw::DrawStyle::Shaded);
                            }

                            if (worklistData->m_debugCtx->m_drawLodRadii)
                            {
                                auxGeomPtr->DrawSphere(c->m_cullData.m_boundingSphere.GetCenter(),
                                    c->m_lodData.m_lodSelectionRadius,
                                    Color(1.0f, 0.5f, 0.0f, 0.3f), RPI::AuxGeomDraw::DrawStyle::Shaded);
                            }
                        }
                    }
                }
            }
            if (worklistData->m_debugCtx->m_enableStats)
            {
                CullingDebugContext::CullStats& cullStats = worklistData->m_debugCtx->GetCullStatsForView(worklistData->m_view);

                //no need for mutex here since these are all atomics
                cullStats.m_numVisibleDrawPackets += numDrawPackets;
                cullStats.m_numVisibleCullables += numVisibleCullables;
                ++cullStats.m_numJobs;
            }
#endif
        }

        static void ProcessVisibilityNode(const AZStd::shared_ptr<WorklistData>& worklistData, const AzFramework::IVisibilityScene::NodeData& nodeData)
        {
            bool nodeIsContainedInFrustum = !worklistData->m_debugCtx->m_enableFrustumCulling || ShapeIntersection::Contains(worklistData->m_frustum, nodeData.m_bounds);

            s32 startIdx = 0, size = s32(nodeData.m_entries.size());
            const AZStd::vector<AzFramework::VisibilityEntry*>& entries = nodeData.m_entries;

            if (worklistData->m_taskGraphEvent)
            {
                static const AZ::TaskDescriptor descriptor{ "AZ::RPI::ProcessWorklist", "Graphics" };

                AZ::TaskGraph taskGraph{ "ProcessCullableEntries" };
                AZ::TaskGraphEvent taskGraphEvent{ "ProcessCullableEntries Wait" };

                while (r_useEntryCountForNodeJobs && (size - startIdx) > s32(r_numEntriesPerCullingJob))
                {
                    taskGraph.AddTask(descriptor, [=, &entries]() -> void
                        {
                            ProcessEntrylist(worklistData, entries, nodeIsContainedInFrustum, startIdx, startIdx + r_numEntriesPerCullingJob);
                        });
                    startIdx += s32(r_numEntriesPerCullingJob);
                }

                if (!taskGraph.IsEmpty())
                {
                    taskGraph.Detach();
                    taskGraph.Submit(worklistData->m_taskGraphEvent);
                }

                ProcessEntrylist(worklistData, nodeData.m_entries, nodeIsContainedInFrustum, startIdx, size);
            }
            else    // Use job system
            {
                while (r_useEntryCountForNodeJobs && (size - startIdx) > s32(r_numEntriesPerCullingJob))
                {
                    auto processEntries = [=, &entries]() -> void
                    {
                        ProcessEntrylist(worklistData, entries, nodeIsContainedInFrustum, startIdx, startIdx + r_numEntriesPerCullingJob);
                    };

                    AZ::Job* job = AZ::CreateJobFunction(AZStd::move(processEntries), true);
                    worklistData->m_parentJob->SetContinuation(job);
                    job->Start();

                    startIdx += s32(r_numEntriesPerCullingJob);
                }

                ProcessEntrylist(worklistData, nodeData.m_entries, nodeIsContainedInFrustum, startIdx, size);
            }

#ifdef AZ_CULL_DEBUG_ENABLED

            //Draw the node bounds
            // "Fully visible" nodes are nodes that are fully inside the frustum. "Partially visible" nodes intersect the edges of the frustum.
            // Since the nodes of an octree have lots of overlapping boxes with coplanar edges, it's easier to view these separately, so
            // we have a few debug booleans to toggle which ones to draw.

            AuxGeomDrawPtr auxGeomPtr = worklistData->GetAuxGeomPtr();
            if (auxGeomPtr)
            {
                if (nodeIsContainedInFrustum && worklistData->m_debugCtx->m_drawFullyVisibleNodes)
                {
                    auxGeomPtr->DrawAabb(nodeData.m_bounds, Colors::Lime, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off);
                }
                else if (!nodeIsContainedInFrustum && worklistData->m_debugCtx->m_drawPartiallyVisibleNodes)
                {
                    auxGeomPtr->DrawAabb(nodeData.m_bounds, Colors::Yellow, RPI::AuxGeomDraw::DrawStyle::Line, RPI::AuxGeomDraw::DepthTest::Off);
                }
            }
#endif
        }

        static void ProcessWorklist(const AZStd::shared_ptr<WorklistData>& worklistData, const WorkListType& worklist)
        {
            AZ_PROFILE_SCOPE(RPI, "Culling: ProcessWorklist");

            AZ_Assert(worklist.m_nodes.size() > 0, "Received empty worklist in ProcessWorklist");

            for (const AzFramework::IVisibilityScene::NodeData& nodeData : worklist.m_nodes)
            {
                ProcessVisibilityNode(worklistData, nodeData);
            }
        }

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
        static MaskedOcclusionCulling::CullingResult TestOcclusionCulling(
            const AZStd::shared_ptr<WorklistData>& worklistData,
            AzFramework::VisibilityEntry* visibleEntry)
        {
            if (!worklistData->m_maskedOcclusionCulling)
            {
                return MaskedOcclusionCulling::CullingResult::VISIBLE;
            }

#ifdef AZ_CULL_PROFILE_VERBOSE
            AZ_PROFILE_SCOPE(RPI, "TestOcclusionCulling");
#endif

            if (visibleEntry->m_boundingVolume.Contains(worklistData->m_view->GetCameraTransform().GetTranslation()))
            {
                // camera is inside bounding volume
                return MaskedOcclusionCulling::CullingResult::VISIBLE;
            }

            const Vector3& minBound = visibleEntry->m_boundingVolume.GetMin();
            const Vector3& maxBound = visibleEntry->m_boundingVolume.GetMax();

            // compute bounding volume corners
            Vector4 corners[8];
            corners[0] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), minBound.GetY(), minBound.GetZ(), 1.0f);
            corners[1] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), minBound.GetY(), maxBound.GetZ(), 1.0f);
            corners[2] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), minBound.GetY(), maxBound.GetZ(), 1.0f);
            corners[3] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), minBound.GetY(), minBound.GetZ(), 1.0f);
            corners[4] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), maxBound.GetY(), minBound.GetZ(), 1.0f);
            corners[5] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(minBound.GetX(), maxBound.GetY(), maxBound.GetZ(), 1.0f);
            corners[6] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), maxBound.GetY(), maxBound.GetZ(), 1.0f);
            corners[7] = worklistData->m_view->GetWorldToClipMatrix() * Vector4(maxBound.GetX(), maxBound.GetY(), minBound.GetZ(), 1.0f);

            // find min clip-space depth and NDC min/max
            float minDepth = FLT_MAX;
            float ndcMinX = FLT_MAX;
            float ndcMinY = FLT_MAX;
            float ndcMaxX = -FLT_MAX;
            float ndcMaxY = -FLT_MAX;
            for (uint32_t index = 0; index < 8; ++index)
            {
                minDepth = AZStd::min(minDepth, corners[index].GetW());
                if (minDepth < 0.00000001f)
                {
                    return MaskedOcclusionCulling::CullingResult::VISIBLE;
                }


                // convert to NDC
                corners[index] /= corners[index].GetW();

                ndcMinX = AZStd::min(ndcMinX, corners[index].GetX());
                ndcMinY = AZStd::min(ndcMinY, corners[index].GetY());
                ndcMaxX = AZStd::max(ndcMaxX, corners[index].GetX());
                ndcMaxY = AZStd::max(ndcMaxY, corners[index].GetY());
            }

            // test against the occlusion buffer, which contains only the manually placed occlusion planes
            return worklistData->m_maskedOcclusionCulling->TestRect(ndcMinX, ndcMinY, ndcMaxX, ndcMaxY, minDepth);
        }
#endif

        void CullingScene::ProcessCullablesCommon(
            const Scene& scene [[maybe_unused]],
            View& view,
            AZ::Frustum& frustum [[maybe_unused]],
            void*& maskedOcclusionCulling [[maybe_unused]])
        {
            AZ_PROFILE_SCOPE(RPI, "CullingScene::ProcessCullablesCommon() - %s", view.GetName().GetCStr());

#ifdef AZ_CULL_DEBUG_ENABLED
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
                    auxGeomPtr->DrawFrustum(frustum, AZ::Colors::White);
                }
            }

            if (m_debugCtx.m_enableStats)
            {
                CullingDebugContext::CullStats& cullStats = m_debugCtx.GetCullStatsForView(&view);
                cullStats.m_cameraViewToWorld = view.GetViewToWorldMatrix();
            }
#endif //AZ_CULL_DEBUG_ENABLED
#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            // setup occlusion culling, if necessary
            maskedOcclusionCulling = m_occlusionPlanes.empty() ? nullptr : view.GetMaskedOcclusionCulling();
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
                    static_cast<MaskedOcclusionCulling*>(maskedOcclusionCulling)->RenderTriangles(verts, indices, 2, nullptr, MaskedOcclusionCulling::BACKFACE_NONE);
                }
            }
#endif
        }

        void CullingScene::ProcessCullables(const Scene& scene, View& view, AZ::Job* parentJob, AZ::TaskGraph* taskGraph, AZ::TaskGraphEvent* taskGraphEvent)
        {
            AZ_PROFILE_SCOPE(RPI, "CullingScene::ProcessCullables() - %s", view.GetName().GetCStr());

            AZ_Assert(parentJob != nullptr || taskGraph != nullptr, "ProcessCullables must have either a valid parent job or a valid task graph");

            const Matrix4x4& worldToClip = view.GetWorldToClipMatrix();
            AZ::Frustum frustum = Frustum::CreateFromMatrixColumnMajor(worldToClip);

            void* maskedOcclusionCulling = nullptr;
            ProcessCullablesCommon(scene, view, frustum, maskedOcclusionCulling);

            AZStd::shared_ptr<WorkListType> worklist = AZStd::make_shared<WorkListType>();
            worklist->Init();
            AZStd::shared_ptr<WorklistData> worklistData = MakeWorklistData(m_debugCtx, scene, view, frustum, maskedOcclusionCulling, parentJob, taskGraphEvent);
            static const AZ::TaskDescriptor descriptor{ "AZ::RPI::ProcessWorklist", "Graphics" };

            auto nodeVisitorLambda = [worklistData, taskGraph, parentJob, &worklist](const AzFramework::IVisibilityScene::NodeData& nodeData) -> void
            {
                auto entriesInNode = nodeData.m_entries.size();
                AZ_Assert(entriesInNode > 0, "should not get called with 0 entries");

                // Check job spawn condition for entries
                bool spawnJob = r_useEntryCountForNodeJobs && (worklist->m_entryCount > 0) &&
                    ((worklist->m_entryCount + entriesInNode) > r_numEntriesPerCullingJob);

                // Check job spawn condition for nodes
                spawnJob = spawnJob || (worklist->m_nodes.size() == worklist->m_nodes.capacity());

                if (spawnJob)
                {
                    // capture worklistData & worklist by value
                    auto processWorklist = [worklistData, worklist]()
                    {
                        ProcessWorklist(worklistData, *worklist);
                    };

                    if (taskGraph != nullptr)
                    {
                        taskGraph->AddTask(descriptor, [worklistData, worklist]()
                            {
                                ProcessWorklist(worklistData, *worklist);
                            });
                    }
                    else
                    {
                        //Kick off a job to process the (full) worklist
                        AZ::Job* job = AZ::CreateJobFunction(processWorklist, true);
                        parentJob->SetContinuation(job);
                        job->Start();
                    }
                    worklist = AZStd::make_shared<WorkListType>();
                    worklist->Init();
                }

                worklist->m_nodes.emplace_back(AZStd::move(nodeData));
                worklist->m_entryCount += u32(entriesInNode);
            };

            if (m_debugCtx.m_enableFrustumCulling)
            {
                m_visScene->Enumerate(frustum, nodeVisitorLambda);
            }
            else
            {
                m_visScene->EnumerateNoCull(nodeVisitorLambda);
            }

            if (worklist->m_nodes.size() > 0)
            {
                // capture worklistData & worklist by value
                auto processWorklist = [worklistData, worklist]()
                {
                    ProcessWorklist(worklistData, *worklist);
                };

                if (taskGraph != nullptr)
                {
                    taskGraph->AddTask(descriptor, AZStd::move(processWorklist));
                }
                else
                {
                    //Kick off a job to process the (full) worklist
                    AZ::Job* job = AZ::CreateJobFunction(AZStd::move(processWorklist), true);
                    parentJob->SetContinuation(job);
                    job->Start();
                }
            }
        }

        // Fastest of the three functions: ProcessCullablesJobsEntries, ProcessCullablesJobsNodes, ProcessCullablesTG
        void CullingScene::ProcessCullablesJobsEntries(const Scene& scene, View& view, AZ::Job* parentJob)
        {
            AZ_PROFILE_SCOPE(RPI, "CullingScene::ProcessCullablesJobsEntries() - %s", view.GetName().GetCStr());

            const Matrix4x4& worldToClip = view.GetWorldToClipMatrix();
            AZ::Frustum frustum = Frustum::CreateFromMatrixColumnMajor(worldToClip);

            void* maskedOcclusionCulling = nullptr;
            ProcessCullablesCommon(scene, view, frustum, maskedOcclusionCulling);

            // Note 1: Cannot do unique_ptr here because compilation error (auto-deletes function from lambda which the job code complains about) 
            // Note 2: Having this be a pointer (even a shared pointer) is faster than just having this live on the stack like:
            // EntryListType entryList;
            // Why isn't immediately clear (did profile several times and noticed the difference of ~0.2-0.3ms, seems making it a stack variable
            // increases the runtime for this function, which runs on a single thread and spawns other jobs).
            AZStd::shared_ptr<EntryListType> entryList = AZStd::make_shared<EntryListType>();
            entryList->m_entries.reserve(r_numEntriesPerCullingJob);
            AZStd::shared_ptr<WorklistData> worklistData = MakeWorklistData(m_debugCtx, scene, view, frustum, maskedOcclusionCulling, parentJob, nullptr);

            auto nodeVisitorLambda = [worklistData, parentJob, &entryList](const AzFramework::IVisibilityScene::NodeData& nodeData) -> void
            {
                AZ_Assert(nodeData.m_entries.size() > 0, "should not get called with 0 entries");
                AZ_Assert(entryList->m_entries.size() < entryList->m_entries.capacity(), "we should always have room to push a node on the queue");

                u32 remainingCount = u32(nodeData.m_entries.size());
                u32 current = 0;
                while (remainingCount > 0)
                {
                    u32 availableCount = u32(entryList->m_entries.capacity() - entryList->m_entries.size());
                    u32 addCount = AZStd::min(availableCount, remainingCount);

                    for (u32 i = 0; i < addCount; ++i)
                    {
                        entryList->m_entries.push_back(nodeData.m_entries[current++]);
                    }
                    remainingCount -= addCount;

                    if (entryList->m_entries.size() == entryList->m_entries.capacity())
                    {
                        auto processWorklist = [worklistData, entryList = AZStd::move(entryList)]()
                        {
                            ProcessEntrylist(worklistData, entryList->m_entries);
                        };

                        AZ::Job* job = AZ::CreateJobFunction(processWorklist, true);
                        entryList = AZStd::make_shared<EntryListType>();
                        entryList->m_entries.reserve(r_numEntriesPerCullingJob);

                        parentJob->SetContinuation(job);
                        job->Start();
                    }
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

            if (entryList->m_entries.size() > 0)
            {
                auto processWorklist = [worklistData, entryList = AZStd::move(entryList)]()
                {
                    ProcessEntrylist(worklistData, entryList->m_entries);
                };

                AZ::Job* job = AZ::CreateJobFunction(processWorklist, true);
                parentJob->SetContinuation(job);
                job->Start();
            }
        }

        void CullingScene::ProcessCullablesJobs(const Scene& scene, View& view, AZ::Job& parentJob)
        {
            if (r_useEntryWorkListsForCulling)
            {
                ProcessCullablesJobsEntries(scene, view, &parentJob);
            }
            else
            {
                ProcessCullables(scene, view, &parentJob, nullptr);
            }
        }

        void CullingScene::ProcessCullablesTG(const Scene& scene, View& view, AZ::TaskGraph& taskGraph, AZ::TaskGraphEvent& taskGraphEvent)
        {
            ProcessCullables(scene, view, nullptr, &taskGraph, &taskGraphEvent);
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
            m_visScene = parentScene->GetVisibilityScene();

            m_taskGraphActive = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();

#ifdef AZ_CULL_DEBUG_ENABLED
            AZ_Assert(CountObjectsInScene() == 0, "The culling system should start with 0 entries in this scene.");
#endif
        }

        void CullingScene::Deactivate()
        {
#ifdef AZ_CULL_DEBUG_ENABLED
            AZ_Assert(CountObjectsInScene() == 0, "All culling entries must be removed from the scene before shutdown.");
#endif
            m_visScene = nullptr;
        }

        void CullingScene::BeginCullingTaskGraph(const AZStd::vector<ViewPtr>& views)
        {
            AZ::TaskGraph taskGraph{ "RPI::Culling" };
            AZ::TaskDescriptor beginCullingDescriptor{"RPI_CullingScene_BeginCullingView", "Graphics"};
            for (auto& view : views)
            {
                taskGraph.AddTask(
                    beginCullingDescriptor,
                    [&view]()
                    {
                        AZ_PROFILE_SCOPE(RPI, "CullingScene: BeginCullingTaskGraph");
                        view->BeginCulling();
                    });
            }

            if (!taskGraph.IsEmpty())
            {
                AZ::TaskGraphEvent waitForCompletion{ "RPI::Culling Wait" };
                taskGraph.Submit(&waitForCompletion);
                waitForCompletion.Wait();
            }
        }

        void CullingScene::BeginCullingJobs(const AZStd::vector<ViewPtr>& views)
        {
            AZ::JobCompletion beginCullingCompletion;

            for (auto& view : views)
            {
                const auto cullingLambda = [&view]()
                {
                    AZ_PROFILE_SCOPE(RPI, "CullingScene: BeginCullingJob");
                    view->BeginCulling();
                };

                AZ::Job* cullingJob = AZ::CreateJobFunction(AZStd::move(cullingLambda), true, nullptr);
                cullingJob->SetDependent(&beginCullingCompletion);
                cullingJob->Start();
            }

            beginCullingCompletion.StartAndWaitForCompletion();
        }

        void CullingScene::BeginCulling(const AZStd::vector<ViewPtr>& views)
        {
            AZ_PROFILE_SCOPE(RPI, "CullingScene: BeginCulling");
            m_cullDataConcurrencyCheck.soft_lock();

            m_debugCtx.ResetCullStats();
            m_debugCtx.m_numCullablesInScene = GetNumCullables();

            m_taskGraphActive = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();

            if(views.size() == 1) // avoid job overhead when only 1 job
            {
                views[0]->BeginCulling();
            }
            else if (m_taskGraphActive && m_taskGraphActive->IsTaskGraphActive())
            {
                BeginCullingTaskGraph(views);
            }
            else
            {
                BeginCullingJobs(views);
            }

#ifdef AZ_CULL_DEBUG_ENABLED
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
#endif
        }

        void CullingScene::EndCulling()
        {
            m_cullDataConcurrencyCheck.soft_unlock();
        }

        size_t CullingScene::CountObjectsInScene()
        {
            size_t numObjects = 0;
            m_visScene->EnumerateNoCull(
                [&numObjects](const AzFramework::IVisibilityScene::NodeData& nodeData)
                {
                    for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                    {
                        if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                        {
                            ++numObjects;
                        }
                    }
                }
            );

            return numObjects;
        }

    } // namespace RPI
} // namespace AZ
