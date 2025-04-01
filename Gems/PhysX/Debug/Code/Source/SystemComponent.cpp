/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "SystemComponent.h"

#include <PhysX/Debug/PhysXDebugInterface.h>

#include <PhysX/SystemComponentBus.h>
#include <PhysX/MathConversion.h>
#include <PhysX/UserDataTypes.h>
#include <PhysX/Utils.h>
#include <PhysX/PhysXLocks.h>

#include <CryCommon/IConsole.h>
#include <CryCommon/ISystem.h>
#include <CryCommon/MathConversion.h>

#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Physics/PhysicsScene.h>
#include <AzFramework/Physics/PhysicsSystem.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzFramework/Physics/Utils.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Debug/Profiler.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>


namespace PhysXDebug
{
    const float SystemComponent::m_maxCullingBoxSize = 150.0f;
    namespace Internal
    {
        const AZ::Crc32 VewportId = AzFramework::g_defaultSceneEntityDebugDisplayId;
    }

    bool UseEditorPhysicsScene()
    {
        // Runtime components are created when 'simulation' mode is enabled in the Editor,
        // so we shouldn't use Editor physics scene in this case
        return gEnv->IsEditing() && !gEnv->IsEditorSimulationMode();
    }

    void ReflectPhysXVisulizationSettings(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<PhysXVisualizationSettings>()
                ->Version(1)
                ->Field("VisualizationEnabled", &PhysXVisualizationSettings::m_visualizationEnabled)
                ->Field("CollisionShapes", &PhysXVisualizationSettings::m_collisionShapes)
                ->Field("CollisionFNormals", &PhysXVisualizationSettings::m_collisionFNormals)
                ->Field("CollisionEdges", &PhysXVisualizationSettings::m_collisionEdges)
                ->Field("CollisionAabbs", &PhysXVisualizationSettings::m_collisionAabbs)
                ->Field("CollisionCompounds", &PhysXVisualizationSettings::m_collisionCompounds)
                ->Field("CollisionStatic", &PhysXVisualizationSettings::m_collisionStatic)
                ->Field("CollisionDynamic", &PhysXVisualizationSettings::m_collisionDynamic)
                ->Field("BodyAxis", &PhysXVisualizationSettings::m_bodyAxes)
                ->Field("BodyMassAxis", &PhysXVisualizationSettings::m_bodyMassAxes)
                ->Field("BodyLinVelocity", &PhysXVisualizationSettings::m_bodyLinVelocity)
                ->Field("BodyAngVelocity", &PhysXVisualizationSettings::m_bodyAngVelocity)
                ->Field("ContactPoint", &PhysXVisualizationSettings::m_contactPoint)
                ->Field("ContactNormal", &PhysXVisualizationSettings::m_contactNormal)
                ->Field("JointLocalFrames", &PhysXVisualizationSettings::m_jointLocalFrames)
                ->Field("JointLimits", &PhysXVisualizationSettings::m_jointLimits)
                ->Field("MbpRegions", &PhysXVisualizationSettings::m_mbpRegions)
                ->Field("ActorAxes", &PhysXVisualizationSettings::m_actorAxes);

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<PhysXVisualizationSettings>("PhysX Debug Draw Settings", "Settings to configure the PhysX Debug Visualization Gem properties.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_visualizationEnabled, "Enable PhysX Debug Visualization", "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_collisionShapes, "Collision Shapes", "Enable collision shapes")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_collisionFNormals, "Collision FNormals", "Enable collision face normals")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_collisionEdges, "Collision Edges", "Enable collision edges")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_collisionAabbs, "Collision Aabbs", "Enable collision aabbs")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_collisionCompounds, "Collision Compounds", "Enable collision compounds")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_collisionStatic, "Collision Static", "Enable collision static")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_collisionDynamic, "Collision Dynamic", "Enable collision dynamic")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_bodyAxes, "Body Axis", "Enable body axis")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_bodyMassAxes, "Body Mass Axis", "Enable body mass axis")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_bodyLinVelocity, "Body Linear Velocity", "Enable body linear velocity")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_bodyAngVelocity, "Body Angular Velocity", "Enable body angular velocity")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_contactPoint, "Contact Point", "Enable contact point")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_contactNormal, "Contact Normal", "Enable contact normal")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_jointLocalFrames, "Joint Local Frames", "Enable joint local frames")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_jointLimits, "Joint Limits", "Enable Joint limits")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_mbpRegions, "MBP Regions", "Enable multi box pruning (MBP) regions")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &PhysXVisualizationSettings::m_actorAxes, "Actor Axes", "Enable actor axes")
                    ->Attribute(AZ::Edit::Attributes::Visibility, &PhysXVisualizationSettings::IsPhysXDebugEnabled)
                ;
            }
        }
    }

    void ReflectPhysXCullingSettings(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<Culling>()
                ->Version(1)
                ->Field("cullingBoxSize", &Culling::m_boxSize)
                ->Field("cullBox", &Culling::m_enabled)
                ->Field("cullBoxWireFrame", &Culling::m_boxWireframe)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<Culling>("Culling Settings", "Settings to configure the PhysX Debug Visualization Culling.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Culling::m_enabled, "Enable Box Culling", "Enable box culling")
                    ->DataElement(AZ::Edit::UIHandlers::CheckBox, &Culling::m_boxWireframe, "Show Culling Box", "Visualize the culling box")
                    ->DataElement(AZ::Edit::UIHandlers::Slider, &Culling::m_boxSize, "Culling Box Size", "Size of the culling box")
                    ->Attribute(AZ::Edit::Attributes::Min, 1.0f)
                    ->Attribute(AZ::Edit::Attributes::Max, 150.0f)
                ;
            }
        }
    }

    SystemComponent::SystemComponent()
        : m_sceneFinishSimHandler([this](
            [[maybe_unused]] AzPhysics::SceneHandle sceneHanle,
            [[maybe_unused]] float fixedDeltatime)
            {
                this->m_editorPhysicsSceneDirty = true;
            })
    {

    }

    void SystemComponent::ReflectPhysXDebugSettings(AZ::ReflectContext* context)
    {
        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<SystemComponent, AZ::Component>()
                ->Version(1)
                ->Field("physxDebugSettings", &SystemComponent::m_settings)
                ->Field("physxDebugCulling", &SystemComponent::m_culling)
            ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<SystemComponent>("PhysX Debug Visualization", "A debug visualization system component for PhysX.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::Category, "PhysX")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_settings, "Settings", "PhysX debug visualization settings")
                    ->DataElement(AZ::Edit::UIHandlers::Default, &SystemComponent::m_culling, "Culling", "PhysX culling options")
                ;
            }
        }
    }

    void SystemComponent::OnCrySystemInitialized([[maybe_unused]] ISystem& system, const SSystemInitParams&)
    {
        InitPhysXColorMappings();
        ConfigurePhysXVisualizationParameters();
    }

    void SystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectPhysXVisulizationSettings(context);
        ReflectPhysXCullingSettings(context);
        ReflectPhysXDebugSettings(context);
    }

    void SystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PhysXDebugService"));
    }

    void SystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PhysXDebugService"));
    }

    void SystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("PhysicsService"));
#ifdef PHYSXDEBUG_GEM_EDITOR
        required.push_back(AZ_CRC_CE("PhysicsEditorService"));
#endif // PHYSXDEBUG_GEM_EDITOR
    }

    void SystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void SystemComponent::Activate()
    {
        PhysXDebugRequestBus::Handler::BusConnect();
        AZ::TickBus::Handler::BusConnect();
        CrySystemEventBus::Handler::BusConnect();
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif // IMGUI_ENABLED
#ifdef PHYSXDEBUG_GEM_EDITOR
        if (auto* sceneInterface = AZ::Interface<AzPhysics::SceneInterface>::Get())
        {
            AzPhysics::SceneHandle sceneHandle = sceneInterface->GetSceneHandle(AzPhysics::EditorPhysicsSceneName);
            sceneInterface->RegisterSceneSimulationFinishHandler(sceneHandle, m_sceneFinishSimHandler);
        }
#endif // PHYSXDEBUG_GEM_EDITOR
    }

    void SystemComponent::Deactivate()
    {
#ifdef PHYSXDEBUG_GEM_EDITOR
        m_sceneFinishSimHandler.Disconnect();
#endif // PHYSXDEBUG_GEM_EDITOR
#ifdef IMGUI_ENABLED
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif // IMGUI_ENABLED
        CrySystemEventBus::Handler::BusDisconnect();
        AZ::TickBus::Handler::BusDisconnect();
        PhysXDebugRequestBus::Handler::BusDisconnect();
    }

#ifdef IMGUI_ENABLED
    void SystemComponent::OnImGuiMainMenuUpdate()
    {
        if (ImGui::BeginMenu("PhysX Debug"))
        {
            ImGui::Checkbox("Debug visualization", &m_settings.m_visualizationEnabled);
            ImGui::Checkbox("Visualize Colliders", &m_settings.m_visualizeCollidersByProximity);

            if (ImGui::BeginMenu("Culling"))
            {
                ImGui::Checkbox("Wireframe", &m_culling.m_boxWireframe);
                ImGui::SliderFloat("Size", &m_culling.m_boxSize, 0, m_maxCullingBoxSize);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Collisions"))
            {
                ImGui::Checkbox("Shapes", &m_settings.m_collisionShapes);
                ImGui::Checkbox("Edges", &m_settings.m_collisionEdges);
                ImGui::Checkbox("F Normals", &m_settings.m_collisionFNormals);
                ImGui::Checkbox("Aabbs", &m_settings.m_collisionAabbs);
                ImGui::Checkbox("Axis", &m_settings.m_collisionAxes);
                ImGui::Checkbox("Compounds", &m_settings.m_collisionCompounds);
                ImGui::Checkbox("Static", &m_settings.m_collisionStatic);
                ImGui::Checkbox("Dynamic", &m_settings.m_collisionDynamic);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Body"))
            {
                ImGui::Checkbox("Axes", &m_settings.m_bodyAxes);
                ImGui::Checkbox("Mass Axes", &m_settings.m_bodyMassAxes);
                ImGui::Checkbox("Linear Velocity", &m_settings.m_bodyLinVelocity);
                ImGui::Checkbox("Angular Velocity", &m_settings.m_bodyAngVelocity);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Contact"))
            {
                ImGui::Checkbox("Point", &m_settings.m_contactPoint);
                ImGui::Checkbox("Normal", &m_settings.m_contactNormal);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Character"))
            {
                ImGui::Checkbox("Joint Limits", &m_settings.m_jointLimits);
                ImGui::Checkbox("Mbp Regions", &m_settings.m_mbpRegions);
                ImGui::Checkbox("Actor Axes", &m_settings.m_actorAxes);
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("PhysX Color Mappings"))
            {
                BuildColorPickingMenuItem("Black", m_colorMappings.m_black);
                BuildColorPickingMenuItem("Red", m_colorMappings.m_red);
                BuildColorPickingMenuItem("Green", m_colorMappings.m_green);
                BuildColorPickingMenuItem("Blue", m_colorMappings.m_blue);
                BuildColorPickingMenuItem("Yellow", m_colorMappings.m_yellow);
                BuildColorPickingMenuItem("Magenta", m_colorMappings.m_magenta);
                BuildColorPickingMenuItem("Cyan", m_colorMappings.m_cyan);
                BuildColorPickingMenuItem("White", m_colorMappings.m_white);
                BuildColorPickingMenuItem("Grey", m_colorMappings.m_grey);
                BuildColorPickingMenuItem("Dark Red", m_colorMappings.m_darkRed);
                BuildColorPickingMenuItem("Dark Green", m_colorMappings.m_darkGreen);
                BuildColorPickingMenuItem("Dark Blue", m_colorMappings.m_darkBlue);

                if (ImGui::Button("Reset Color Mappings"))
                {
                    InitPhysXColorMappings();
                }

                ImGui::EndMenu();
            }

            if (ImGui::Button("Enable/Disable all settings"))
            {
                ToggleVisualizationConfiguration();
            }

            ImGui::SliderFloat("PhysX Scale", &m_settings.m_scale, 1.0f, 10.0f);
            ImGui::EndMenu();
        }
    }

    void SystemComponent::BuildColorPickingMenuItem(const AZStd::string& label, AZ::Color& color)
    {
        float col[3] = {color.GetR(), color.GetG(), color.GetB()};
        if (ImGui::ColorEdit3(label.c_str(), col, ImGuiColorEditFlags_NoAlpha))
        {
            const float r = AZ::GetClamp(col[0], 0.0f, 1.0f);
            const float g = AZ::GetClamp(col[1], 0.0f, 1.0f);
            const float b = AZ::GetClamp(col[2], 0.0f, 1.0f);

            color.SetR(r);
            color.SetG(g);
            color.SetB(b);
        }
    }
#endif // IMGUI_ENABLED

    static const physx::PxRenderBuffer& GetRenderBuffer(physx::PxScene* physxScene)
    {
        AZ_PROFILE_FUNCTION(Physics);
        PHYSX_SCENE_READ_LOCK(physxScene);
        return physxScene->getRenderBuffer();
    }

    void SystemComponent::ToggleCullingWireFrame()
    {
        m_culling.m_boxWireframe = !m_culling.m_boxWireframe;
    }

    void SystemComponent::ToggleVisualizationConfiguration()
    {
        const bool updatedValue = !m_settings.m_collisionFNormals;

        m_settings.m_visualizeCollidersByProximity = updatedValue;
        m_settings.m_collisionFNormals = updatedValue;
        m_settings.m_collisionAabbs = updatedValue;
        m_settings.m_collisionAxes = updatedValue;
        m_settings.m_collisionCompounds = updatedValue;
        m_settings.m_collisionStatic = updatedValue;
        m_settings.m_collisionDynamic = updatedValue;
        m_settings.m_bodyAxes = updatedValue;
        m_settings.m_bodyMassAxes = updatedValue;
        m_settings.m_bodyLinVelocity = updatedValue;
        m_settings.m_bodyAngVelocity = updatedValue;
        m_settings.m_contactPoint = updatedValue;
        m_settings.m_contactNormal = updatedValue;
        m_settings.m_jointLocalFrames = updatedValue;
        m_settings.m_jointLimits = updatedValue;
        m_settings.m_mbpRegions = updatedValue;
        m_settings.m_actorAxes = updatedValue;

        ConfigurePhysXVisualizationParameters();
    }

    void SystemComponent::SetVisualization(bool enabled)
    {
        m_settings.m_visualizationEnabled = enabled;

        ConfigurePhysXVisualizationParameters();
    }

    void SystemComponent::ToggleColliderProximityDebugVisualization()
    {
        m_settings.m_visualizeCollidersByProximity = !m_settings.m_visualizeCollidersByProximity;
    }

    void SystemComponent::SetCullingBoxSize(float cullingBoxSize)
    {
        if (cullingBoxSize <= m_maxCullingBoxSize)
        {
            m_culling.m_enabled = true;
            m_culling.m_boxSize = cullingBoxSize;

            ConfigurePhysXVisualizationParameters();
            ConfigureCullingBox();
        }
    }

    physx::PxScene* SystemComponent::GetCurrentPxScene()
    {
        AzPhysics::SceneHandle sceneHandle;

        if (UseEditorPhysicsScene())
        {
            // Editor scene needs to be ticked for debug rendering to work (taking place in EditorSystemComponent)
            Physics::EditorWorldBus::BroadcastResult(sceneHandle, &Physics::EditorWorldRequests::GetEditorSceneHandle);
        }
        else
        {
            Physics::DefaultWorldBus::BroadcastResult(sceneHandle, &Physics::DefaultWorldRequests::GetDefaultSceneHandle);
        }

        if (auto* physicsSystem = AZ::Interface<AzPhysics::SystemInterface>::Get())
        {
            if (auto* scene = physicsSystem->GetScene(sceneHandle))
            {
                return static_cast<physx::PxScene*>(scene->GetNativePointer());
            }
        }

        return nullptr;
    }

    // TickBus::Handler
    void SystemComponent::OnTick([[maybe_unused]] float deltaTime, AZ::ScriptTimePoint time)
    {
        if (!m_settings.IsPhysXDebugEnabled())
        {
            return;
        }

        AZ_PROFILE_FUNCTION(Physics);
        m_currentTime = time;
        bool dirty = true;

        if (UseEditorPhysicsScene())
        {
            dirty = m_editorPhysicsSceneDirty;
        }

        UpdateColliderVisualizationByProximity();

        if (dirty)
        {
            // The physics scene is dirty and contains changes to be gathered.
            if (GetCurrentPxScene())
            {
                ConfigurePhysXVisualizationParameters();
                ConfigureCullingBox();

                ClearBuffers();
                GatherBuffers();

                m_editorPhysicsSceneDirty = false;
            }
        }

        RenderBuffers();
    }

    AZ::Vector3 GetViewCameraPosition()
    {
        using Camera::ActiveCameraRequestBus;

        AZ::Transform tm = AZ::Transform::CreateIdentity();
        ActiveCameraRequestBus::BroadcastResult(tm, &ActiveCameraRequestBus::Events::GetActiveCameraTransform);
        return tm.GetTranslation();
    }

    void SystemComponent::UpdateColliderVisualizationByProximity()
    {
        if (auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
            UseEditorPhysicsScene() && m_settings.m_visualizeCollidersByProximity
           && debug != nullptr)
        {
            const AZ::Vector3& viewPos = GetViewCameraPosition();
            const PhysX::Debug::ColliderProximityVisualization data(
                m_settings.m_visualizeCollidersByProximity,
                viewPos,
                m_culling.m_boxSize * 0.5f);
            debug->UpdateColliderProximityVisualization(data);
        }
    }

    void SystemComponent::ClearBuffers()
    {
        m_linePoints.clear();
        m_lineColors.clear();
        m_trianglePoints.clear();
        m_triangleColors.clear();
    }

    void SystemComponent::GatherBuffers()
    {
        physx::PxScene* physxScene = GetCurrentPxScene();
        const physx::PxRenderBuffer& rb = GetRenderBuffer(physxScene);
        GatherLines(rb);
        GatherTriangles(rb);
        GatherJointLimits();
    }

    void SystemComponent::RenderBuffers()
    {
        if (!m_linePoints.empty() || !m_trianglePoints.empty())
        {
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, Internal::VewportId);
            AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");
            AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus);
            if (debugDisplay)
            {
                if (!m_linePoints.empty())
                {
                    AZ_Assert(m_linePoints.size() == m_lineColors.size(), "Lines: Expected an equal number of points to colors.");
                    const size_t minLen = AZ::GetMin(m_linePoints.size(), m_lineColors.size());
                    for (size_t i = 0; i < minLen; i += 2)
                    {
                        debugDisplay->DrawLine(m_linePoints[i], m_linePoints[i + 1], m_lineColors[i].GetAsVector4(), m_lineColors[i + 1].GetAsVector4());
                    }
                }
                if (!m_trianglePoints.empty())
                {
                    AZ_Assert(m_trianglePoints.size() == m_triangleColors.size(), "Triangles: Expected an equal number of points to colors.");
                    const size_t minLen = AZ::GetMin(m_trianglePoints.size(), m_triangleColors.size());
                    for (size_t i = 0; i < minLen; i += 3)
                    {
                        debugDisplay->SetColor(m_triangleColors[i]);
                        debugDisplay->DrawTri(m_trianglePoints[i], m_trianglePoints[i + 1], m_trianglePoints[i + 2]);
                    }
                }
            }
        }
    }

    static void physx_CullingBox([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        PhysXDebug::PhysXDebugRequestBus::Broadcast(&PhysXDebug::PhysXDebugRequestBus::Events::ToggleCullingWireFrame);
    }
    AZ_CONSOLEFREEFUNC(physx_CullingBox, AZ::ConsoleFunctorFlags::DontReplicate, "Enables physx wireframe view");

    static void physx_PvdConnect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (debug)
        {
            debug->ConnectToPvd();
        }
    }
    AZ_CONSOLEFREEFUNC(physx_PvdConnect, AZ::ConsoleFunctorFlags::DontReplicate, "Connects to the physx visual debugger");

    static void physx_PvdDisconnect([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        auto* debug = AZ::Interface<PhysX::Debug::PhysXDebugInterface>::Get();
        if (debug)
        {
            debug->DisconnectFromPvd();
        }
    }
    AZ_CONSOLEFREEFUNC(physx_PvdDisconnect, AZ::ConsoleFunctorFlags::DontReplicate, "Disconnects from the physx visual debugger");

    static void physx_CullingBoxSize([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        const size_t argumentCount = arguments.size();
        if (argumentCount == 1)
        {
            float newCullingBoxSize = (float)strtol(AZ::CVarFixedString(arguments[0]).c_str(), nullptr, 10);
            PhysXDebug::PhysXDebugRequestBus::Broadcast(&PhysXDebug::PhysXDebugRequestBus::Events::SetCullingBoxSize, newCullingBoxSize);
        }
        else
        {
            AZ_Warning("PhysXDebug", false, "Invalid physx_SetDebugCullingBoxSize Arguments. "
                "Please use physx_SetDebugCullingBoxSize <boxSize> e.g. physx_SetDebugCullingBoxSize 100.");
        }
    }
    AZ_CONSOLEFREEFUNC(physx_CullingBoxSize, AZ::ConsoleFunctorFlags::DontReplicate, "Sets physx debug culling box size");

    static void physx_Debug([[maybe_unused]] const AZ::ConsoleCommandContainer& arguments)
    {
        const size_t argumentCount = arguments.size();

        if (argumentCount == 1)
        {
            const auto userPreference = static_cast<DebugCVarValues>(strtol(AZ::CVarFixedString(arguments[0]).c_str(), nullptr, 10));

            switch (userPreference)
            {
            case DebugCVarValues::Enable:
                PhysXDebug::PhysXDebugRequestBus::Broadcast(&PhysXDebug::PhysXDebugRequestBus::Events::SetVisualization, true);
                break;
            case DebugCVarValues::Disable:
                PhysXDebug::PhysXDebugRequestBus::Broadcast(&PhysXDebug::PhysXDebugRequestBus::Events::SetVisualization, false);
                break;
            case DebugCVarValues::SwitchConfigurationPreference:
                PhysXDebug::PhysXDebugRequestBus::Broadcast(&PhysXDebug::PhysXDebugRequestBus::Events::ToggleVisualizationConfiguration);
                break;
            case DebugCVarValues::ColliderProximityDebug:
                PhysXDebug::PhysXDebugRequestBus::Broadcast(&PhysXDebug::PhysXDebugRequestBus::Events::ToggleColliderProximityDebugVisualization);
                break;
            default:
                AZ_Warning("PhysXDebug", false, "Unknown user preference used.");
                break;
            }
        }
        else
        {
            AZ_Warning("PhysXDebug", false, "Invalid physx_Debug Arguments. Please use physx_Debug 1 to enable, physx_Debug 0 to disable or physx_Debug 2 to enable all configuration settings.");
        }
    }
    AZ_CONSOLEFREEFUNC(physx_Debug, AZ::ConsoleFunctorFlags::DontReplicate, "Toggles physx debug visualization");

    void SystemComponent::ConfigurePhysXVisualizationParameters()
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (physx::PxScene* physxScene = GetCurrentPxScene())
        {
            PHYSX_SCENE_WRITE_LOCK(physxScene);

            // Warning: if "mScale" is enabled, then debug visualization data will be available and requested from PhysX
            // this however creates a "significant performance impact" !
            // we do however provide culling (as default), however this will only cull eCOLLISION_SHAPES, eCOLLISION_FNORMALS and eCOLLISION_EDGES.
            // if you enabled more settings, we will culling them but the data will still be made available in physX but simply will not be rendered in the viewport.
            // https://docs.nvidia.com/gameworks/content/gameworkslibrary/physx/guide/Manual/DebugVisualization.html

            if (!m_settings.m_visualizationEnabled)
            {
                physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, 0.0f);
            }
            else
            {
                physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eSCALE, m_settings.m_scale);
            }

            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_SHAPES, m_settings.m_collisionShapes ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_FNORMALS, m_settings.m_collisionFNormals ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_EDGES, m_settings.m_collisionEdges ? 1.0f : 0.0f);

            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_AABBS, m_settings.m_collisionAabbs ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eBODY_AXES, m_settings.m_bodyAxes ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eBODY_MASS_AXES, m_settings.m_bodyMassAxes ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eBODY_LIN_VELOCITY, m_settings.m_bodyLinVelocity ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eBODY_ANG_VELOCITY, m_settings.m_bodyAngVelocity ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCONTACT_POINT, m_settings.m_contactPoint ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCONTACT_NORMAL, m_settings.m_contactNormal ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_AXES, m_settings.m_collisionAxes ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_COMPOUNDS, m_settings.m_collisionCompounds ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_STATIC, m_settings.m_collisionStatic ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCOLLISION_DYNAMIC, m_settings.m_collisionDynamic ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eJOINT_LOCAL_FRAMES, m_settings.m_jointLocalFrames ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eJOINT_LIMITS, m_settings.m_jointLimits ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eMBP_REGIONS, m_settings.m_mbpRegions ? 1.0f : 0.0f);
            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eACTOR_AXES, m_settings.m_actorAxes ? 1.0f : 0.0f);

            physxScene->setVisualizationParameter(physx::PxVisualizationParameter::eCULL_BOX, m_culling.m_enabled ? 1.0f : 0.0f);
        }
    }

    void SystemComponent::ConfigureCullingBox()
    {
        AZ_PROFILE_FUNCTION(Physics);

        // Currently using the Cry view camera to support Editor, Game and Launcher modes. This will be updated in due course.
        const AZ::Vector3 cameraTranslation = GetViewCameraPosition();

        if (!cameraTranslation.IsClose(AZ::Vector3::CreateZero()))
        {
            const physx::PxVec3 min = PxMathConvert(cameraTranslation - AZ::Vector3(m_culling.m_boxSize));
            const physx::PxVec3 max = PxMathConvert(cameraTranslation + AZ::Vector3(m_culling.m_boxSize));
            m_cullingBox = physx::PxBounds3(min, max);

            if (m_culling.m_boxWireframe)
            {
                const AZ::Aabb cullingBoxAabb = AZ::Aabb::CreateFromMinMax(PxMathConvert(min), PxMathConvert(max));
                DrawDebugCullingBox(cullingBoxAabb);
            }

            if (physx::PxScene* physxScene = GetCurrentPxScene())
            {
                PHYSX_SCENE_WRITE_LOCK(physxScene);
                physxScene->setVisualizationCullingBox(m_cullingBox);
            }
        }
    }

    void SystemComponent::GatherTriangles(const physx::PxRenderBuffer& rb)
    {
        AZ_PROFILE_FUNCTION(Physics);
        if (!m_settings.m_visualizationEnabled)
        {
            return;
        }

        if (GetCurrentPxScene())
        {
            // Reserve vector capacity
            const physx::PxU32 numTriangles = static_cast<physx::PxU32>(rb.getNbTriangles());
            m_trianglePoints.reserve(numTriangles * 3);
            m_triangleColors.reserve(numTriangles * 3);

            for (physx::PxU32 triangleIndex = 0; triangleIndex < numTriangles; ++triangleIndex)
            {
                const physx::PxDebugTriangle& triangle = rb.getTriangles()[triangleIndex];

                if (!m_culling.m_enabled ||
                    (m_cullingBox.contains(triangle.pos0) && m_cullingBox.contains(triangle.pos1) && m_cullingBox.contains(triangle.pos2)))
                {
                    m_trianglePoints.emplace_back(PxMathConvert(triangle.pos0));
                    m_trianglePoints.emplace_back(PxMathConvert(triangle.pos1));
                    m_trianglePoints.emplace_back(PxMathConvert(triangle.pos2));

                    m_triangleColors.emplace_back(MapOriginalPhysXColorToUserDefinedValues(triangle.color0));
                    m_triangleColors.emplace_back(MapOriginalPhysXColorToUserDefinedValues(triangle.color1));
                    m_triangleColors.emplace_back(MapOriginalPhysXColorToUserDefinedValues(triangle.color2));
                }
            }
        }
    }

    void SystemComponent::GatherLines(const physx::PxRenderBuffer& rb)
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (!m_settings.m_visualizationEnabled)
        {
            return;
        }

        if (GetCurrentPxScene())
        {
            const physx::PxU32 numLines = static_cast<physx::PxU32>(rb.getNbLines());

            // Reserve vector capacity
            m_linePoints.reserve(numLines * 2);
            m_lineColors.reserve(numLines * 2);

            for (physx::PxU32 lineIndex = 0; lineIndex < numLines; ++lineIndex)
            {
                const physx::PxDebugLine& line = rb.getLines()[lineIndex];

                // Bespoke culling of lines on top of the provided PhysX 3.4 box culling.
                // if culling is enabled we will perform additional culling as required.
                if (!m_culling.m_enabled || (m_cullingBox.contains(line.pos0) && m_cullingBox.contains(line.pos1)))
                {
                    m_linePoints.emplace_back(PxMathConvert(line.pos0));
                    m_linePoints.emplace_back(PxMathConvert(line.pos1));

                    m_lineColors.emplace_back(MapOriginalPhysXColorToUserDefinedValues(line.color0));
                    m_lineColors.emplace_back(MapOriginalPhysXColorToUserDefinedValues(line.color0));
                }
            }
        }
    }

    void SystemComponent::GatherJointLimits()
    {
        AZ_PROFILE_FUNCTION(Physics);

        physx::PxScene* scene = GetCurrentPxScene();

        // The PhysX debug render buffer does not seem to include joint limits, even when
        // PxVisualizationParameter::eJOINT_LIMITS is set, so they are separately added to the line buffer here.
        if (m_settings.m_jointLimits && scene)
        {
            const physx::PxU32 numConstraints = scene->getNbConstraints();
            for (physx::PxU32 constraintIndex = 0; constraintIndex < numConstraints; constraintIndex++)
            {
                physx::PxConstraint* constraint;
                scene->getConstraints(&constraint, 1, constraintIndex);
                physx::PxRigidActor* actor0;
                physx::PxRigidActor* actor1;
                constraint->getActors(actor0, actor1);

                PhysX::ActorData* actorData = PhysX::Utils::GetUserData(actor1);
                if (actorData)
                {
                    Physics::RagdollNode* ragdollNode = actorData->GetRagdollNode();
                    if (ragdollNode)
                    {
                        AzPhysics::Joint* joint = ragdollNode->GetJoint();
                        physx::PxJoint* pxJoint = static_cast<physx::PxJoint*>(joint->GetNativePointer());
                        physx::PxTransform jointPose = actor1->getGlobalPose() * pxJoint->getLocalPose(physx::PxJointActorIndex::eACTOR1);
                        if (!m_culling.m_enabled || m_cullingBox.contains(jointPose.p))
                        {
                            m_jointVertexBuffer.clear();
                            m_jointIndexBuffer.clear();
                            m_jointLineBuffer.clear();
                            m_jointLineValidityBuffer.clear();

                            joint->GenerateJointLimitVisualizationData(0.1f, 32, 2, m_jointVertexBuffer,
                                m_jointIndexBuffer, m_jointLineBuffer, m_jointLineValidityBuffer);

                            physx::PxTransform jointWorldTransformPx = actor0->getGlobalPose();
                            jointWorldTransformPx.p = jointPose.p;
                            AZ::Transform jointWorldTransform = PxMathConvert(jointWorldTransformPx);

                            const size_t jointLineBufferSize = m_jointLineBuffer.size();

                            m_linePoints.reserve((m_linePoints.size() + jointLineBufferSize));
                            m_lineColors.reserve((m_lineColors.size() + jointLineBufferSize));

                            for (size_t lineIndex = 0; lineIndex < jointLineBufferSize / 2; lineIndex++)
                            {
                                m_linePoints.emplace_back(jointWorldTransform.TransformPoint(m_jointLineBuffer[2 * lineIndex]));
                                m_linePoints.emplace_back(jointWorldTransform.TransformPoint(m_jointLineBuffer[2 * lineIndex + 1]));
                                m_lineColors.emplace_back(m_colorMappings.m_green);
                                m_lineColors.emplace_back(m_colorMappings.m_green);
                            }
                        }
                    }
                }
            }
        }
    }

    void SystemComponent::DrawDebugCullingBox(const AZ::Aabb& cullingBoxAabb)
    {
        AZ_PROFILE_FUNCTION(Physics);

        if (m_settings.m_visualizationEnabled && m_culling.m_boxWireframe)
        {
            AzFramework::DebugDisplayRequestBus::BusPtr debugDisplayBus;
            AzFramework::DebugDisplayRequestBus::Bind(debugDisplayBus, Internal::VewportId);
            AZ_Assert(debugDisplayBus, "Invalid DebugDisplayRequestBus.");
            if (AzFramework::DebugDisplayRequests* debugDisplay = AzFramework::DebugDisplayRequestBus::FindFirstHandler(debugDisplayBus))
            {
                const AZ::Color wireframeColor = MapOriginalPhysXColorToUserDefinedValues(1);
                debugDisplay->SetColor(wireframeColor);
                debugDisplay->DrawWireBox(cullingBoxAabb.GetMin(), cullingBoxAabb.GetMax());
            }
        }
    }

    AZ::Color SystemComponent::MapOriginalPhysXColorToUserDefinedValues(const physx::PxU32& originalColor)
    {
        AZ_PROFILE_FUNCTION(Physics);

        // color mapping from PhysX to LY user preference: \PhysX_3.4\Include\common\PxRenderBuffer.h
        switch (static_cast<physx::PxDebugColor::Enum>(originalColor))
        {
        case physx::PxDebugColor::eARGB_BLACK:
            return m_colorMappings.m_black;
        case physx::PxDebugColor::eARGB_RED:
            return m_colorMappings.m_red;
        case physx::PxDebugColor::eARGB_GREEN:
            return m_colorMappings.m_green;
        case physx::PxDebugColor::eARGB_BLUE:
            return m_colorMappings.m_blue;
        case physx::PxDebugColor::eARGB_YELLOW:
            return m_colorMappings.m_yellow;
        case physx::PxDebugColor::eARGB_MAGENTA:
            return m_colorMappings.m_magenta;
        case physx::PxDebugColor::eARGB_CYAN:
            return m_colorMappings.m_cyan;
        case physx::PxDebugColor::eARGB_WHITE:
            return m_colorMappings.m_white;
        case physx::PxDebugColor::eARGB_GREY:
            return m_colorMappings.m_grey;
        case physx::PxDebugColor::eARGB_DARKRED:
            return m_colorMappings.m_darkRed;
        case physx::PxDebugColor::eARGB_DARKGREEN:
            return m_colorMappings.m_darkGreen;
        case physx::PxDebugColor::eARGB_DARKBLUE:
            return m_colorMappings.m_darkBlue;
        default:
            return m_colorMappings.m_defaultColor;
        }
    }

    void SystemComponent::InitPhysXColorMappings()
    {
        AZ_PROFILE_FUNCTION(Physics);
        m_colorMappings.m_defaultColor.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_GREEN));
        m_colorMappings.m_black.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_BLACK));
        m_colorMappings.m_red.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_RED));
        m_colorMappings.m_green.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_GREEN));
        m_colorMappings.m_blue.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_BLUE));
        m_colorMappings.m_yellow.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_YELLOW));
        m_colorMappings.m_magenta.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_MAGENTA));
        m_colorMappings.m_cyan.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_CYAN));
        m_colorMappings.m_white.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_WHITE));
        m_colorMappings.m_grey.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_GREY));
        m_colorMappings.m_darkRed.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_DARKRED));
        m_colorMappings.m_darkGreen.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_DARKGREEN));
        m_colorMappings.m_darkBlue.FromU32(static_cast<AZ::u32>(physx::PxDebugColor::eARGB_DARKBLUE));
    }
}
