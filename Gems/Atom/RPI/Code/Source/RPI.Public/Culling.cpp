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
#include <Atom/RPI.Public/RenderPipeline.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/View.h>

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Jobs/Job.h>
#include <AzCore/Jobs/JobFunction.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/Task/TaskGraph.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Visibility/OcclusionBus.h>

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
        // Entry work lists
        AZ_CVAR(bool, r_useEntryWorkListsForCulling, false, nullptr, AZ::ConsoleFunctorFlags::Null, "Use entity work lists instead of node work lists for job distribution");
        AZ_CVAR(uint32_t, r_numEntriesPerCullingJob, 750, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls amount of entries to collect for jobs when using entry work lists");

        // Node work lists using entry count
        AZ_CVAR(bool, r_useEntryCountForNodeJobs, true, nullptr, AZ::ConsoleFunctorFlags::Null, "Use entity count instead of node count when checking whether to spawn job for node work list");
        AZ_CVAR(uint32_t, r_maxNodesWhenUsingEntryCount, 100, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls max amount of nodes to collect when using entry count");

        // Node work lists using node count
        AZ_CVAR(uint32_t, r_numNodesPerCullingJob, 25, nullptr, AZ::ConsoleFunctorFlags::Null, "Controls amount of nodes to collect for jobs when not using the entry count");

        // This value dictates the amount to extrude the octree node OBB when doing a frustum intersection test against the camera frustum to help cut draw calls for shadow cascade passes.
        // Default is set to -1 as this is optimization needs to be triggered by the content developer by setting a reasonable non-negative value applicable for their content. 
        AZ_CVAR(int, r_shadowCascadeExtrusionAmount, -1, nullptr, AZ::ConsoleFunctorFlags::Null, "The amount of meters to extrude the Obb towards light direction when doing frustum overlap test against camera frustum");


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

        CullingDebugContext& CullingScene::GetDebugContext()
        {
            return m_debugCtx;
        }

        const AzFramework::IVisibilityScene* CullingScene::GetVisibilityScene() const
        {
            return m_visScene;
        }

        // Search for and return the entity context ID associated with the scene and connected to OcclusionRequestBus. If there is no
        // matching scene, return a null ID.
        static AzFramework::EntityContextId GetEntityContextIdForOcclusion(const AZ::RPI::Scene* scene)
        {
            // Active RPI scenes are registered with the AzFramework::SceneSystem using unique names.
            auto sceneSystem = AzFramework::SceneSystemInterface::Get();
            AZ_Assert(sceneSystem, "Attempting to retrieve the entity context ID for a scene before the scene system interface is ready.");

            AzFramework::EntityContextId resultId = AzFramework::EntityContextId::CreateNull();

            // Enumerate all scenes registered with the AzFramework::SceneSystem
            sceneSystem->IterateActiveScenes(
                [&](const AZStd::shared_ptr<AzFramework::Scene>& azScene)
                {
                    // AzFramework::Scene uses "subsystems" bind arbitrary data. This is generally used to maintain an association between
                    // AzFramework::Scene and AZ::RPI::Scene. We search for the AzFramework::Scene scene with a subsystem matching the input
                    // scene pointer.
                    AZ::RPI::ScenePtr* rpiScene = azScene->FindSubsystemInScene<AZ::RPI::ScenePtr>();
                    if (rpiScene && (*rpiScene).get() == scene)
                    {
                        // Each scene should only be bound to one entity context for entities that will appear in the scene.
                        AzFramework::EntityContext** entityContext =
                            azScene->FindSubsystemInScene<AzFramework::EntityContext::SceneStorageType>();
                        if (entityContext)
                        {
                            // Return if the entity context is valid and connected to OcclusionRequestBus
                            const AzFramework::EntityContextId contextId = (*entityContext)->GetContextId();
                            if (AzFramework::OcclusionRequestBus::HasHandlers(contextId))
                            {
                                resultId = contextId;
                                return false; // Result found, returning
                            }
                        }
                    }

                    return true; // No match, continuing to search for containing scene.
                });
            return resultId;
        }

        struct WorklistData
        {
            CullingDebugContext* m_debugCtx = nullptr;
            const Scene* m_scene = nullptr;
            AzFramework::EntityContextId m_sceneEntityContextId;
            View* m_view = nullptr;
            Frustum m_frustum;
            Frustum m_cameraFrustum;
            Frustum m_excludeFrustum;
            AZ::Job* m_parentJob = nullptr;
            AZ::TaskGraphEvent* m_taskGraphEvent = nullptr;
            bool m_hasExcludeFrustum = false;
            bool m_applyCameraFrustumIntersectionTest = false;
#ifdef AZ_CULL_DEBUG_ENABLED

            AuxGeomDrawPtr GetAuxGeomPtr()
            {
                if (m_debugCtx->m_debugDraw && (m_view->GetName() == m_debugCtx->m_currentViewSelectionName))
                {
                    return AuxGeomFeatureProcessorInterface::GetDrawQueueForScene(m_scene);
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
            AZ::Job* parentJob,
            AZ::TaskGraphEvent* taskGraphEvent)
        {
            AZStd::shared_ptr<WorklistData> worklistData = AZStd::make_shared<WorklistData>();
            worklistData->m_debugCtx = &debugCtx;
            worklistData->m_scene = &scene;
            worklistData->m_sceneEntityContextId = GetEntityContextIdForOcclusion(&scene);
            worklistData->m_view = &view;
            worklistData->m_frustum = frustum;
            worklistData->m_parentJob = parentJob;
            worklistData->m_taskGraphEvent = taskGraphEvent;
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

        static bool TestOcclusionCulling(
            const AZStd::shared_ptr<WorklistData>& worklistData, const AzFramework::VisibilityEntry* visibleEntry);

        static void ProcessEntrylist(
            const AZStd::shared_ptr<WorklistData>& worklistData,
            const AZStd::vector<AzFramework::VisibilityEntry*>& entries,
            bool parentNodeContainedInFrustum = false,
            s32 startIdx = 0,
            s32 endIdx = -1)
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

                if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable ||
                    visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_VisibleObjectList)
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

                    if (worklistData->m_hasExcludeFrustum &&
                        ShapeIntersection::Classify(worklistData->m_excludeFrustum, c->m_cullData.m_boundingSphere) == IntersectResult::Interior)
                    {
                        // Skip item contained in exclude frustum.
                        continue;
                    }

                    if (TestOcclusionCulling(worklistData, visibleEntry))
                    {
                        // There are ways to write this without [[maybe_unused]], but they are brittle.
                        // For example, using #else could cause a bug where the function's parameter
                        // is changed in #ifdef but not in #else.
                        [[maybe_unused]] const uint32_t drawPacketCount = AddLodDataToView(
                            c->m_cullData.m_boundingSphere.GetCenter(), c->m_lodData, *worklistData->m_view, visibleEntry->m_typeFlags);
                        c->m_isVisible = true;
                        worklistData->m_view->ApplyFlags(c->m_flags);

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
                        if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable ||
                            visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_VisibleObjectList)
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

        static bool TestOcclusionCulling(
            const AZStd::shared_ptr<WorklistData>& worklistData, const AzFramework::VisibilityEntry* visibleEntry)
        {
#ifdef AZ_CULL_PROFILE_VERBOSE
            AZ_PROFILE_SCOPE(RPI, "TestOcclusionCulling");
#endif

            if (visibleEntry->m_boundingVolume.Contains(worklistData->m_view->GetCameraTransform().GetTranslation()))
            {
                // camera is inside bounding volume
                return true;
            }

            // Perform occlusion tests using OcclusionRequestBus if it is connected to the entity context ID for this scene.
            if (!worklistData->m_sceneEntityContextId.IsNull())
            {
                AzFramework::OcclusionState state = AzFramework::OcclusionState::Unknown;
                const auto& viewName = worklistData->m_view->GetName();

                AzFramework::OcclusionRequestBus::Event(
                    worklistData->m_sceneEntityContextId,
                    [&](AzFramework::OcclusionRequestBus::Events* handler)
                    {
                        // An occlusion culling system might precompute visibility data for static objects or entities in a scene. If the
                        // system that implements OcclusionRequestBus supports that behavior then we want to perform an initial visibility
                        // test using the entity ID. This can avoid potentially more expensive dynamic tests, like those against an
                        // occlusion buffer.
                        if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                        {
                            auto cullable = static_cast<RPI::Cullable*>(visibleEntry->m_userData);
                            if (cullable && cullable->m_cullData.m_entityId.IsValid())
                            {
                                state = handler->GetOcclusionViewEntityVisibility(viewName, cullable->m_cullData.m_entityId);
                            }
                        }

                        // Entries that don't meet the above criteria or return an inconclusive or partially visible state will perform a
                        // dynamic, bounding box visibility test. One entity can have multiple visibility entries that may need to be tested
                        // individually. If the entire entity is hidden, no further testing is required.
                        if (state != AzFramework::OcclusionState::Hidden)
                        {
                            state = handler->GetOcclusionViewAabbVisibility(viewName, visibleEntry->m_boundingVolume);
                        }
                    });

                // Return immediately to bypass MaskedOcclusionCulling
                return state != AzFramework::OcclusionState::Hidden;
            }

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            MaskedOcclusionCulling* maskedOcclusionCulling = worklistData->m_view->GetMaskedOcclusionCulling();
            if (!maskedOcclusionCulling || !worklistData->m_view->GetMaskedOcclusionCullingDirty())
            {
                return true;
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
                    return true;
                }

                // convert to NDC
                corners[index] /= corners[index].GetW();

                ndcMinX = AZStd::min(ndcMinX, corners[index].GetX());
                ndcMinY = AZStd::min(ndcMinY, corners[index].GetY());
                ndcMaxX = AZStd::max(ndcMaxX, corners[index].GetX());
                ndcMaxY = AZStd::max(ndcMaxY, corners[index].GetY());
            }

            // test against the occlusion buffer, which contains only the manually placed occlusion planes
            if (maskedOcclusionCulling->TestRect(ndcMinX, ndcMinY, ndcMaxX, ndcMaxY, minDepth) !=
                MaskedOcclusionCulling::CullingResult::VISIBLE)
            {
                return false;
            }
#endif
            return true;
        }

        void CullingScene::ProcessCullablesCommon(
            const Scene& scene,
            View& view,
            AZ::Frustum& frustum [[maybe_unused]])
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

            // If connected, update the occlusion views for this scene and view combination.
            if (const auto& entityContextId = GetEntityContextIdForOcclusion(&scene); !entityContextId.IsNull())
            {
                AzFramework::OcclusionRequestBus::Event(
                    entityContextId,
                    &AzFramework::OcclusionRequestBus::Events::UpdateOcclusionView,
                    view.GetName(),
                    view.GetCameraTransform().GetTranslation(),
                    view.GetWorldToClipMatrix());

                // Return immediately to bypass MaskedOcclusionCulling
                return;
            }

#if AZ_TRAIT_MASKED_OCCLUSION_CULLING_SUPPORTED
            // setup occlusion culling, if necessary
            MaskedOcclusionCulling* maskedOcclusionCulling = view.GetMaskedOcclusionCulling();
            if (maskedOcclusionCulling && !m_occlusionPlanes.empty())
            {
                // frustum cull occlusion planes
                using VisibleOcclusionPlane = AZStd::pair<OcclusionPlane, float>;
                AZStd::vector<VisibleOcclusionPlane> visibleOccluders;
                visibleOccluders.reserve(m_occlusionPlanes.size());
                for (const auto& occlusionPlane : m_occlusionPlanes)
                {
                    if (ShapeIntersection::Overlaps(frustum, occlusionPlane.m_aabb))
                    {
                        // occluder is visible, compute view space distance and add to list
                        float depth = (view.GetWorldToViewMatrix() * occlusionPlane.m_aabb.GetMin()).GetZ();
                        depth = AZStd::min(depth, (view.GetWorldToViewMatrix() * occlusionPlane.m_aabb.GetMax()).GetZ());

                        visibleOccluders.emplace_back(occlusionPlane, depth);
                    }
                }

                // sort the occlusion planes by view space distance, front-to-back
                AZStd::sort(visibleOccluders.begin(), visibleOccluders.end(), [](const VisibleOcclusionPlane& LHS, const VisibleOcclusionPlane& RHS)
                {
                    return LHS.second > RHS.second;
                });

                bool anyVisible = false;
                for (const VisibleOcclusionPlane& occlusionPlane : visibleOccluders)
                {
                    // convert to clip-space
                    const Vector4 projectedBL = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerBL);
                    const Vector4 projectedTL = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerTL);
                    const Vector4 projectedTR = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerTR);
                    const Vector4 projectedBR = view.GetWorldToClipMatrix() * Vector4(occlusionPlane.first.m_cornerBR);

                    // store to float array
                    float verts[16];
                    projectedBL.StoreToFloat4(&verts[0]);
                    projectedTL.StoreToFloat4(&verts[4]);
                    projectedTR.StoreToFloat4(&verts[8]);
                    projectedBR.StoreToFloat4(&verts[12]);

                    static constexpr const uint32_t indices[6] = { 0, 1, 2, 2, 3, 0 };

                    // render into the occlusion buffer, specifying BACKFACE_NONE so it functions as a double-sided occluder
                    if (static_cast<MaskedOcclusionCulling*>(maskedOcclusionCulling)
                            ->RenderTriangles(verts, indices, 2, nullptr, MaskedOcclusionCulling::BACKFACE_NONE) ==
                        MaskedOcclusionCulling::CullingResult::VISIBLE)
                    {
                        anyVisible = true;
                    }
                }

                if (anyVisible)
                {
                    view.SetMaskedOcclusionCullingDirty(true);
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

            ProcessCullablesCommon(scene, view, frustum);

            AZStd::shared_ptr<WorkListType> worklist = AZStd::make_shared<WorkListType>();
            worklist->Init();
            AZStd::shared_ptr<WorklistData> worklistData = MakeWorklistData(m_debugCtx, scene, view, frustum, parentJob, taskGraphEvent);
            static const AZ::TaskDescriptor descriptor{ "AZ::RPI::ProcessWorklist", "Graphics" };

            if (const Matrix4x4* worldToClipExclude = view.GetWorldToClipExcludeMatrix())
            {
                worklistData->m_hasExcludeFrustum = true;
                worklistData->m_excludeFrustum = Frustum::CreateFromMatrixColumnMajor(*worldToClipExclude);

                // Get the render pipeline associated with the shadow pass of the given view
                RenderPipelinePtr renderPipeline = scene.GetRenderPipeline(view.GetShadowPassRenderPipelineId());             
                //Only apply this optimization if you only have one view available.
                if (renderPipeline && renderPipeline->GetViews(renderPipeline->GetMainViewTag()).size() == 1)
                {
                    RPI::ViewPtr cameraView = renderPipeline->GetDefaultView();
                    const Matrix4x4& cameraWorldToClip = cameraView->GetWorldToClipMatrix();
                    worklistData->m_cameraFrustum = Frustum::CreateFromMatrixColumnMajor(cameraWorldToClip);
                    worklistData->m_applyCameraFrustumIntersectionTest = true;
                }
            }
            
            auto nodeVisitorLambda = [worklistData, taskGraph, parentJob, &worklist](const AzFramework::IVisibilityScene::NodeData& nodeData) -> void
            {
                // For shadow cascades that are greater than index 0 we can do another check to see if we can reject any Octree node that do not
                // intersect with the camera frustum. We do this by checking for an overlap between the camera frustum and the Obb created
                // from the node's AABB but rotated and extended towards light direction. This optimization is only activated when someone sets
                // a non-negative extrusion value (i.e r_shadowCascadeExtrusionAmount) for their given content.
                if (r_shadowCascadeExtrusionAmount >= 0 && worklistData->m_applyCameraFrustumIntersectionTest && worklistData->m_hasExcludeFrustum)
                {
                    // Build an Obb from the Octree node's aabb
                    AZ::Obb extrudedBounds = AZ::Obb::CreateFromAabb(nodeData.m_bounds);

                    // Rotate the Obb in the direction of the light
                    AZ::Quaternion directionalLightRot = worklistData->m_view->GetCameraTransform().GetRotation();
                    extrudedBounds.SetRotation(directionalLightRot);
                    
                    AZ::Vector3 halfLength = 0.5f * nodeData.m_bounds.GetExtents();
                    // After converting AABB to OBB we apply a rotation and this can incorrectly fail intersection test. If you have an OBB cube built from an octree node,
                    // rotating it can cause it to not encapsulate meshes it encapsulated beforehand. The type of shape we want here is essentially a capsule that starts from the
                    // light and wraps the aabb of the octree node cube and extends towards light direction. This capsule's diameter needs to the size of the body diagonal
                    // of the cube. Since using capsule shape will make intersection test expensive we simply expand the Obb to have each side be at least the size of the body diagonal
                    // which is sqrt(3) * side size. Hence we expand the Obb by 73%. Since this is half length, we expand it by 73% / 2, or 36.5%.
                    halfLength *= Vector3(1.365f);
                    
                    // Next we extrude the Obb in the direction of the light in order to ensure we capture meshes that are behind the camera but cast a shadow within it's frustum
                    halfLength.SetY(halfLength.GetY() + r_shadowCascadeExtrusionAmount);
                    extrudedBounds.SetHalfLengths(halfLength);
                    if (!AZ::ShapeIntersection::Overlaps(worklistData->m_cameraFrustum, extrudedBounds))
                    {
                        return;
                    }
                }

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
                if (worklistData->m_hasExcludeFrustum)
                {
                    m_visScene->Enumerate(frustum, worklistData->m_excludeFrustum, nodeVisitorLambda);
                }
                else
                {
                    m_visScene->Enumerate(frustum, nodeVisitorLambda);
                }
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

            ProcessCullablesCommon(scene, view, frustum);

            // Note 1: Cannot do unique_ptr here because compilation error (auto-deletes function from lambda which the job code complains about) 
            // Note 2: Having this be a pointer (even a shared pointer) is faster than just having this live on the stack like:
            // EntryListType entryList;
            // Why isn't immediately clear (did profile several times and noticed the difference of ~0.2-0.3ms, seems making it a stack variable
            // increases the runtime for this function, which runs on a single thread and spawns other jobs).
            AZStd::shared_ptr<EntryListType> entryList = AZStd::make_shared<EntryListType>();
            entryList->m_entries.reserve(r_numEntriesPerCullingJob);
            AZStd::shared_ptr<WorklistData> worklistData = MakeWorklistData(m_debugCtx, scene, view, frustum, parentJob, nullptr);

            if (const Matrix4x4* worldToClipExclude = view.GetWorldToClipExcludeMatrix())
            {
                worklistData->m_hasExcludeFrustum = true;
                worklistData->m_excludeFrustum = Frustum::CreateFromMatrixColumnMajor(*worldToClipExclude);
            }

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

        uint32_t AddLodDataToView(
            const Vector3& pos, const Cullable::LodData& lodData, RPI::View& view, AzFramework::VisibilityEntry::TypeFlags typeFlags)
        {
#ifdef AZ_CULL_PROFILE_DETAILED
            AZ_PROFILE_SCOPE(RPI, "AddLodDataToView");
#endif

            uint32_t numVisibleDrawPackets = 0;

            auto addLodToDrawPacket = [&](const Cullable::LodData::Lod& lod)
            {
#ifdef AZ_CULL_PROFILE_VERBOSE
                AZ_PROFILE_SCOPE(RPI, "add draw packets: %zu", lod.m_drawPackets.size());
#endif
                numVisibleDrawPackets += static_cast<uint32_t>(lod.m_drawPackets.size());   //don't want to pay the cost of aznumeric_cast<> here so using static_cast<> instead
                if (typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_VisibleObjectList)
                {
                    view.AddVisibleObject(lod.m_visibleObjectUserData, pos);
                }
                else if (typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable)
                {
                    for (const RHI::DrawPacket* drawPacket : lod.m_drawPackets)
                    {
                        view.AddDrawPacket(drawPacket, pos);
                    }
                }
                else
                {
                    AZ_Assert(false, "Invalid cullable type flags.")
                }
            };

            switch (lodData.m_lodConfiguration.m_lodType)
            {
                case Cullable::LodType::SpecificLod:
                    if (lodData.m_lodConfiguration.m_lodOverride < lodData.m_lods.size())
                    {
                    addLodToDrawPacket(
                        lodData.m_lods.at(lodData.m_lodConfiguration.m_lodOverride));
                    }
                    break;
                case Cullable::LodType::ScreenCoverage:
                default:
                {
                    const Matrix4x4& viewToClip = view.GetViewToClipMatrix();
                    // the [1][1] element of a perspective projection matrix stores cot(FovY/2) (equal to
                    // 2*nearPlaneDistance/nearPlaneHeight), which is used to determine the (vertical) projected size in screen space
                    const float yScale = viewToClip.GetElement(1, 1);
                    const bool isPerspective = viewToClip.GetElement(3, 3) == 0.f;
                    const Vector3 cameraPos = view.GetViewToWorldMatrix().GetTranslation();

                    const float approxScreenPercentage =
                        ModelLodUtils::ApproxScreenPercentage(pos, lodData.m_lodSelectionRadius, cameraPos, yScale, isPerspective);

                    for (uint32_t lodIndex = 0; lodIndex < static_cast<uint32_t>(lodData.m_lods.size()); ++lodIndex)
                    {
                        const Cullable::LodData::Lod& lod = lodData.m_lods[lodIndex];
                        // Note that this supports overlapping lod ranges (to support cross-fading lods, for example)
                        if (approxScreenPercentage >= lod.m_screenCoverageMin && approxScreenPercentage <= lod.m_screenCoverageMax)
                        {
                            addLodToDrawPacket(lod);
                        }
                    }
                    break;
                }
            }

            return numVisibleDrawPackets;
        }

        void CullingScene::Activate(const Scene* parentScene)
        {
            m_parentScene = parentScene;
            m_visScene = parentScene->GetVisibilityScene();

            m_taskGraphActive = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();

            if (auto* console = AZ::Interface<AZ::IConsole>::Get(); console != nullptr)
            {
                // Start with default value
                int shadowCascadeExtrusionAmount = r_shadowCascadeExtrusionAmount;
                // Get the cvar value from settings registry
                console->GetCvarValue("r_shadowCascadeExtrusionAmount", shadowCascadeExtrusionAmount);
                // push the cvars value so anything in this dll can access it directly.
                console->PerformCommand(
                    AZStd::string::format("r_shadowCascadeExtrusionAmount %i", shadowCascadeExtrusionAmount).c_str());
            }

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

        void CullingScene::BeginCullingTaskGraph(const Scene& scene, AZStd::span<const ViewPtr> views)
        {
            AZ::TaskGraph taskGraph{ "RPI::Culling" };
            AZ::TaskDescriptor beginCullingDescriptor{ "RPI_CullingScene_BeginCullingView", "Graphics" };

            const auto& entityContextId = GetEntityContextIdForOcclusion(&scene);
            for (auto& view : views)
            {
                taskGraph.AddTask(
                    beginCullingDescriptor,
                    [&]()
                    {
                        AZ_PROFILE_SCOPE(RPI, "CullingScene: BeginCullingTaskGraph");
                        view->BeginCulling();
                        AzFramework::OcclusionRequestBus::Event(
                            entityContextId, &AzFramework::OcclusionRequestBus::Events::CreateOcclusionView, view->GetName());
                    });
            }

            if (!taskGraph.IsEmpty())
            {
                AZ::TaskGraphEvent waitForCompletion{ "RPI::Culling Wait" };
                taskGraph.Submit(&waitForCompletion);
                waitForCompletion.Wait();
            }
        }

        void CullingScene::BeginCullingJobs(const Scene& scene, AZStd::span<const ViewPtr> views)
        {
            AZ::JobCompletion beginCullingCompletion;

            const auto& entityContextId = GetEntityContextIdForOcclusion(&scene);
            for (auto& view : views)
            {
                const auto cullingLambda = [&]()
                {
                    AZ_PROFILE_SCOPE(RPI, "CullingScene: BeginCullingJob");
                    view->BeginCulling();
                    AzFramework::OcclusionRequestBus::Event(
                        entityContextId, &AzFramework::OcclusionRequestBus::Events::CreateOcclusionView, view->GetName());
                };

                AZ::Job* cullingJob = AZ::CreateJobFunction(AZStd::move(cullingLambda), true, nullptr);
                cullingJob->SetDependent(&beginCullingCompletion);
                cullingJob->Start();
            }

            beginCullingCompletion.StartAndWaitForCompletion();
        }

        void CullingScene::BeginCulling(const Scene& scene, AZStd::span<const ViewPtr> views)
        {
            AZ_PROFILE_SCOPE(RPI, "CullingScene: BeginCulling");
            m_cullDataConcurrencyCheck.soft_lock();

            m_debugCtx.ResetCullStats();
            m_debugCtx.m_numCullablesInScene = GetNumCullables();

            m_taskGraphActive = AZ::Interface<AZ::TaskGraphActiveInterface>::Get();

            // Remove any debug artifacts from the previous occlusion culling session.
            const auto& entityContextId = GetEntityContextIdForOcclusion(&scene);
            AzFramework::OcclusionRequestBus::Event(
                entityContextId, &AzFramework::OcclusionRequestBus::Events::ClearOcclusionViewDebugInfo);

            if (views.size() == 1) // avoid job overhead when only 1 job
            {
                views[0]->BeginCulling();
                AzFramework::OcclusionRequestBus::Event(
                    entityContextId, &AzFramework::OcclusionRequestBus::Events::CreateOcclusionView, views[0]->GetName());
            }
            else if (m_taskGraphActive && m_taskGraphActive->IsTaskGraphActive())
            {
                BeginCullingTaskGraph(scene, views);
            }
            else
            {
                BeginCullingJobs(scene, views);
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

        void CullingScene::EndCulling(const Scene& scene, AZStd::span<const ViewPtr> views)
        {
            m_cullDataConcurrencyCheck.soft_unlock();

            // When culling has completed, destroy all of the occlusion views.
            if (const auto& entityContextId = GetEntityContextIdForOcclusion(&scene); !entityContextId.IsNull())
            {
                for (auto& view : views)
                {
                    AzFramework::OcclusionRequestBus::Event(
                        entityContextId, &AzFramework::OcclusionRequestBus::Events::DestroyOcclusionView, view->GetName());
                }
            }
        }

        size_t CullingScene::CountObjectsInScene()
        {
            size_t numObjects = 0;
            m_visScene->EnumerateNoCull(
                [&numObjects](const AzFramework::IVisibilityScene::NodeData& nodeData)
                {
                    for (AzFramework::VisibilityEntry* visibleEntry : nodeData.m_entries)
                    {
                        if (visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_Cullable ||
                            visibleEntry->m_typeFlags & AzFramework::VisibilityEntry::TYPE_RPI_VisibleObjectList)
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
