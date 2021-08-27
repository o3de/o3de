/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "RenderUpdateCallback.h"
#include "RenderPlugin.h"
#include "../EMStudioCore.h"
#include <EMotionFX/Rendering/Common/OrthographicCamera.h>
#include <EMotionFX/Source/TransformData.h>
#include "RenderWidget.h"
#include "RenderViewWidget.h"


namespace EMStudio
{
    // constructor
    RenderUpdateCallback::RenderUpdateCallback(RenderPlugin* plugin)
    {
        m_enableRendering    = true;
        m_plugin             = plugin;
    }


    // enable or disable rendering
    void RenderUpdateCallback::SetEnableRendering(bool renderingEnabled)
    {
        m_enableRendering = renderingEnabled;
    }


    // check for visibility
    // inside this example we just mark the character as visible always
    void RenderUpdateCallback::OnUpdateVisibilityFlags(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        // set to visible for the cases the active view widget is nullptr
        // this happens when call the Process() function from the render plugin before we update our view
        RenderViewWidget* widget = m_plugin->GetActiveViewWidget();
        if (widget == nullptr)
        {
            actorInstance->SetIsVisible(true);
            return;
        }

        MCommon::Camera* camera = widget->GetRenderWidget()->GetCamera();
        if (camera == nullptr)
        {
            actorInstance->SetIsVisible(false);
            return;
        }

        // check if the actor is actually visible in our view frustum
        bool visible = true;
        /*  const AABB& aabb = actorInstance->GetAABB();
            if (aabb.CheckIfIsValid())
                visible = camera->GetFrustum().PartiallyContains( actorInstance->GetAABB() );*/

        actorInstance->SetIsVisible(visible);
    }


    // update a visible character
    void RenderUpdateCallback::OnUpdate(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        // update the transformation of the actor instance and deform its meshes
        //actorInstance->UpdateTransformations( timePassedInSeconds, true);

        // find the corresponding trajectory trace path for the given actor instance
        MCommon::RenderUtil::TrajectoryTracePath* trajectoryPath = m_plugin->FindTracePath(actorInstance);
        if (trajectoryPath)
        {
            EMotionFX::Actor* actor = actorInstance->GetActor();
            EMotionFX::Node* motionExtractionNode = actor->GetMotionExtractionNode();
            if (motionExtractionNode)
            {
                // get access to the world space matrix of the trajectory
                EMotionFX::TransformData* transformData = actorInstance->GetTransformData();
                const EMotionFX::Transform globalTM = transformData->GetCurrentPose()->GetWorldSpaceTransform(motionExtractionNode->GetNodeIndex()).ProjectedToGroundPlane();

                bool distanceTraveledEnough = false;
                if (trajectoryPath->m_traceParticles.empty())
                {
                    distanceTraveledEnough = true;
                }
                else
                {
                    const size_t numParticles = trajectoryPath->m_traceParticles.size();
                    const EMotionFX::Transform& oldGlobalTM = trajectoryPath->m_traceParticles[numParticles - 1].m_worldTm;

                    const AZ::Vector3& oldPos = oldGlobalTM.m_position;
                    const AZ::Quaternion& oldRot = oldGlobalTM.m_rotation;
                    const AZ::Quaternion rotation = globalTM.m_rotation.GetNormalized();

                    const AZ::Vector3 deltaPos = globalTM.m_position - oldPos;
                    float deltaRot = MCore::Math::Abs(rotation.Dot(oldRot));

                    if (MCore::SafeLength(deltaPos) > 0.0001f || deltaRot < 0.99f)
                    {
                        distanceTraveledEnough = true;
                    }
                }

                // add the time delta to the time passed since the last add
                trajectoryPath->m_timePassed += timePassedInSeconds;

                const uint32 particleSampleRate = 30;
                if (trajectoryPath->m_timePassed >= (1.0f / particleSampleRate) && distanceTraveledEnough)
                {
                    // create the particle, fill its data and add it to the trajectory trace path
                    MCommon::RenderUtil::TrajectoryPathParticle trajectoryParticle;
                    trajectoryParticle.m_worldTm = globalTM;
                    trajectoryPath->m_traceParticles.emplace_back(trajectoryParticle);

                    // reset the time passed as we just added a new particle
                    trajectoryPath->m_timePassed = 0.0f;
                }
            }

            // make sure we don't have too many items in our array
            if (trajectoryPath->m_traceParticles.size() > 50)
            {
                trajectoryPath->m_traceParticles.erase(begin(trajectoryPath->m_traceParticles));
            }
        }
    }


    // update a visible character
    void RenderUpdateCallback::OnRender(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        if (m_enableRendering == false)
        {
            return;
        }

        RenderPlugin::EMStudioRenderActor* emstudioActor = m_plugin->FindEMStudioActor(actorInstance);
        if (emstudioActor == nullptr)
        {
            return;
        }

        // renderUtil options
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        actorInstance->UpdateMeshDeformers(timePassedInSeconds);

        // get the active widget & it's rendering options
        RenderViewWidget*   widget          = m_plugin->GetActiveViewWidget();
        RenderOptions*      renderOptions   = m_plugin->GetRenderOptions();

        const AZStd::unordered_set<size_t>& visibleJointIndices = GetManager()->GetVisibleJointIndices();
        const AZStd::unordered_set<size_t>& selectedJointIndices = GetManager()->GetSelectedJointIndices();

        // render the AABBs
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_AABB))
        {
            MCommon::RenderUtil::AABBRenderSettings settings;
            settings.m_nodeBasedColor          = renderOptions->GetNodeAABBColor();
            settings.m_staticBasedColor        = renderOptions->GetStaticAABBColor();
            settings.m_meshBasedColor          = renderOptions->GetMeshAABBColor();

            renderUtil->RenderAabbs(actorInstance, settings);
        }

        if (widget->GetRenderFlag(RenderViewWidget::RENDER_LINESKELETON))
        {
            renderUtil->RenderSimpleSkeleton(actorInstance, &visibleJointIndices, &selectedJointIndices, renderOptions->GetLineSkeletonColor(), renderOptions->GetSelectedObjectColor());
        }

        bool cullingEnabled = renderUtil->GetCullingEnabled();
        bool lightingEnabled = renderUtil->GetLightingEnabled();
        renderUtil->EnableCulling(false); // disable culling
        renderUtil->EnableLighting(false); // disable lighting
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_SKELETON))
        {
            renderUtil->RenderSkeleton(actorInstance, emstudioActor->m_boneList, &visibleJointIndices, &selectedJointIndices, renderOptions->GetSkeletonColor(), renderOptions->GetSelectedObjectColor());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODEORIENTATION))
        {
            renderUtil->RenderNodeOrientations(actorInstance, emstudioActor->m_boneList, &visibleJointIndices, &selectedJointIndices, renderOptions->GetNodeOrientationScale(), renderOptions->GetScaleBonesOnLength());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_ACTORBINDPOSE))
        {
            renderUtil->RenderBindPose(actorInstance);
        }

        // render motion extraction debug info
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_MOTIONEXTRACTION))
        {
            // render an arrow for the trajectory node
            renderUtil->RenderTrajectoryPath(m_plugin->FindTracePath(actorInstance), renderOptions->GetTrajectoryArrowInnerColor(), emstudioActor->m_characterHeight * 0.05f);
        }
        renderUtil->EnableCulling(cullingEnabled); // reset to the old state
        renderUtil->EnableLighting(lightingEnabled);

        const bool renderVertexNormals  = widget->GetRenderFlag(RenderViewWidget::RENDER_VERTEXNORMALS);
        const bool renderFaceNormals    = widget->GetRenderFlag(RenderViewWidget::RENDER_FACENORMALS);
        const bool renderTangents       = widget->GetRenderFlag(RenderViewWidget::RENDER_TANGENTS);
        const bool renderWireframe      = widget->GetRenderFlag(RenderViewWidget::RENDER_WIREFRAME);
        const bool renderCollisionMeshes = widget->GetRenderFlag(RenderViewWidget::RENDER_COLLISIONMESHES);

        if (renderVertexNormals || renderFaceNormals || renderTangents || renderWireframe || renderCollisionMeshes)
        {
            // iterate through all enabled nodes
            const EMotionFX::Pose* pose = actorInstance->GetTransformData()->GetCurrentPose();
            const size_t geomLODLevel   = actorInstance->GetLODLevel();
            const size_t numEnabled     = actorInstance->GetNumEnabledNodes();
            for (size_t i = 0; i < numEnabled; ++i)
            {
                EMotionFX::Node*    node            = emstudioActor->m_actor->GetSkeleton()->GetNode(actorInstance->GetEnabledNode(i));
                EMotionFX::Mesh*    mesh            = emstudioActor->m_actor->GetMesh(geomLODLevel, node->GetNodeIndex());
                const AZ::Transform globalTM        = pose->GetWorldSpaceTransform(node->GetNodeIndex()).ToAZTransform();

                renderUtil->ResetCurrentMesh();

                if (mesh == nullptr)
                {
                    continue;
                }

                if (mesh->GetIsCollisionMesh() == false)
                {
                    renderUtil->RenderNormals(mesh, globalTM, renderVertexNormals, renderFaceNormals, renderOptions->GetVertexNormalsScale() * emstudioActor->m_normalsScaleMultiplier, renderOptions->GetFaceNormalsScale() * emstudioActor->m_normalsScaleMultiplier, renderOptions->GetVertexNormalsColor(), renderOptions->GetFaceNormalsColor());
                    if (renderTangents)
                    {
                        renderUtil->RenderTangents(mesh, globalTM, renderOptions->GetTangentsScale() * emstudioActor->m_normalsScaleMultiplier, renderOptions->GetTangentsColor(), renderOptions->GetMirroredBitangentsColor(), renderOptions->GetBitangentsColor());
                    }
                    if (renderWireframe)
                    {
                        renderUtil->RenderWireframe(mesh, globalTM, renderOptions->GetWireframeColor());
                    }
                }
                else
                if (renderCollisionMeshes)
                {
                    renderUtil->RenderWireframe(mesh, globalTM, renderOptions->GetCollisionMeshColor());
                }
            }
        }

        // render the selection
        if (renderOptions->GetRenderSelectionBox() && EMotionFX::GetActorManager().GetNumActorInstances() != 1 && m_plugin->GetCurrentSelection()->CheckIfHasActorInstance(actorInstance))
        {
            AZ::Aabb aabb = actorInstance->GetAabb();
            aabb.Expand(aabb.GetExtents() * 0.005f);
            renderUtil->RenderSelection(aabb, renderOptions->GetSelectionColor());
        }

        // render node names
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODENAMES))
        {
            MCommon::Camera*    camera          = widget->GetRenderWidget()->GetCamera();
            const uint32        screenWidth     = widget->GetRenderWidget()->GetScreenWidth();
            const uint32        screenHeight    = widget->GetRenderWidget()->GetScreenHeight();

            renderUtil->RenderNodeNames(actorInstance, camera, screenWidth, screenHeight, renderOptions->GetNodeNameColor(), renderOptions->GetSelectedObjectColor(), visibleJointIndices, selectedJointIndices);
        }
    }
} // namespace EMStudio
