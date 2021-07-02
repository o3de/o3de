/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/base.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/parallel/lock.h>
#include <AzCore/std/sort.h>
#include <AzCore/std/string/string.h>

#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/Culling.h>

namespace AZ
{    
    namespace Render
    {
        //"inline" is necessary since this is declared in a .inl file. Remove it if this definition moves into a .cpp at some point
        inline void ImGuiDrawCullingDebug(bool& draw, AZ::RPI::Scene* scene)
        {
            using namespace RPI;

            CullingScene* cullScene = scene->GetCullingScene();
            CullingDebugContext& debugCtx = cullScene->GetDebugContext();

            ImGui::SetNextWindowSize(ImVec2(900.f, 700.f), ImGuiCond_Once);
            if (ImGui::Begin("Culling Debug", &draw, ImGuiWindowFlags_None))
            {
                ImGui::SetWindowPos("Culling Debug", ImVec2(100.f, 100.f), ImGuiCond_Once);

                char sceneIdBuffer[255] = "";
                scene->GetId().ToString(sceneIdBuffer, 255);
                ImGui::Text("SceneId: %s", sceneIdBuffer);

                ImGui::Separator();

                ImGui::Checkbox("Enable Frustum Culling", &debugCtx.m_enableFrustumCulling);
                ImGui::Checkbox("Enable Parallel Octree Traversal",  &debugCtx.m_parallelOctreeTraversal);
                ImGui::Checkbox("Freeze Frustums", &debugCtx.m_freezeFrustums);
                ImGui::Checkbox("Debug Draw", &debugCtx.m_debugDraw);
                {
                    ImGui::Indent();
                    ImGui::Checkbox("Show View Frustum", &debugCtx.m_drawViewFrustum);
                    ImGui::Checkbox("Show Fully Visible Scene Nodes", &debugCtx.m_drawFullyVisibleNodes);
                    ImGui::Checkbox("Show Partially Visible Scene Nodes", &debugCtx.m_drawPartiallyVisibleNodes);
                    ImGui::Checkbox("Show Object Bounding Boxes", &debugCtx.m_drawBoundingBoxes);
                    ImGui::Checkbox("Show Object Bounding Spheres", &debugCtx.m_drawBoundingSpheres);
                    ImGui::Checkbox("Show Object Lod Radius", &debugCtx.m_drawLodRadii);
                    ImGui::Checkbox("Show World Coordinate Axes", &debugCtx.m_drawWorldCoordinateAxes);
                    ImGui::Unindent();
                }

                uint32_t totalCullables = 0;
                uint32_t totalVisibleCullables = 0;
                uint32_t totalVisibleDrawPackets = 0;
                uint32_t totalCullJobs = 0;
                size_t numViews = 0;

                auto& perViewCullStats = debugCtx.LockAndGetAllCullStats();
                numViews = perViewCullStats.size();

                // sort by view name
                using CullStatsType = CullingDebugContext::CullStats;
                AZStd::vector<CullStatsType*> cullStatsSorted;
                for (auto& cullStatsIter : perViewCullStats)
                {
                    cullStatsSorted.push_back(cullStatsIter.second);
                }
                AZStd::sort(cullStatsSorted.begin(), cullStatsSorted.end(),
                    [](const CullStatsType* a, const CullStatsType* b) -> bool
                    {
                        return azstricmp(a->m_name.GetCStr(), b->m_name.GetCStr()) < 0;
                    }
                );
                
                AZStd::vector<AZStd::string> itemStrings;
                for (CullStatsType* cullStats : cullStatsSorted)
                {
                    // create formatted display strings
                    itemStrings.push_back(AZStd::string::format("%s - %d/%d CullPackets visible, %d drawPackets visible, %d cull jobs",
                        cullStats->m_name.GetCStr(),
                        static_cast<uint32_t>(cullStats->m_numVisibleCullables),
                        static_cast<uint32_t>(debugCtx.m_numCullablesInScene),
                        static_cast<uint32_t>(cullStats->m_numVisibleDrawPackets),
                        static_cast<uint32_t>(cullStats->m_numJobs)
                    ));

                    // collect totals
                    totalCullables += debugCtx.m_numCullablesInScene;
                    totalVisibleCullables += cullStats->m_numVisibleCullables;
                    totalVisibleDrawPackets += cullStats->m_numVisibleDrawPackets;
                    totalCullJobs += cullStats->m_numJobs;
                }

                if (ImGui::BeginChild("Totals", ImVec2(0, 120.0f), true, ImGuiWindowFlags_None))
                {
                    ImGui::Text("Totals:");
                    ImGui::Separator();
                    ImGui::Text("   %zu Views", numViews);
                    ImGui::Text("   %u Cull Jobs", totalCullJobs);
                    ImGui::Text("   %d/%d Visible Cullables", totalVisibleCullables, totalCullables);
                    ImGui::Text("   %d Submitted DrawPackets", totalVisibleDrawPackets);
                }                
                ImGui::EndChild();

                auto vectorGetter = [](void* data, int idx, const char** outText) -> bool
                {
                    auto& v = *static_cast<AZStd::vector<AZStd::string>*>(data);
                    if (idx < 0 || idx >= aznumeric_cast<int>(v.size()))
                    {
                        return false;
                    }
                    *outText = v[idx].c_str();
                    return true;
                };
                int numItems = aznumeric_cast<int>(itemStrings.size());
                ImGui::Text("Views");
                ImGui::PushItemWidth(-1); //remove the right-aligned label from the listbox
                ImGui::ListBox("", &debugCtx.m_currentViewSelection, vectorGetter, &itemStrings, numItems, 15);
                ImGui::PopItemWidth();

                if (debugCtx.m_currentViewSelection >= 0 && debugCtx.m_currentViewSelection < aznumeric_cast<int>(cullStatsSorted.size()))
                {
                    CullStatsType* cullStats = cullStatsSorted[debugCtx.m_currentViewSelection];
                    debugCtx.m_currentViewSelectionName = cullStats->m_name;
                    const Matrix4x4& m = cullStats->m_cameraViewToWorld;
                    ImGui::Text("Selected View's ViewToWorld: \n[%.2f, %.2f, %.2f, %.2f]\n[%.2f, %.2f, %.2f, %.2f]\n[%.2f, %.2f, %.2f, %.2f]\n[%.2f, %.2f, %.2f, %.2f]",
                        m(0,0), m(0,1), m(0,2), m(0,3),
                        m(1,0), m(1,1), m(1,2), m(1,3),
                        m(2,0), m(2,1), m(2,2), m(2,3),
                        m(3,0), m(3,1), m(3,2), m(3,3));
                }
                else
                {
                    debugCtx.m_currentViewSelectionName = "";
                }

                debugCtx.UnlockAllCullStats();
            }
            ImGui::End();

            debugCtx.m_enableStats = draw;  //turn off stats tracking when the window is closed
        }
    }
}
