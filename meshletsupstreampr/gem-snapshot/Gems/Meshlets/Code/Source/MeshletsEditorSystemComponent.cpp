
/*
* Copyright (c) Contributors to the Open 3D Engine Project.
* For complete copyright and license terms please see the LICENSE at the root of this distribution.
*
* SPDX-License-Identifier: Apache-2.0 OR MIT
*
*/

#include <AzCore/Serialization/SerializeContext.h>
#include <MeshletsEditorSystemComponent.h>

#ifdef IMGUI_ENABLED
#include <imgui/imgui.h>
#include <MeshletsFeatureProcessor.h>
#include <Atom/RPI.Public/Scene.h>
#include <Atom/RPI.Public/ViewportContextBus.h>
#include <Atom/RPI.Public/ViewportContext.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Console/IConsole.h>
#endif

namespace AZ
{
    namespace Meshlets
    {
        void MeshletsEditorSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<MeshletsEditorSystemComponent, MeshletsSystemComponent>()
                    ->Version(0);
            }
        }

        MeshletsEditorSystemComponent::MeshletsEditorSystemComponent() = default;

        MeshletsEditorSystemComponent::~MeshletsEditorSystemComponent() = default;

        void MeshletsEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            BaseSystemComponent::GetProvidedServices(provided);
            provided.push_back(AZ_CRC_CE("MeshletsEditorService"));
        }

        void MeshletsEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            BaseSystemComponent::GetIncompatibleServices(incompatible);
            incompatible.push_back(AZ_CRC_CE("MeshletsEditorService"));
        }

        void MeshletsEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            BaseSystemComponent::GetRequiredServices(required);
        }

        void MeshletsEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            BaseSystemComponent::GetDependentServices(dependent);
        }

        void MeshletsEditorSystemComponent::Activate()
        {
            MeshletsSystemComponent::Activate();
            AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
#ifdef IMGUI_ENABLED
            ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif
        }

        void MeshletsEditorSystemComponent::Deactivate()
        {
#ifdef IMGUI_ENABLED
            ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif
            AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
            MeshletsSystemComponent::Deactivate();
        }

#ifdef IMGUI_ENABLED
        void MeshletsEditorSystemComponent::OnImGuiMainMenuUpdate()
        {
            if (ImGui::BeginMenu("Meshlets"))
            {
                ImGui::MenuItem("Debug", nullptr, &m_showDebugWindow);
                ImGui::EndMenu();
            }
        }

        void MeshletsEditorSystemComponent::OnImGuiUpdate()
        {
            if (!m_showDebugWindow)
            {
                return;
            }

            ImGui::SetNextWindowSize(ImVec2(440, 560), ImGuiCond_FirstUseEver);
            if (ImGui::Begin("Meshlets Debug", &m_showDebugWindow))
            {
                // Find the meshlet feature processor on the active viewport's scene.
                MeshletsFeatureProcessor* fp = nullptr;
                if (auto* vcReq = AZ::Interface<AZ::RPI::ViewportContextRequestsInterface>::Get())
                {
                    if (AZ::RPI::ViewportContextPtr vc = vcReq->GetDefaultViewportContext())
                    {
                        if (AZ::RPI::ScenePtr scene = vc->GetRenderScene())
                        {
                            fp = scene->GetFeatureProcessor<MeshletsFeatureProcessor>();
                        }
                    }
                }

                if (!fp)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                        "No Meshlets feature processor in the active scene.");
                }
                else
                {
                    const MeshletsFeatureProcessor::DebugStats stats = fp->GetDebugStats();
                    MeshletsFeatureProcessor::DebugControls& ctrl = fp->GetDebugControls();

                    ImGui::Text("Render objects: %u      Instances: %u",
                        stats.m_renderObjectCount, stats.m_instanceCount);
                    ImGui::Text("Totals  clusters: %llu   triangles: %llu   vertices: %llu",
                        (unsigned long long)stats.m_totalClusters,
                        (unsigned long long)stats.m_totalTriangles,
                        (unsigned long long)stats.m_totalVertices);
                    ImGui::BulletText("PSOs  depth:%s shadow:%s forward:%s motion:%s indirect:%s",
                        stats.m_depthActive ? "y" : "-", stats.m_shadowActive ? "y" : "-",
                        stats.m_forwardActive ? "y" : "-", stats.m_motionActive ? "y" : "-",
                        stats.m_indirectActive ? "y" : "-");

                    ImGui::Separator();
                    ImGui::TextUnformatted("Render pass toggles");
                    bool changed = false;
                    changed |= ImGui::Checkbox("Depth prepass", &ctrl.m_depthPassEnabled);
                    changed |= ImGui::Checkbox("Shadow pass", &ctrl.m_shadowPassEnabled);
                    changed |= ImGui::Checkbox("Forward (PBR)", &ctrl.m_forwardPassEnabled);
                    changed |= ImGui::Checkbox("Motion vectors", &ctrl.m_motionPassEnabled);
                    changed |= ImGui::Checkbox("Force debug (UV) shader", &ctrl.m_useDebugShader);
                    if (changed)
                    {
                        // Pass-enable / shader changes affect which DrawItems are built.
                        fp->InvalidateAllDrawPackets();
                    }

                    ImGui::Separator();
                    ImGui::TextUnformatted("Debug coloring");
                    // Flat per-cluster (meshlet) color on the forward pass. No packet
                    // rebuild needed — the feature processor applies it via an SRG
                    // constant. Works with or without culling.
                    ImGui::Checkbox("Meshlet colors", &ctrl.m_meshletColorMode);
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                        "(forward pass; turn off 'Force debug shader')");

                    ImGui::Separator();
                    ImGui::TextUnformatted("Level of Detail (LOD)");
                    if (stats.m_maxLodCount <= 1)
                    {
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                            "No LOD hierarchy baked (maxLodCount=1).");
                        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                            "Re-bake packs (BuilderVersion>=8) to get LODs.");
                    }
                    // Force-LOD A/B: -1 = auto (screen coverage), 0..K-1 = pin that LOD on
                    // every instance. Distant geometry should drop LODs automatically; force
                    // a coarse LOD to confirm the vertex win shows up in the GPU pass times.
                    {
                        int forceLod = ctrl.m_forceLodIndex;
                        // m_maxLodCount is always >= 1 (GetDebugStats seeds it from GetLodCount).
                        const int maxLod = static_cast<int>(stats.m_maxLodCount) - 1;
                        if (ImGui::SliderInt("Force LOD (-1=auto)", &forceLod, -1, maxLod))
                        {
                            ctrl.m_forceLodIndex = forceLod;
                        }
                        ImGui::SameLine();
                        ImGui::TextColored(
                            ctrl.m_forceLodIndex >= 0 ? ImVec4(1.0f, 0.6f, 0.2f, 1.0f)
                                                      : ImVec4(0.4f, 1.0f, 0.4f, 1.0f),
                            ctrl.m_forceLodIndex >= 0 ? "[FORCED]" : "[auto]");
                    }
                    // Per-LOD instance histogram (where screen-coverage put each instance).
                    ImGui::TextUnformatted("Instances per LOD:");
                    for (uint32_t l = 0; l < stats.m_maxLodCount && l < 8; ++l)
                    {
                        ImGui::SameLine();
                        ImGui::Text("L%u=%u", l, stats.m_lodHistogram[l]);
                    }
                    // Rendered vs full vertices = the LOD vertex reduction (the actual win).
                    if (stats.m_fullVertices > 0)
                    {
                        const float pct = 100.0f *
                            float(stats.m_fullVertices - stats.m_renderedVertices) /
                            float(stats.m_fullVertices);
                        ImGui::Text("Verts drawn %llu / %llu full  (LOD saves %.1f%%)",
                            (unsigned long long)stats.m_renderedVertices,
                            (unsigned long long)stats.m_fullVertices, pct);
                    }

                    ImGui::Separator();
                    ImGui::TextUnformatted("Cluster culling");
                    bool cullModeChanged = false;
                    cullModeChanged |= ImGui::Checkbox("Enable", &ctrl.m_cullEnabled);
                    ImGui::SameLine();
                    ImGui::Checkbox("Frustum", &ctrl.m_frustumCull);
                    ImGui::SameLine();
                    ImGui::Checkbox("Cone", &ctrl.m_coneCull);
                    // CPU vs GPU cull. GPU culls in a compute pass and draws from a
                    // compute-written indirect-args buffer; CPU culls each frame on the
                    // CPU. Switching changes the geometry view, so rebuild the packets.
                    cullModeChanged |= ImGui::Checkbox("GPU cull (compute)", &ctrl.m_gpuCull);
                    ImGui::SameLine();
                    ImGui::TextColored(
                        ctrl.m_gpuCull ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
                        ctrl.m_gpuCull ? "[GPU]" : "[CPU]");
                    if (cullModeChanged)
                    {
                        fp->InvalidateAllDrawPackets();
                    }
                    // Per-cluster HiZ occlusion (GPU cull only). Currently a no-op:
                    // MeshletsCullSrg::m_hiZTexture isn't wired to a pyramid yet, and the
                    // shader's own dimension check treats an unbound texture as "never
                    // occluded" regardless of this flag. Left here so the toggle exists
                    // once a pipeline connects the shared HiZ pyramid.
                    ImGui::Checkbox("HiZ occlusion (prev-frame pyramid; GPU + AS cull)", &ctrl.m_hiZCull);
                    ImGui::Checkbox("Freeze cull camera", &ctrl.m_freezeCullCamera);
                    if (ctrl.m_freezeCullCamera)
                    {
                        ImGui::SameLine();
                        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f),
                            "[FROZEN - fly around to inspect culled clusters]");
                    }
                    if (stats.m_totalClusters > 0)
                    {
                        const uint64_t vis = ctrl.m_visibleClusters;
                        const uint64_t tot = stats.m_totalClusters;
                        const float pct = tot > 0 ? (100.0f * float(tot - vis) / float(tot)) : 0.0f;
                        ImGui::Text("Visible %llu / %llu  (culled %llu, %.1f%%)",
                            (unsigned long long)vis,
                            (unsigned long long)tot,
                            (unsigned long long)ctrl.m_culledClusters,
                            pct);
                    }

                    // ---- Phase 6 cluster-DAG LOD (cvar-backed; needs a DAG-baked pack:
                    // sidecar "generate_cluster_dag" + reprocess) ----
                    ImGui::Separator();
                    ImGui::TextUnformatted("Cluster DAG LOD (Phase 6)");
                    if (auto* console = AZ::Interface<AZ::IConsole>::Get())
                    {
                        bool dagLod = false;
                        console->GetCvarValue("r_meshletsDagLod", dagLod);
                        if (ImGui::Checkbox("DAG LOD (r_meshletsDagLod)", &dagLod))
                        {
                            console->PerformCommand(
                                AZStd::string::format("r_meshletsDagLod %d", dagLod ? 1 : 0).c_str());
                        }
                        float dagErrPx = 1.0f;
                        console->GetCvarValue("r_meshletsDagErrorPx", dagErrPx);
                        if (ImGui::SliderFloat("Cut error (px)", &dagErrPx, 0.25f, 8.0f, "%.2f"))
                        {
                            console->PerformCommand(
                                AZStd::string::format("r_meshletsDagErrorPx %f", dagErrPx).c_str());
                        }
                        bool dagColor = false;
                        console->GetCvarValue("r_meshletsDagDebugColor", dagColor);
                        if (ImGui::Checkbox("Color by DAG depth", &dagColor))
                        {
                            console->PerformCommand(
                                AZStd::string::format("r_meshletsDagDebugColor %d", dagColor ? 1 : 0).c_str());
                        }
                        ImGui::TextDisabled("Sweep the error slider: detail must recede smoothly, no cracks.");

                        // Phase 7 streaming (v1 residency tracking; needs a v4
                        // "generate_pages" pack + r_meshletsStreaming).
                        bool streaming = false;
                        console->GetCvarValue("r_meshletsStreaming", streaming);
                        if (ImGui::Checkbox("Streaming residency (r_meshletsStreaming)", &streaming))
                        {
                            console->PerformCommand(
                                AZStd::string::format("r_meshletsStreaming %d", streaming ? 1 : 0).c_str());
                        }
                        if (streaming)
                        {
                            ImGui::Text("Pages: %u resident / %u tracked (slots %u) | +%u / -%u this frame",
                                fp->GetStreamingResidentPages(), fp->GetStreamingTrackedPages(),
                                fp->GetStreamingSlotCapacity(), fp->GetStreamingLoadsThisFrame(),
                                fp->GetStreamingEvictsThisFrame());
                            // Phase 4 soak: churn = pages re-loaded within ~1s of their
                            // own eviction. Should stay FLAT during normal camera motion;
                            // if it climbs, raise r_meshletsStreamingHysteresis or the pool.
                            if (fp->GetStreamingStarvedPages() > 0)
                            {
                                // Persistent nonzero = pool smaller than the wanted set:
                                // geometry is (by design) coarser than the cut asks for.
                                ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.2f, 1.0f),
                                    "STARVED: %u wanted pages unloadable (pool too small)",
                                    fp->GetStreamingStarvedPages());
                            }
                            ImGui::Text("Churn (evict->reload): %llu",
                                static_cast<unsigned long long>(fp->GetStreamingChurnCount()));
                            float hysteresis = 1.5f;
                            console->GetCvarValue("r_meshletsStreamingHysteresis", hysteresis);
                            if (ImGui::SliderFloat("Evict hysteresis", &hysteresis, 1.0f, 4.0f, "%.2f"))
                            {
                                console->PerformCommand(
                                    AZStd::string::format("r_meshletsStreamingHysteresis %f", hysteresis).c_str());
                            }
                        }
                    }

                    ImGui::Separator();
                    ImGui::TextUnformatted("Per-object");
                    for (const MeshletsFeatureProcessor::DebugObjectInfo& o : stats.m_objects)
                    {
                        ImGui::BulletText("%s\n   clusters %u  tris %u  verts %u  LODs %u  inst %u  mat:%s",
                            o.m_name.c_str(), o.m_clusters, o.m_triangles, o.m_vertices,
                            o.m_lods, o.m_instances, o.m_materialResolved ? "model" : "default");
                    }
                }
            }
            ImGui::End();
        }
#endif // IMGUI_ENABLED

    } // namespace Meshlets
} // namespace AZ

