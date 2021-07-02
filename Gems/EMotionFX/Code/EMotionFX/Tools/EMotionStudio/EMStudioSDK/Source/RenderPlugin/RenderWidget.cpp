/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
#include "RenderWidget.h"
#include "RenderPlugin.h"
#include <EMotionFX/Rendering/Common/OrbitCamera.h>
#include <EMotionFX/Rendering/Common/OrthographicCamera.h>
#include <EMotionFX/Rendering/Common/FirstPersonCamera.h>
#include <EMotionFX/CommandSystem/Source/SelectionList.h>
#include <EMotionFX/CommandSystem/Source/SelectionCommands.h>
#include <EMotionFX/CommandSystem/Source/ActorInstanceCommands.h>
#include <EMotionFX/Source/ActorInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/TransformData.h>
#include "../EMStudioManager.h"
#include "../MainWindow.h"
#include <MCore/Source/AzCoreConversions.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(RenderWidget::EventHandler, EMotionFX::EventHandlerAllocator, 0)


    // constructor
    RenderWidget::RenderWidget(RenderPlugin* renderPlugin, RenderViewWidget* viewWidget)
        : mEventHandler(this)
    {
        // create our event handler
        EMotionFX::GetEventManager().AddEventHandler(&mEventHandler);

        //mLines.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);
        //mLines.Reserve(2048);

        mSelectedActorInstances.SetMemoryCategory(MEMCATEGORY_EMSTUDIOSDK_RENDERPLUGINBASE);

        // camera used to render the little axis on the bottom left
        mAxisFakeCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_FRONT);

        mPlugin                 = renderPlugin;
        mViewWidget             = viewWidget;
        mWidth                  = 0;
        mHeight                 = 0;
        mViewCloseupWaiting     = 0;
        mPrevMouseX             = 0;
        mPrevMouseY             = 0;
        mPrevLocalMouseX        = 0;
        mPrevLocalMouseY        = 0;
        mOldActorInstancePos    = AZ::Vector3::CreateZero();
        mCamera                 = nullptr;
        mActiveTransformManip   = nullptr;
        mSkipFollowCalcs        = false;
        mNeedDisableFollowMode  = true;
    }


    // destructor
    RenderWidget::~RenderWidget()
    {
        // get rid of the event handler
        EMotionFX::GetEventManager().RemoveEventHandler(&mEventHandler);

        // get rid of the camera objects
        delete mCamera;
        delete mAxisFakeCamera;
    }

    // start view closeup flight
    void RenderWidget::ViewCloseup(const MCore::AABB& aabb, float flightTime, uint32 viewCloseupWaiting)
    {
        //LogError("ViewCloseup: AABB: Pos=(%.3f, %.3f, %.3f), Width=%.3f, Height=%.3f, Depth=%.3f", aabb.CalcMiddle().x, aabb.CalcMiddle().y, aabb.CalcMiddle().z, aabb.CalcWidth(), aabb.CalcHeight(), aabb.CalcDepth());
        mViewCloseupWaiting     = viewCloseupWaiting;
        mViewCloseupAABB        = aabb;
        mViewCloseupFlightTime  = flightTime;
    }

    void RenderWidget::ViewCloseup(bool selectedInstancesOnly, float flightTime, uint32 viewCloseupWaiting)
    {
        //LogError("ViewCloseup: AABB: Pos=(%.3f, %.3f, %.3f), Width=%.3f, Height=%.3f, Depth=%.3f", aabb.CalcMiddle().x, aabb.CalcMiddle().y, aabb.CalcMiddle().z, aabb.CalcWidth(), aabb.CalcHeight(), aabb.CalcDepth());
        mViewCloseupWaiting     = viewCloseupWaiting;
        mViewCloseupAABB        = mPlugin->GetSceneAABB(selectedInstancesOnly);
        mViewCloseupFlightTime  = flightTime;
    }


    // switch the active camera
    void RenderWidget::SwitchCamera(CameraMode mode)
    {
        delete mCamera;
        mCameraMode = mode;

        switch (mode)
        {
        case CAMMODE_ORBIT:
        {
            mCamera = new MCommon::OrbitCamera();
            break;
        }
        case CAMMODE_FIRSTPERSON:
        {
            mCamera = new MCommon::FirstPersonCamera();
            break;
        }
        case CAMMODE_FRONT:
        {
            mCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_FRONT);
            break;
        }
        case CAMMODE_BACK:
        {
            mCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_BACK);
            break;
        }
        case CAMMODE_LEFT:
        {
            mCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_LEFT);
            break;
        }
        case CAMMODE_RIGHT:
        {
            mCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_RIGHT);
            break;
        }
        case CAMMODE_TOP:
        {
            mCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_TOP);
            break;
        }
        case CAMMODE_BOTTOM:
        {
            mCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_BOTTOM);
            break;
        }
        }

        // show the entire scene
        mPlugin->ViewCloseup(false, this, 0.0f);
    }


    // calculates the camera distance used for the gizmo sizing
    void RenderWidget::UpdateActiveTransformationManipulator(MCommon::TransformationManipulator* activeManipulator)
    {
        // check if active manipulator was set
        if (activeManipulator == nullptr)
        {
            return;
        }

        // get callback pointer
        MCommon::ManipulatorCallback* callback = activeManipulator->GetCallback();
        if (callback == nullptr)
        {
            return;
        }

        // init the camera distance
        float camDist = 0.0f;

        // calculate cam distance for the orthographic cam mode
        if (mCamera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC)
        {
            camDist = 0.75f;
            switch (GetCameraMode())
            {
            case CAMMODE_FRONT:
            case CAMMODE_BOTTOM:
                // -(scale.x)
                camDist *= -2.0f / static_cast<float>(mCamera->GetViewProjMatrix().GetElement(0, 0));
                break;
            case CAMMODE_BACK:
            case CAMMODE_TOP:
                // scale.x
                camDist *= 2.0f / static_cast<float>(mCamera->GetViewProjMatrix().GetElement(0, 0));
                break;
            case CAMMODE_LEFT:
                // -(scale.y)
                camDist *= -2.0f / static_cast<float>(mCamera->GetViewProjMatrix().GetElement(0, 1));
                break;
            case CAMMODE_RIGHT:
                // scale.y
                camDist *= 2.0f / static_cast<float>(mCamera->GetViewProjMatrix().GetElement(0, 1));
                break;
            default:
                break;
            }
        }
        // calculate cam distance for perspective cam
        else
        {
            if (activeManipulator->GetSelectionLocked() &&
                mViewWidget->GetIsCharacterFollowModeActive() == false &&
                activeManipulator->GetType() == MCommon::TransformationManipulator::GIZMOTYPE_TRANSLATION)
            {
                camDist = (callback->GetOldValueVec() - mCamera->GetPosition()).GetLength();
            }
            else
            {
                camDist = (activeManipulator->GetPosition() - mCamera->GetPosition()).GetLength();
            }
        }

        // adjust the scale of the manipulator
        if (activeManipulator->GetType() == MCommon::TransformationManipulator::GIZMOTYPE_TRANSLATION)
        {
            activeManipulator->SetScale(aznumeric_cast<float>(camDist * 0.12));
        }
        else if (activeManipulator->GetType() == MCommon::TransformationManipulator::GIZMOTYPE_ROTATION)
        {
            activeManipulator->SetScale(aznumeric_cast<float>(camDist * 0.8));
        }
        else if (activeManipulator->GetType() == MCommon::TransformationManipulator::GIZMOTYPE_SCALE)
        {
            activeManipulator->SetScale(aznumeric_cast<float>(camDist * 0.15), mCamera);
        }

        // update position of the actor instance (needed for camera follow mode)
        EMotionFX::ActorInstance* actorInstance = callback->GetActorInstance();
        if (actorInstance)
        {
            activeManipulator->Init(actorInstance->GetLocalSpaceTransform().mPosition);
        }
    }


    // handle mouse move event
    void RenderWidget::OnMouseMoveEvent(QWidget* renderWidget, QMouseEvent* event)
    {
        // calculate the delta mouse movement
        int32 deltaX = event->globalX() - mPrevMouseX;
        int32 deltaY = event->globalY() - mPrevMouseY;

        // store the current value as previous value
        mPrevMouseX = event->globalX();
        mPrevMouseY = event->globalY();
        mPrevLocalMouseX = event->x();
        mPrevLocalMouseY = event->y();

        // get the button states
        const bool leftButtonPressed            = event->buttons() & Qt::LeftButton;
        const bool middleButtonPressed          = event->buttons() & Qt::MidButton;
        const bool rightButtonPressed           = event->buttons() & Qt::RightButton;
        //const bool singleLeftButtonPressed    = (leftButtonPressed) && (middleButtonPressed == false) && (rightButtonPressed == false);
        const bool altPressed                   = event->modifiers() & Qt::AltModifier;
        bool gizmoHit                           = false;

        // accumulate the number of pixels moved since the last right click
        if (leftButtonPressed == false && middleButtonPressed == false && rightButtonPressed && altPressed == false)
        {
            mPixelsMovedSinceRightClick += (int32)MCore::Math::Abs(aznumeric_cast<float>(deltaX)) + (int32)MCore::Math::Abs(aznumeric_cast<float>(deltaY));
        }

        // update size/bounding volumes volumes of all existing gizmos
        const MCore::Array<MCommon::TransformationManipulator*>* transformationManipulators = GetManager()->GetTransformationManipulators();

        // render all visible gizmos
        const uint32 numGizmos = transformationManipulators->GetLength();
        for (uint32 i = 0; i < numGizmos; ++i)
        {
            MCommon::TransformationManipulator* activeManipulator = transformationManipulators->GetItem(i);
            if (activeManipulator == nullptr)
            {
                continue;
            }

            // get camera position for size adjustments of the gizmos
            UpdateActiveTransformationManipulator(activeManipulator);
        }

        // get the translate manipulator
        MCommon::TransformationManipulator* mouseOveredManip = mPlugin->GetActiveManipulator(mCamera, event->x(), event->y());

        // check if the current manipulator is hit
        if (mouseOveredManip)
        {
            gizmoHit = mouseOveredManip->Hit(mCamera, event->x(), event->y());
        }
        else
        {
            mouseOveredManip = mActiveTransformManip;
        }

        // flag to check if mouse wrapping occured
        bool mouseWrapped = false;

        // map the cursor if it goes out of the screen
        //if (activeManipulator != (MCommon::TransformationManipulator*)translateManipulator || (translateManipulator && translateManipulator->GetMode() == MCommon::TranslateManipulator::TRANSLATE_NONE))
        if (mouseOveredManip == nullptr || (mouseOveredManip && mouseOveredManip->GetType() != MCommon::TransformationManipulator::GIZMOTYPE_TRANSLATION))
        {
            const int32 width = mCamera->GetScreenWidth();
            const int32 height = mCamera->GetScreenHeight();

            // handle mouse wrapping, to enable smoother panning
            if (event->x() > (int32)width)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() - width, event->globalY()));
                mPrevMouseX = event->globalX() - width;
            }
            else if (event->x() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() + width, event->globalY()));
                mPrevMouseX = event->globalX() + width;
            }

            if (event->y() > (int32)height)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() - height));
                mPrevMouseY = event->globalY() - height;
            }
            else if (event->y() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() + height));
                mPrevMouseY = event->globalY() + height;
            }

            // don't apply the delta, if mouse has been wrapped
            if (mouseWrapped)
            {
                deltaX = 0;
                deltaY = 0;
            }
        }

        // update the gizmos
        if (mouseOveredManip)
        {
            // adjust the mouse cursor, depending on the gizmo state
            if (gizmoHit && leftButtonPressed == false)
            {
                renderWidget->setCursor(Qt::OpenHandCursor);
            }
            else if (mouseOveredManip->GetSelectionLocked())
            {
                if (mNeedDisableFollowMode)
                {
                    MCommon::ManipulatorCallback* callback = mouseOveredManip->GetCallback();
                    if (callback)
                    {
                        if (callback->GetResetFollowMode())
                        {
                            mIsCharacterFollowModeActive = mViewWidget->GetIsCharacterFollowModeActive();
                            mViewWidget->SetCharacterFollowModeActive(false);
                            mNeedDisableFollowMode = false;
                        }
                    }
                }
                renderWidget->setCursor(Qt::ClosedHandCursor);
            }
            else
            {
                renderWidget->setCursor(Qt::ArrowCursor);
            }

            // update the gizmo position
            /*
            ActorInstance* actorInstance = GetCommandManager()->GetCurrentSelection().GetSingleActorInstance();
            if (actorInstance && altPressed == false)
                activeManipulator->Init( actorInstance->GetLocalPos() );
            */

            // send mouse movement to the manipulators
            mouseOveredManip->ProcessMouseInput(mCamera, event->x(), event->y(), deltaX, deltaY, leftButtonPressed && !altPressed, middleButtonPressed, rightButtonPressed);
        }
        else
        {
            // set the default cursor if the cursor is not on a gizmo
            renderWidget->setCursor(Qt::ArrowCursor);
        }

        // camera movement
        if (leftButtonPressed && altPressed == false && !(leftButtonPressed && rightButtonPressed))
        {
            //setCursor(Qt::ArrowCursor);
        }
        else
        {
            // adjust the camera based on keyboard and mouse input
            if (mCamera)
            {
                switch (mCameraMode)
                {
                case CAMMODE_ORBIT:
                {
                    // rotate camera around target point
                    if (leftButtonPressed && rightButtonPressed == false && middleButtonPressed == false)
                    {
                        renderWidget->setCursor(Qt::ClosedHandCursor);
                    }
                    // zoom camera in or out
                    if (leftButtonPressed == false && rightButtonPressed && middleButtonPressed == false)
                    {
                        if (deltaY < 0)
                        {
                            renderWidget->setCursor(mPlugin->GetZoomOutCursor());
                        }
                        else
                        {
                            renderWidget->setCursor(mPlugin->GetZoomInCursor());
                        }
                    }
                    // move camera forward, backward, left or right
                    if ((leftButtonPressed == false && rightButtonPressed == false && middleButtonPressed) ||
                        (leftButtonPressed && rightButtonPressed && middleButtonPressed == false))
                    {
                        renderWidget->setCursor(Qt::SizeAllCursor);
                    }

                    break;
                }

                default:
                {
                    // rotate camera around target point
                    if (leftButtonPressed && rightButtonPressed == false && middleButtonPressed == false)
                    {
                        renderWidget->setCursor(Qt::ForbiddenCursor);
                    }
                    // zoom camera in or out
                    if (leftButtonPressed == false && rightButtonPressed && middleButtonPressed == false)
                    {
                        if (deltaY < 0)
                        {
                            renderWidget->setCursor(mPlugin->GetZoomOutCursor());
                        }
                        else
                        {
                            renderWidget->setCursor(mPlugin->GetZoomInCursor());
                        }
                    }
                    // move camera forward, backward, left or right
                    if ((leftButtonPressed == false && rightButtonPressed == false && middleButtonPressed) ||
                        (leftButtonPressed && rightButtonPressed && middleButtonPressed == false))
                    {
                        renderWidget->setCursor(Qt::SizeAllCursor);
                    }

                    break;
                }
                }

                mCamera->ProcessMouseInput(deltaX, deltaY, leftButtonPressed, middleButtonPressed, rightButtonPressed);
                mCamera->Update();
            }
        }

        //LogInfo( "mouseMoveEvent: x=%i, y=%i, deltaX=%i, deltaY=%i, globalX=%i, globalY=%i, wrapped=%i", event->x(), event->y(), deltaX, deltaY, event->globalX(), event->globalY(), mouseWrapped );
    }


    // handle mouse button press event
    void RenderWidget::OnMousePressEvent(QWidget* renderWidget, QMouseEvent* event)
    {
        // reset the number of pixels moved since the last right click
        mPixelsMovedSinceRightClick = 0;

        // calculate the delta mouse movement and set old mouse position
        //const int32 deltaX = event->globalX() - mPrevMouseX;
        //const int32 deltaY = event->globalY() - mPrevMouseY;
        mPrevMouseX = event->globalX();
        mPrevMouseY = event->globalY();

        // get the button states
        const bool leftButtonPressed    = event->buttons() & Qt::LeftButton;
        const bool middleButtonPressed  = event->buttons() & Qt::MidButton;
        const bool rightButtonPressed   = event->buttons() & Qt::RightButton;
        const bool ctrlPressed          = event->modifiers() & Qt::ControlModifier;
        const bool altPressed           = event->modifiers() & Qt::AltModifier;

        // set the click position if right click was done
        if (rightButtonPressed)
        {
            mRightClickPosX = QCursor::pos().x();
            mRightClickPosY = QCursor::pos().y();
        }

        // get the current selection
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // check if the active gizmo is hit by the mouse (left click)
        bool gizmoHit = false;
        MCommon::TransformationManipulator* activeManipulator = nullptr;
        if (leftButtonPressed && middleButtonPressed == false && rightButtonPressed == false)
        {
            activeManipulator = mPlugin->GetActiveManipulator(mCamera, event->x(), event->y());
        }

        if (activeManipulator)
        {
            gizmoHit = (activeManipulator->GetMode() != 0);

            MCommon::ManipulatorCallback* callback = activeManipulator->GetCallback();
            if (callback)
            {
                if (gizmoHit && callback->GetResetFollowMode())
                {
                    mIsCharacterFollowModeActive = mViewWidget->GetIsCharacterFollowModeActive();
                    mViewWidget->SetCharacterFollowModeActive(false);
                    mNeedDisableFollowMode = false;

                    mActiveTransformManip = activeManipulator;
                    mActiveTransformManip->ProcessMouseInput(mCamera, event->x(), event->y(), 0, 0, leftButtonPressed && !altPressed, middleButtonPressed, rightButtonPressed);
                }
            }

            // set the closed hand cursor if hit the gizmo
            if (gizmoHit)
            {
                renderWidget->setCursor(Qt::ClosedHandCursor);
            }
            else
            {
                renderWidget->setCursor(Qt::ArrowCursor);
            }
        }
        else
        {
            // set the default cursor if the cursor is not on a gizmo
            renderWidget->setCursor(Qt::ArrowCursor);
        }

        // handle visual mouse selection
        if (EMStudio::GetCommandManager()->GetLockSelection() == false && gizmoHit == false) // avoid selection operations when there is only one actor instance
        {
            AZ::u32 editorActorInstanceCount = 0;
            const EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
            const AZ::u32 totalActorInstanceCount = actorManager.GetNumActorInstances();
            for (AZ::u32 i = 0; i < totalActorInstanceCount; ++i)
            {
                const EMotionFX::ActorInstance* actorInstance = actorManager.GetActorInstance(i);
                if (!actorInstance->GetIsOwnedByRuntime())
                {
                    editorActorInstanceCount++;
                }
            }

            // only allow selection changes when there are multiple actors or when there is only one actor but that one is not selected
            if (editorActorInstanceCount != 1 || !selection.GetSingleActorInstance())
            {
                if (event->buttons() & Qt::LeftButton &&
                    (event->modifiers() & Qt::AltModifier) == false &&
                    (event->buttons() & Qt::MidButton) == false &&
                    (event->buttons() & Qt::RightButton) == false)
                {
                    QPoint tempMousePos = renderWidget->mapFromGlobal(QCursor::pos());
                    const int32 mousePosX = tempMousePos.x();
                    const int32 mousePosY = tempMousePos.y();

                    EMotionFX::ActorInstance* selectedActorInstance = nullptr;
                    AZ::Vector3 oldIntersectionPoint;

                    const MCore::Ray ray = mCamera->Unproject(mousePosX, mousePosY);

                    // get the number of actor instances and iterate through them
                    const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
                    for (uint32 i = 0; i < numActorInstances; ++i)
                    {
                        EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
                        if (actorInstance->GetIsVisible() == false || actorInstance->GetRender() == false || actorInstance->GetIsUsedForVisualization() || actorInstance->GetIsOwnedByRuntime())
                        {
                            continue;
                        }

                        // update the mesh so that the currently checked actor instance always uses the most up to date mesh (remember: meshes are shared across actor instances)
                        actorInstance->UpdateTransformations(0.0f, true);
                        actorInstance->UpdateMeshDeformers(0.0f, true);

                        // check for an intersection
                        AZ::Vector3 intersect, normal;
                        EMotionFX::Node* intersectionNode = actorInstance->IntersectsMesh(0, ray, &intersect, &normal);
                        if (intersectionNode)
                        {
                            if (selectedActorInstance == nullptr)
                            {
                                selectedActorInstance = actorInstance;
                                oldIntersectionPoint = intersect;
                            }
                            else
                            {
                                // find the actor instance closer to the camera
                                const float distOld = (mCamera->GetPosition() - oldIntersectionPoint).GetLength();
                                const float distNew = (mCamera->GetPosition() - intersect).GetLength();
                                if (distNew < distOld)
                                {
                                    selectedActorInstance = actorInstance;
                                    oldIntersectionPoint = intersect;
                                }
                            }
                        }
                        else
                        {
                            // check if the actor has any meshes at all, if not use the node based AABB for selection
                            EMotionFX::Actor* actor = actorInstance->GetActor();
                            if (actor->CheckIfHasMeshes(actorInstance->GetLODLevel()) == false)
                            {
                                // calculate the node based AABB
                                MCore::AABB box;
                                actorInstance->CalcNodeBasedAABB(&box);

                                // render the aabb
                                if (box.CheckIfIsValid())
                                {
                                    AZ::Vector3 ii, n;
                                    if (ray.Intersects(box, &ii, &n))
                                    {
                                        selectedActorInstance = actorInstance;
                                        oldIntersectionPoint = ii;
                                    }
                                }
                            }
                        }
                    }

                    mSelectedActorInstances.Clear(false);

                    if (ctrlPressed)
                    {
                        // add the old selection to the selected actor instances (selection mode = add)
                        const uint32 numSelectedActorInstances = selection.GetNumSelectedActorInstances();
                        for (uint32 i = 0; i < numSelectedActorInstances; ++i)
                        {
                            mSelectedActorInstances.Add(selection.GetActorInstance(i));
                        }
                    }

                    if (selectedActorInstance)
                    {
                        mSelectedActorInstances.Add(selectedActorInstance);
                    }

                    CommandSystem::SelectActorInstancesUsingCommands(mSelectedActorInstances);
                }
            }
        }
    }


    // handle mouse button release event
    void RenderWidget::OnMouseReleaseEvent(QWidget* renderWidget, QMouseEvent* event)
    {
        // apply the transformation of the gizmos
        const bool altPressed = event->modifiers() & Qt::AltModifier;
        if (altPressed == false)
        {
            // check which manipulator is currently mouse-overed and use the active one in case we're not hoving any
            MCommon::TransformationManipulator* mouseOveredManip = mPlugin->GetActiveManipulator(mCamera, event->x(), event->y());
            if (mouseOveredManip == nullptr)
            {
                mouseOveredManip = mActiveTransformManip;
            }

            // only do in case a manipulator got hovered or is active
            if (mouseOveredManip)
            {
                // apply the current transformation if the mouse was released
                MCommon::ManipulatorCallback* callback = mouseOveredManip->GetCallback();
                if (callback)
                {
                    callback->ApplyTransformation();
                }

                // the manipulator
                mouseOveredManip->ProcessMouseInput(mCamera, 0, 0, 0, 0, false, false, false);

                // reset the camera follow mode state
                if (callback && callback->GetResetFollowMode() && mIsCharacterFollowModeActive)
                {
                    mViewWidget->SetCharacterFollowModeActive(mIsCharacterFollowModeActive);
                    mSkipFollowCalcs = true;

                    /*  CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
                        ActorInstance* followInstance = selectionList.GetFirstActorInstance();
                        if (followInstance)
                            mOldActorInstancePos = followInstance->GetLocalPos();*/

                    //mViewWidget->OnFollowCharacter();
                }
            }
        }

        // reset the active manipulator
        mActiveTransformManip = nullptr;

        // reset the disable follow flag
        mNeedDisableFollowMode = true;

        // set the arrow cursor
        renderWidget->setCursor(Qt::ArrowCursor);

        // context menu handling
        if (mPixelsMovedSinceRightClick < 5)
        {
            OnContextMenuEvent(renderWidget, event->modifiers() & Qt::ControlModifier, event->modifiers() & Qt::AltModifier, event->x(), event->y(), event->globalPos());
        }
    }


    // handle mouse wheel event
    void RenderWidget::OnWheelEvent(QWidget* renderWidget, QWheelEvent* event)
    {
        MCORE_UNUSED(renderWidget);

        mCamera->ProcessMouseInput(0,
            event->angleDelta().y(),
            false,
            false,
            true
        );

        mCamera->Update();
    }


    // handles context menu events
    void RenderWidget::OnContextMenuEvent(QWidget* renderWidget, bool shiftPressed, bool altPressed, int32 localMouseX, int32 localMouseY, QPoint globalMousePos)
    {
        // stop context menu execution, if mouse position changed or alt is pressed
        // so block it if zooming, moving etc. is enabled
        if (QCursor::pos().x() != mRightClickPosX || QCursor::pos().y() != mRightClickPosY || altPressed)
        {
            return;
        }

        // call the context menu handler
        mViewWidget->OnContextMenuEvent(renderWidget, shiftPressed, localMouseX, localMouseY, globalMousePos, mPlugin, mCamera);
    }


    void RenderWidget::RenderAxis()
    {
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // set the camera used to render the axis
        MCommon::Camera* camera = mCamera;
        if (mCamera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            camera = mAxisFakeCamera;
        }

        // store the old projection mode so that we can set it back later on
        const MCommon::Camera::ProjectionMode oldProjectionMode = camera->GetProjectionMode();
        camera->SetProjectionMode(MCommon::Camera::PROJMODE_ORTHOGRAPHIC);

        // store the old far clip distance so that we can set it back later on
        const float oldFarClipDistance = camera->GetFarClipDistance();
        camera->SetFarClipDistance(1000.0f);

        // fake zoom the camera so that we draw the axis in a nice size and remember the old distance
        int32 distanceFromBorder    = 40;
        float size                  = 25;
        if (mCamera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            MCommon::OrthographicCamera* orgCamera = (MCommon::OrthographicCamera*)mCamera;
            MCommon::OrthographicCamera* orthoCamera = (MCommon::OrthographicCamera*)camera;
            orthoCamera->SetCurrentDistance(1.0f);
            orthoCamera->SetPosition(orgCamera->GetPosition());
            orthoCamera->SetMode(orgCamera->GetMode());
            orthoCamera->SetScreenDimensions(mWidth, mHeight);
            size *= 0.001f;
        }

        // update the camera
        camera->SetOrthoClipDimensions(AZ::Vector2(aznumeric_cast<float>(mWidth), aznumeric_cast<float>(mHeight)));
        camera->Update();

        MCommon::RenderUtil::AxisRenderingSettings axisRenderingSettings;
        int32 originScreenX = 0;
        int32 originScreenY = 0;
        switch (mCameraMode)
        {
        case CAMMODE_ORBIT:
        {
            originScreenX = distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = true;
            axisRenderingSettings.mRenderYAxis = true;
            axisRenderingSettings.mRenderZAxis = true;
            break;
        }
        case CAMMODE_FIRSTPERSON:
        {
            originScreenX = distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = true;
            axisRenderingSettings.mRenderYAxis = true;
            axisRenderingSettings.mRenderZAxis = true;
            break;
        }
        case CAMMODE_FRONT:
        {
            originScreenX = distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = true;
            axisRenderingSettings.mRenderYAxis = true;
            axisRenderingSettings.mRenderZAxis = false;
            break;
        }
        case CAMMODE_BACK:
        {
            originScreenX = 2 * distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = true;
            axisRenderingSettings.mRenderYAxis = true;
            axisRenderingSettings.mRenderZAxis = false;
            break;
        }
        case CAMMODE_LEFT:
        {
            originScreenX = distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = false;
            axisRenderingSettings.mRenderYAxis = true;
            axisRenderingSettings.mRenderZAxis = true;
            break;
        }
        case CAMMODE_RIGHT:
        {
            originScreenX = 2 * distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = false;
            axisRenderingSettings.mRenderYAxis = true;
            axisRenderingSettings.mRenderZAxis = true;
            break;
        }
        case CAMMODE_TOP:
        {
            originScreenX = distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = true;
            axisRenderingSettings.mRenderYAxis = false;
            axisRenderingSettings.mRenderZAxis = true;
            break;
        }
        case CAMMODE_BOTTOM:
        {
            originScreenX = 2 * distanceFromBorder;
            originScreenY = mHeight - distanceFromBorder;
            axisRenderingSettings.mRenderXAxis = true;
            axisRenderingSettings.mRenderYAxis = false;
            axisRenderingSettings.mRenderZAxis = true;
            break;
        }
        default:
            MCORE_ASSERT(false);
        }

        const AZ::Vector3 axisPosition = MCore::UnprojectOrtho(aznumeric_cast<float>(originScreenX), aznumeric_cast<float>(originScreenY), aznumeric_cast<float>(mWidth), aznumeric_cast<float>(mHeight), 0.0f, camera->GetProjectionMatrix(), camera->GetViewMatrix());

        AZ::Matrix4x4 inverseCameraMatrix = camera->GetViewMatrix();
        inverseCameraMatrix.InvertFull();

        AZ::Transform worldTM = AZ::Transform::CreateIdentity();
        worldTM.SetTranslation(axisPosition);

        axisRenderingSettings.mSize             = size;
        axisRenderingSettings.mWorldTM          = worldTM;
        axisRenderingSettings.mCameraRight      = MCore::GetRight(inverseCameraMatrix).GetNormalized();
        axisRenderingSettings.mCameraUp         = MCore::GetUp(inverseCameraMatrix).GetNormalized();
        axisRenderingSettings.mRenderXAxisName  = true;
        axisRenderingSettings.mRenderYAxisName  = true;
        axisRenderingSettings.mRenderZAxisName  = true;

        // render directly as we have to disable the depth test, hope the additional render call won't slow down so much
        renderUtil->RenderLineAxis(axisRenderingSettings);
        renderUtil->RenderLines();

        // set the adjusted attributes back to the original values and reset the used camera
        camera->SetProjectionMode(oldProjectionMode);
        camera->SetFarClipDistance(oldFarClipDistance);
        camera->Update();
    }


    void RenderWidget::RenderNodeFilterString()
    {
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // render the camera mode name at the bottom of the gl widget
        const char*         text                = mCamera->GetTypeString();
        const uint32        textSize            = 10;
        const uint32        cameraNameColor     = MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f).ToInt();
        const uint32        cameraNameX         = aznumeric_cast<uint32>(mWidth * 0.5f);
        const uint32        cameraNameY         = mHeight - 20;

        renderUtil->RenderText(aznumeric_cast<float>(cameraNameX), aznumeric_cast<float>(cameraNameY), text, cameraNameColor, textSize, true);
        //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        //renderText(screenX, screenY, text);

        // make sure the lines are rendered directly afterwards
        renderUtil->Render2DLines();
    }


    void RenderWidget::UpdateCharacterFollowModeData()
    {
        if (mViewWidget->GetIsCharacterFollowModeActive())
        {
            const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
            EMotionFX::ActorInstance* followInstance = selectionList.GetFirstActorInstance();

            if (followInstance && mCamera)
            {
                const AZ::Vector3& localPos = followInstance->GetLocalSpaceTransform().mPosition;
                mPlugin->GetTranslateManipulator()->Init(localPos);
                mPlugin->GetRotateManipulator()->Init(localPos);
                mPlugin->GetScaleManipulator()->Init(localPos);

                AZ::Vector3 actorInstancePos;

                EMotionFX::Actor* followActor = followInstance->GetActor();
                const uint32 motionExtractionNodeIndex = followActor->GetMotionExtractionNodeIndex();
                if (motionExtractionNodeIndex != MCORE_INVALIDINDEX32)
                {
                    actorInstancePos = followInstance->GetWorldSpaceTransform().mPosition;
                    RenderPlugin::EMStudioRenderActor* emstudioActor = mPlugin->FindEMStudioActor(followActor);
                    if (emstudioActor)
                    {
                        #ifndef EMFX_SCALE_DISABLED
                            const float scaledOffsetFromTrajectoryNode = followInstance->GetWorldSpaceTransform().mScale.GetZ() * emstudioActor->mOffsetFromTrajectoryNode;
                        #else
                            const float scaledOffsetFromTrajectoryNode = 1.0f;
                        #endif
                        actorInstancePos.SetZ(actorInstancePos.GetZ() + scaledOffsetFromTrajectoryNode);
                    }
                }
                else
                {
                    actorInstancePos = followInstance->GetWorldSpaceTransform().mPosition;
                }

                // Calculate movement since last frame.
                AZ::Vector3 deltaPos = actorInstancePos - mOldActorInstancePos;

                if (mSkipFollowCalcs)
                {
                    deltaPos = AZ::Vector3::CreateZero();
                    mSkipFollowCalcs = false;
                }

                mOldActorInstancePos = actorInstancePos;

                switch (mCamera->GetType())
                {
                case MCommon::OrbitCamera::TYPE_ID:
                {
                    MCommon::OrbitCamera* orbitCamera = static_cast<MCommon::OrbitCamera*>(mCamera);

                    if (orbitCamera->GetIsFlightActive())
                    {
                        orbitCamera->SetFlightTargetPosition(actorInstancePos);
                    }
                    else
                    {
                        orbitCamera->SetPosition(orbitCamera->GetPosition() + deltaPos);
                        orbitCamera->SetTarget(orbitCamera->GetTarget() + deltaPos);
                    }

                    break;
                }

                case MCommon::OrthographicCamera::TYPE_ID:
                {
                    MCommon::OrthographicCamera* orthoCamera = static_cast<MCommon::OrthographicCamera*>(mCamera);

                    if (orthoCamera->GetIsFlightActive())
                    {
                        orthoCamera->SetFlightTargetPosition(actorInstancePos);
                    }
                    else
                    {
                        orthoCamera->SetPosition(orthoCamera->GetPosition() + deltaPos);
                    }

                    break;
                }
                }
            }
        }
        else
        {
            mOldActorInstancePos.Set(0.0f, 0.0f, 0.0f);
        }
    }


    // render the manipulator gizmos
    void RenderWidget::RenderManipulators()
    {
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        MCore::Array<MCommon::TransformationManipulator*>* transformationManipulators = GetManager()->GetTransformationManipulators();
        const uint32 numGizmos = transformationManipulators->GetLength();

        // render all visible gizmos
        for (uint32 i = 0; i < numGizmos; ++i)
        {
            // update the gizmos
            MCommon::TransformationManipulator* activeManipulator = transformationManipulators->GetItem(i);

            // update the gizmos if there is an active manipulator
            if (activeManipulator == nullptr)
            {
                continue;
            }

            // update the active manipulator depending on camera distance and mode
            UpdateActiveTransformationManipulator(activeManipulator);

            // render the current actor
            activeManipulator->Render(mCamera, renderUtil);
        }

        // render any remaining lines
        if (renderUtil)
        {
            renderUtil->RenderLines();
        }
    }


    // render all triangles that got added to the render util
    void RenderWidget::RenderTriangles()
    {
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // render custom triangles
        const uint32 numTriangles = mTriangles.GetLength();
        for (uint32 i = 0; i < numTriangles; ++i)
        {
            const Triangle& curTri = mTriangles[i];
            renderUtil->AddTriangle(curTri.mPosA, curTri.mPosB, curTri.mPosC, curTri.mNormalA, curTri.mNormalB, curTri.mNormalC, curTri.mColor); // TODO: make renderutil use uint32 colors instead
        }

        ClearTriangles();
        renderUtil->RenderTriangles();
    }


    // iterate through all plugins and render their helper data
    void RenderWidget::RenderCustomPluginData()
    {
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // render all custom plugin visuals
        const uint32 numPlugins = GetPluginManager()->GetNumActivePlugins();
        for (uint32 i = 0; i < numPlugins; ++i)
        {
            EMStudioPlugin* plugin = GetPluginManager()->GetActivePlugin(i);
            EMStudioPlugin::RenderInfo renderInfo(renderUtil, mCamera, mWidth, mHeight);

            plugin->Render(mPlugin, &renderInfo);
        }

        RenderDebugDraw();

        // render all triangles
        RenderTriangles();
    }


    void RenderWidget::RenderDebugDraw()
    {
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (!renderUtil)
        {
            return;
        }

        EMotionFX::DebugDraw& debugDraw = EMotionFX::GetDebugDraw();
        debugDraw.Lock();
        for (const auto& item : debugDraw.GetActorInstanceData())
        {
            EMotionFX::DebugDraw::ActorInstanceData* actorInstanceData = item.second;
            actorInstanceData->Lock();
            int numLines = 0;
            for (const EMotionFX::DebugDraw::Line& line : actorInstanceData->GetLines())
            {
                const MCore::RGBAColor color(line.m_startColor.GetR(), line.m_startColor.GetG(), line.m_startColor.GetB(), line.m_startColor.GetA());
                renderUtil->RenderLine(line.m_start, line.m_end, color);
                numLines++;
            }
            actorInstanceData->Unlock();
        }
        renderUtil->RenderLines();
        debugDraw.Unlock();
    }


    // render solid characters
    void RenderWidget::RenderActorInstances()
    {
        MCommon::RenderUtil* renderUtil = mPlugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // backface culling
        const bool backfaceCullingEnabled = mViewWidget->GetRenderFlag(RenderViewWidget::RENDER_BACKFACECULLING);
        renderUtil->EnableCulling(backfaceCullingEnabled);

        EMotionFX::GetAnimGraphManager().SetAnimGraphVisualizationEnabled(true);

        // Only keep the following line when we do not link to the update system component OnTick anymore.
        /////        EMotionFX::GetEMotionFX().Update(0.0f);

        // render
        const uint32 numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (uint32 i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            if (actorInstance->GetRender() && actorInstance->GetIsVisible() && actorInstance->GetIsOwnedByRuntime() == false)
            {
                mPlugin->RenderActorInstance(actorInstance, 0.0f);
            }
        }
    }


    // prepare the camera
    void RenderWidget::UpdateCamera()
    {
        if (mCamera == nullptr)
        {
            return;
        }

        RenderOptions* renderOptions = mPlugin->GetRenderOptions();

        // update the camera
        mCamera->SetNearClipDistance(renderOptions->GetNearClipPlaneDistance());
        mCamera->SetFarClipDistance(renderOptions->GetFarClipPlaneDistance());
        mCamera->SetFOV(renderOptions->GetFOV());
        mCamera->SetAspectRatio(mWidth / (float)mHeight);
        mCamera->SetScreenDimensions(mWidth, mHeight);
        mCamera->AutoUpdateLimits();

        if (mViewCloseupWaiting != 0 && mHeight != 0 && mWidth != 0)
        {
            mViewCloseupWaiting--;
            if (mViewCloseupWaiting == 0)
            {
                mCamera->ViewCloseup(mViewCloseupAABB, mViewCloseupFlightTime);
                //mViewCloseupWaiting = 0;
            }
        }

        // update the manipulators, camera, old actor instance position etc. when using the character follow mode
        UpdateCharacterFollowModeData();

        mCamera->Update();
    }


    // render grid in lines and checkerboard
    void RenderWidget::RenderGrid()
    {
        // directly return in case we do not want to render any type of grid
        if (mViewWidget->GetRenderFlag(RenderViewWidget::RENDER_GRID) == false)
        {
            return;
        }

        // get access to the render utility and render options
        MCommon::RenderUtil*    renderUtil      = mPlugin->GetRenderUtil();
        RenderOptions*          renderOptions   = mPlugin->GetRenderOptions();
        if (renderUtil == nullptr || renderOptions == nullptr)
        {
            return;
        }

        const float unitSize = renderOptions->GetGridUnitSize();
        AZ::Vector3  gridNormal   = AZ::Vector3(0.0f, 0.0f, 1.0f);

        if (mCamera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            // disable depth writing for ortho views
            renderUtil->SetDepthMaskWrite(false);

            switch (mCameraMode)
            {
            case CAMMODE_LEFT:
            case CAMMODE_RIGHT:
                gridNormal = MCore::GetForward(mCamera->GetViewMatrix());
                break;

            default:
                gridNormal = MCore::GetUp(mCamera->GetViewMatrix());
            }
            gridNormal.Normalize();
        }


        // render the grid
        AZ::Vector2 gridStart, gridEnd;
        renderUtil->CalcVisibleGridArea(mCamera, mWidth, mHeight, unitSize, &gridStart, &gridEnd);
        if (mViewWidget->GetRenderFlag(RenderViewWidget::RENDER_GRID))
        {
            renderUtil->RenderGrid(gridStart, gridEnd, gridNormal, unitSize, renderOptions->GetMainAxisColor(), renderOptions->GetGridColor(), renderOptions->GetSubStepColor(), true);
        }

        renderUtil->SetDepthMaskWrite(true);
    }


    void RenderWidget::closeEvent([[maybe_unused]] QCloseEvent* event)
    {
        if (mPlugin)
        {
            mPlugin->SaveRenderOptions();
        }
    }
} // namespace EMStudio
