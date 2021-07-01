/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mEnableRendering    = true;
        mPlugin             = plugin;
    }


    // enable or disable rendering
    void RenderUpdateCallback::SetEnableRendering(bool renderingEnabled)
    {
        mEnableRendering = renderingEnabled;
    }


    // check for visibility
    // inside this example we just mark the character as visible always
    void RenderUpdateCallback::OnUpdateVisibilityFlags(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        // set to visible for the cases the active view widget is nullptr
        // this happens when call the Process() function from the render plugin before we update our view
        RenderViewWidget* widget = mPlugin->GetActiveViewWidget();
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
        MCommon::RenderUtil::TrajectoryTracePath* trajectoryPath = mPlugin->FindTracePath(actorInstance);
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
                if (trajectoryPath->mTraceParticles.GetIsEmpty())
                {
                    distanceTraveledEnough = true;
                }
                else
                {
                    const uint32 numParticles = trajectoryPath->mTraceParticles.GetLength();
                    const EMotionFX::Transform& oldGlobalTM = trajectoryPath->mTraceParticles[numParticles - 1].mWorldTM;

                    const AZ::Vector3& oldPos = oldGlobalTM.mPosition;
                    const AZ::Quaternion& oldRot = oldGlobalTM.mRotation;
                    const AZ::Quaternion rotation = globalTM.mRotation.GetNormalized();

                    const AZ::Vector3 deltaPos = globalTM.mPosition - oldPos;
                    float deltaRot = MCore::Math::Abs(rotation.Dot(oldRot));

                    if (MCore::SafeLength(deltaPos) > 0.0001f || deltaRot < 0.99f)
                    {
                        distanceTraveledEnough = true;
                    }
                }

                // add the time delta to the time passed since the last add
                trajectoryPath->mTimePassed += timePassedInSeconds;

                const uint32 particleSampleRate = 30;
                if (trajectoryPath->mTimePassed >= (1.0f / particleSampleRate) && distanceTraveledEnough)
                {
                    // create the particle, fill its data and add it to the trajectory trace path
                    MCommon::RenderUtil::TrajectoryPathParticle trajectoryParticle;
                    trajectoryParticle.mWorldTM = globalTM;
                    trajectoryPath->mTraceParticles.Add(trajectoryParticle);

                    // reset the time passed as we just added a new particle
                    trajectoryPath->mTimePassed = 0.0f;
                }
            }

            // make sure we don't have too many items in our array
            if (trajectoryPath->mTraceParticles.GetLength() > 50)
            {
                trajectoryPath->mTraceParticles.RemoveFirst();
            }
        }
    }


    // update a visible character
    void RenderUpdateCallback::OnRender(EMotionFX::ActorInstance* actorInstance, float timePassedInSeconds)
    {
        MCORE_UNUSED(timePassedInSeconds);

        if (mEnableRendering == false)
        {
            return;
        }

        RenderPlugin::EMStudioRenderActor* emstudioActor = mPlugin->FindEMStudioActor(actorInstance);
        if (emstudioActor == nullptr)
        {
            return;
        }

        // renderUtil options
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        actorInstance->UpdateMeshDeformers(timePassedInSeconds);

        // get the active widget & it's rendering options
        RenderViewWidget*   widget          = mPlugin->GetActiveViewWidget();
        RenderOptions*      renderOptions   = mPlugin->GetRenderOptions();

        const AZStd::unordered_set<AZ::u32>& visibleJointIndices = GetManager()->GetVisibleJointIndices();
        const AZStd::unordered_set<AZ::u32>& selectedJointIndices = GetManager()->GetSelectedJointIndices();

        // render the AABBs
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_AABB))
        {
            MCommon::RenderUtil::AABBRenderSettings settings;
            settings.mNodeBasedColor          = renderOptions->GetNodeAABBColor();
            settings.mStaticBasedColor        = renderOptions->GetStaticAABBColor();
            settings.mMeshBasedColor          = renderOptions->GetMeshAABBColor();
            settings.mCollisionMeshBasedColor = renderOptions->GetCollisionMeshAABBColor();

            renderUtil->RenderAABBs(actorInstance, settings);
        }

        if (widget->GetRenderFlag(RenderViewWidget::RENDER_OBB))
        {
            renderUtil->RenderOBBs(actorInstance, &visibleJointIndices, &selectedJointIndices, renderOptions->GetOBBsColor(), renderOptions->GetSelectedObjectColor());
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
            renderUtil->RenderSkeleton(actorInstance, emstudioActor->mBoneList, &visibleJointIndices, &selectedJointIndices, renderOptions->GetSkeletonColor(), renderOptions->GetSelectedObjectColor());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_NODEORIENTATION))
        {
            renderUtil->RenderNodeOrientations(actorInstance, emstudioActor->mBoneList, &visibleJointIndices, &selectedJointIndices, renderOptions->GetNodeOrientationScale(), renderOptions->GetScaleBonesOnLength());
        }
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_ACTORBINDPOSE))
        {
            renderUtil->RenderBindPose(actorInstance);
        }

        // render motion extraction debug info
        if (widget->GetRenderFlag(RenderViewWidget::RENDER_MOTIONEXTRACTION))
        {
            // render an arrow for the trajectory node
            //renderUtil->RenderTrajectoryNode(actorInstance, renderOptions->mTrajectoryArrowInnerColor, renderOptions->mTrajectoryArrowBorderColor, emstudioActor->mCharacterHeight*0.05f);
            renderUtil->RenderTrajectoryPath(mPlugin->FindTracePath(actorInstance), renderOptions->GetTrajectoryArrowInnerColor(), emstudioActor->mCharacterHeight * 0.05f);
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
            const uint32 geomLODLevel   = actorInstance->GetLODLevel();
            const uint32 numEnabled     = actorInstance->GetNumEnabledNodes();
            for (uint32 i = 0; i < numEnabled; ++i)
            {
                EMotionFX::Node*    node            = emstudioActor->mActor->GetSkeleton()->GetNode(actorInstance->GetEnabledNode(i));
                EMotionFX::Mesh*    mesh            = emstudioActor->mActor->GetMesh(geomLODLevel, node->GetNodeIndex());
                //EMotionFX::Mesh*  collisionMesh   = emstudioActor->mActor->GetCollisionMesh( geomLODLevel, node->GetNodeIndex() );
                const AZ::Transform globalTM        = pose->GetWorldSpaceTransform(node->GetNodeIndex()).ToAZTransform();

                renderUtil->ResetCurrentMesh();

                if (mesh == nullptr)
                {
                    continue;
                }

                if (mesh->GetIsCollisionMesh() == false)
                {
                    renderUtil->RenderNormals(mesh, globalTM, renderVertexNormals, renderFaceNormals, renderOptions->GetVertexNormalsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetFaceNormalsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetVertexNormalsColor(), renderOptions->GetFaceNormalsColor());
                    if (renderTangents)
                    {
                        renderUtil->RenderTangents(mesh, globalTM, renderOptions->GetTangentsScale() * emstudioActor->mNormalsScaleMultiplier, renderOptions->GetTangentsColor(), renderOptions->GetMirroredBitangentsColor(), renderOptions->GetBitangentsColor());
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
        if (renderOptions->GetRenderSelectionBox() && EMotionFX::GetActorManager().GetNumActorInstances() != 1 && mPlugin->GetCurrentSelection()->CheckIfHasActorInstance(actorInstance))
        {
            MCore::AABB aabb = actorInstance->GetAABB();
            aabb.Widen(aabb.CalcRadius() * 0.005f);
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
