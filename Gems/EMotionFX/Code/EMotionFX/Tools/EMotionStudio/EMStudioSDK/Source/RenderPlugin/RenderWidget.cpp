/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

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
#include <MCore/Source/AABB.h>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(RenderWidget::EventHandler, EMotionFX::EventHandlerAllocator, 0)


    // constructor
    RenderWidget::RenderWidget(RenderPlugin* renderPlugin, RenderViewWidget* viewWidget)
        : m_eventHandler(this)
    {
        // create our event handler
        EMotionFX::GetEventManager().AddEventHandler(&m_eventHandler);

        // camera used to render the little axis on the bottom left
        m_axisFakeCamera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_FRONT);

        m_plugin                 = renderPlugin;
        m_viewWidget             = viewWidget;
        m_width                  = 0;
        m_height                 = 0;
        m_viewCloseupWaiting     = 0;
        m_prevMouseX             = 0;
        m_prevMouseY             = 0;
        m_prevLocalMouseX        = 0;
        m_prevLocalMouseY        = 0;
        m_oldActorInstancePos    = AZ::Vector3::CreateZero();
        m_camera                 = nullptr;
        m_activeTransformManip   = nullptr;
        m_skipFollowCalcs        = false;
        m_needDisableFollowMode  = true;
    }


    // destructor
    RenderWidget::~RenderWidget()
    {
        // get rid of the event handler
        EMotionFX::GetEventManager().RemoveEventHandler(&m_eventHandler);

        // get rid of the camera objects
        delete m_camera;
        delete m_axisFakeCamera;
    }

    // start view closeup flight
    void RenderWidget::ViewCloseup(const AZ::Aabb& aabb, float flightTime, uint32 viewCloseupWaiting)
    {
        m_viewCloseupWaiting     = viewCloseupWaiting;
        m_viewCloseupAabb        = aabb;
        m_viewCloseupFlightTime  = flightTime;
    }

    void RenderWidget::ViewCloseup(bool selectedInstancesOnly, float flightTime, uint32 viewCloseupWaiting)
    {
        m_viewCloseupWaiting     = viewCloseupWaiting;
        m_viewCloseupAabb        = m_plugin->GetSceneAabb(selectedInstancesOnly);
        m_viewCloseupFlightTime  = flightTime;
    }


    // switch the active camera
    void RenderWidget::SwitchCamera(CameraMode mode)
    {
        delete m_camera;
        m_cameraMode = mode;

        switch (mode)
        {
        case CAMMODE_ORBIT:
        {
            m_camera = new MCommon::OrbitCamera();
            break;
        }
        case CAMMODE_FIRSTPERSON:
        {
            m_camera = new MCommon::FirstPersonCamera();
            break;
        }
        case CAMMODE_FRONT:
        {
            m_camera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_FRONT);
            break;
        }
        case CAMMODE_BACK:
        {
            m_camera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_BACK);
            break;
        }
        case CAMMODE_LEFT:
        {
            m_camera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_LEFT);
            break;
        }
        case CAMMODE_RIGHT:
        {
            m_camera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_RIGHT);
            break;
        }
        case CAMMODE_TOP:
        {
            m_camera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_TOP);
            break;
        }
        case CAMMODE_BOTTOM:
        {
            m_camera = new MCommon::OrthographicCamera(MCommon::OrthographicCamera::VIEWMODE_BOTTOM);
            break;
        }
        }

        // show the entire scene
        m_plugin->ViewCloseup(false, this, 0.0f);
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
        if (m_camera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC)
        {
            camDist = 0.75f;
            switch (GetCameraMode())
            {
            case CAMMODE_FRONT:
            case CAMMODE_BOTTOM:
                // -(scale.x)
                camDist *= -2.0f / static_cast<float>(m_camera->GetViewProjMatrix().GetElement(0, 0));
                break;
            case CAMMODE_BACK:
            case CAMMODE_TOP:
                // scale.x
                camDist *= 2.0f / static_cast<float>(m_camera->GetViewProjMatrix().GetElement(0, 0));
                break;
            case CAMMODE_LEFT:
                // -(scale.y)
                camDist *= -2.0f / static_cast<float>(m_camera->GetViewProjMatrix().GetElement(0, 1));
                break;
            case CAMMODE_RIGHT:
                // scale.y
                camDist *= 2.0f / static_cast<float>(m_camera->GetViewProjMatrix().GetElement(0, 1));
                break;
            default:
                break;
            }
        }
        // calculate cam distance for perspective cam
        else
        {
            if (activeManipulator->GetSelectionLocked() &&
                m_viewWidget->GetIsCharacterFollowModeActive() == false &&
                activeManipulator->GetType() == MCommon::TransformationManipulator::GIZMOTYPE_TRANSLATION)
            {
                camDist = (callback->GetOldValueVec() - m_camera->GetPosition()).GetLength();
            }
            else
            {
                camDist = (activeManipulator->GetPosition() - m_camera->GetPosition()).GetLength();
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
            activeManipulator->SetScale(aznumeric_cast<float>(camDist * 0.15), m_camera);
        }

        // update position of the actor instance (needed for camera follow mode)
        EMotionFX::ActorInstance* actorInstance = callback->GetActorInstance();
        if (actorInstance)
        {
            activeManipulator->Init(actorInstance->GetLocalSpaceTransform().m_position);
        }
    }


    // handle mouse move event
    void RenderWidget::OnMouseMoveEvent(QWidget* renderWidget, QMouseEvent* event)
    {
        // calculate the delta mouse movement
        int32 deltaX = event->globalX() - m_prevMouseX;
        int32 deltaY = event->globalY() - m_prevMouseY;

        // store the current value as previous value
        m_prevMouseX = event->globalX();
        m_prevMouseY = event->globalY();
        m_prevLocalMouseX = event->x();
        m_prevLocalMouseY = event->y();

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
            m_pixelsMovedSinceRightClick += (int32)MCore::Math::Abs(aznumeric_cast<float>(deltaX)) + (int32)MCore::Math::Abs(aznumeric_cast<float>(deltaY));
        }

        // update size/bounding volumes volumes of all existing gizmos
        const AZStd::vector<MCommon::TransformationManipulator*>* transformationManipulators = GetManager()->GetTransformationManipulators();

        // render all visible gizmos
        for (MCommon::TransformationManipulator* activeManipulator : *transformationManipulators)
        {
            if (activeManipulator == nullptr)
            {
                continue;
            }

            // get camera position for size adjustments of the gizmos
            UpdateActiveTransformationManipulator(activeManipulator);
        }

        // get the translate manipulator
        MCommon::TransformationManipulator* mouseOveredManip = m_plugin->GetActiveManipulator(m_camera, event->x(), event->y());

        // check if the current manipulator is hit
        if (mouseOveredManip)
        {
            gizmoHit = mouseOveredManip->Hit(m_camera, event->x(), event->y());
        }
        else
        {
            mouseOveredManip = m_activeTransformManip;
        }

        // flag to check if mouse wrapping occured
        bool mouseWrapped = false;

        // map the cursor if it goes out of the screen
        //if (activeManipulator != (MCommon::TransformationManipulator*)translateManipulator || (translateManipulator && translateManipulator->GetMode() == MCommon::TranslateManipulator::TRANSLATE_NONE))
        if (mouseOveredManip == nullptr || (mouseOveredManip && mouseOveredManip->GetType() != MCommon::TransformationManipulator::GIZMOTYPE_TRANSLATION))
        {
            const int32 width = m_camera->GetScreenWidth();
            const int32 height = m_camera->GetScreenHeight();

            // handle mouse wrapping, to enable smoother panning
            if (event->x() > (int32)width)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() - width, event->globalY()));
                m_prevMouseX = event->globalX() - width;
            }
            else if (event->x() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX() + width, event->globalY()));
                m_prevMouseX = event->globalX() + width;
            }

            if (event->y() > (int32)height)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() - height));
                m_prevMouseY = event->globalY() - height;
            }
            else if (event->y() < 0)
            {
                mouseWrapped = true;
                QCursor::setPos(QPoint(event->globalX(), event->globalY() + height));
                m_prevMouseY = event->globalY() + height;
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
                if (m_needDisableFollowMode)
                {
                    MCommon::ManipulatorCallback* callback = mouseOveredManip->GetCallback();
                    if (callback)
                    {
                        if (callback->GetResetFollowMode())
                        {
                            m_isCharacterFollowModeActive = m_viewWidget->GetIsCharacterFollowModeActive();
                            m_viewWidget->SetCharacterFollowModeActive(false);
                            m_needDisableFollowMode = false;
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
            mouseOveredManip->ProcessMouseInput(m_camera, event->x(), event->y(), deltaX, deltaY, leftButtonPressed && !altPressed, middleButtonPressed, rightButtonPressed);
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
            if (m_camera)
            {
                switch (m_cameraMode)
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
                            renderWidget->setCursor(m_plugin->GetZoomOutCursor());
                        }
                        else
                        {
                            renderWidget->setCursor(m_plugin->GetZoomInCursor());
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
                            renderWidget->setCursor(m_plugin->GetZoomOutCursor());
                        }
                        else
                        {
                            renderWidget->setCursor(m_plugin->GetZoomInCursor());
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

                m_camera->ProcessMouseInput(deltaX, deltaY, leftButtonPressed, middleButtonPressed, rightButtonPressed);
                m_camera->Update();
            }
        }

        //LogInfo( "mouseMoveEvent: x=%i, y=%i, deltaX=%i, deltaY=%i, globalX=%i, globalY=%i, wrapped=%i", event->x(), event->y(), deltaX, deltaY, event->globalX(), event->globalY(), mouseWrapped );
    }


    // handle mouse button press event
    void RenderWidget::OnMousePressEvent(QWidget* renderWidget, QMouseEvent* event)
    {
        // reset the number of pixels moved since the last right click
        m_pixelsMovedSinceRightClick = 0;

        // calculate the delta mouse movement and set old mouse position
        m_prevMouseX = event->globalX();
        m_prevMouseY = event->globalY();

        // get the button states
        const bool leftButtonPressed    = event->buttons() & Qt::LeftButton;
        const bool middleButtonPressed  = event->buttons() & Qt::MidButton;
        const bool rightButtonPressed   = event->buttons() & Qt::RightButton;
        const bool ctrlPressed          = event->modifiers() & Qt::ControlModifier;
        const bool altPressed           = event->modifiers() & Qt::AltModifier;

        // set the click position if right click was done
        if (rightButtonPressed)
        {
            m_rightClickPosX = QCursor::pos().x();
            m_rightClickPosY = QCursor::pos().y();
        }

        // get the current selection
        const CommandSystem::SelectionList& selection = GetCommandManager()->GetCurrentSelection();

        // check if the active gizmo is hit by the mouse (left click)
        bool gizmoHit = false;
        MCommon::TransformationManipulator* activeManipulator = nullptr;
        if (leftButtonPressed && middleButtonPressed == false && rightButtonPressed == false)
        {
            activeManipulator = m_plugin->GetActiveManipulator(m_camera, event->x(), event->y());
        }

        if (activeManipulator)
        {
            gizmoHit = (activeManipulator->GetMode() != 0);

            MCommon::ManipulatorCallback* callback = activeManipulator->GetCallback();
            if (callback)
            {
                if (gizmoHit && callback->GetResetFollowMode())
                {
                    m_isCharacterFollowModeActive = m_viewWidget->GetIsCharacterFollowModeActive();
                    m_viewWidget->SetCharacterFollowModeActive(false);
                    m_needDisableFollowMode = false;

                    m_activeTransformManip = activeManipulator;
                    m_activeTransformManip->ProcessMouseInput(m_camera, event->x(), event->y(), 0, 0, leftButtonPressed && !altPressed, middleButtonPressed, rightButtonPressed);
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
            size_t editorActorInstanceCount = 0;
            const EMotionFX::ActorManager& actorManager = EMotionFX::GetActorManager();
            const size_t totalActorInstanceCount = actorManager.GetNumActorInstances();
            for (size_t i = 0; i < totalActorInstanceCount; ++i)
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

                    const MCore::Ray ray = m_camera->Unproject(mousePosX, mousePosY);

                    // get the number of actor instances and iterate through them
                    const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
                    for (size_t i = 0; i < numActorInstances; ++i)
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
                                const float distOld = (m_camera->GetPosition() - oldIntersectionPoint).GetLength();
                                const float distNew = (m_camera->GetPosition() - intersect).GetLength();
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
                                AZ::Aabb box;
                                actorInstance->CalcNodeBasedAabb(&box);

                                // render the aabb
                                if (box.IsValid())
                                {
                                    const MCore::AABB mcoreAabb(box.GetMin(), box.GetMax());
                                    AZ::Vector3 ii, n;
                                    if (ray.Intersects(mcoreAabb, &ii, &n))
                                    {
                                        selectedActorInstance = actorInstance;
                                        oldIntersectionPoint = ii;
                                    }
                                }
                            }
                        }
                    }

                    m_selectedActorInstances.clear();

                    if (ctrlPressed)
                    {
                        // add the old selection to the selected actor instances (selection mode = add)
                        const size_t numSelectedActorInstances = selection.GetNumSelectedActorInstances();
                        for (size_t i = 0; i < numSelectedActorInstances; ++i)
                        {
                            m_selectedActorInstances.emplace_back(selection.GetActorInstance(i));
                        }
                    }

                    if (selectedActorInstance)
                    {
                        m_selectedActorInstances.emplace_back(selectedActorInstance);
                    }

                    CommandSystem::SelectActorInstancesUsingCommands(m_selectedActorInstances);
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
            MCommon::TransformationManipulator* mouseOveredManip = m_plugin->GetActiveManipulator(m_camera, event->x(), event->y());
            if (mouseOveredManip == nullptr)
            {
                mouseOveredManip = m_activeTransformManip;
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
                mouseOveredManip->ProcessMouseInput(m_camera, 0, 0, 0, 0, false, false, false);

                // reset the camera follow mode state
                if (callback && callback->GetResetFollowMode() && m_isCharacterFollowModeActive)
                {
                    m_viewWidget->SetCharacterFollowModeActive(m_isCharacterFollowModeActive);
                    m_skipFollowCalcs = true;
                }
            }
        }

        // reset the active manipulator
        m_activeTransformManip = nullptr;

        // reset the disable follow flag
        m_needDisableFollowMode = true;

        // set the arrow cursor
        renderWidget->setCursor(Qt::ArrowCursor);

        // context menu handling
        if (m_pixelsMovedSinceRightClick < 5)
        {
            OnContextMenuEvent(renderWidget, event->modifiers() & Qt::ControlModifier, event->modifiers() & Qt::AltModifier, event->x(), event->y(), event->globalPos());
        }
    }


    // handle mouse wheel event
    void RenderWidget::OnWheelEvent(QWidget* renderWidget, QWheelEvent* event)
    {
        MCORE_UNUSED(renderWidget);

        m_camera->ProcessMouseInput(0,
            event->angleDelta().y(),
            false,
            false,
            true
        );

        m_camera->Update();
    }


    // handles context menu events
    void RenderWidget::OnContextMenuEvent(QWidget* renderWidget, bool shiftPressed, bool altPressed, int32 localMouseX, int32 localMouseY, QPoint globalMousePos)
    {
        // stop context menu execution, if mouse position changed or alt is pressed
        // so block it if zooming, moving etc. is enabled
        if (QCursor::pos().x() != m_rightClickPosX || QCursor::pos().y() != m_rightClickPosY || altPressed)
        {
            return;
        }

        // call the context menu handler
        m_viewWidget->OnContextMenuEvent(renderWidget, shiftPressed, localMouseX, localMouseY, globalMousePos, m_plugin, m_camera);
    }


    void RenderWidget::RenderAxis()
    {
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // set the camera used to render the axis
        MCommon::Camera* camera = m_camera;
        if (m_camera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            camera = m_axisFakeCamera;
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
        if (m_camera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            MCommon::OrthographicCamera* orgCamera = (MCommon::OrthographicCamera*)m_camera;
            MCommon::OrthographicCamera* orthoCamera = (MCommon::OrthographicCamera*)camera;
            orthoCamera->SetCurrentDistance(1.0f);
            orthoCamera->SetPosition(orgCamera->GetPosition());
            orthoCamera->SetMode(orgCamera->GetMode());
            orthoCamera->SetScreenDimensions(m_width, m_height);
            size *= 0.001f;
        }

        // update the camera
        camera->SetOrthoClipDimensions(AZ::Vector2(aznumeric_cast<float>(m_width), aznumeric_cast<float>(m_height)));
        camera->Update();

        MCommon::RenderUtil::AxisRenderingSettings axisRenderingSettings;
        int32 originScreenX = 0;
        int32 originScreenY = 0;
        switch (m_cameraMode)
        {
        case CAMMODE_ORBIT:
        {
            originScreenX = distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = true;
            axisRenderingSettings.m_renderYAxis = true;
            axisRenderingSettings.m_renderZAxis = true;
            break;
        }
        case CAMMODE_FIRSTPERSON:
        {
            originScreenX = distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = true;
            axisRenderingSettings.m_renderYAxis = true;
            axisRenderingSettings.m_renderZAxis = true;
            break;
        }
        case CAMMODE_FRONT:
        {
            originScreenX = distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = true;
            axisRenderingSettings.m_renderYAxis = true;
            axisRenderingSettings.m_renderZAxis = false;
            break;
        }
        case CAMMODE_BACK:
        {
            originScreenX = 2 * distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = true;
            axisRenderingSettings.m_renderYAxis = true;
            axisRenderingSettings.m_renderZAxis = false;
            break;
        }
        case CAMMODE_LEFT:
        {
            originScreenX = distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = false;
            axisRenderingSettings.m_renderYAxis = true;
            axisRenderingSettings.m_renderZAxis = true;
            break;
        }
        case CAMMODE_RIGHT:
        {
            originScreenX = 2 * distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = false;
            axisRenderingSettings.m_renderYAxis = true;
            axisRenderingSettings.m_renderZAxis = true;
            break;
        }
        case CAMMODE_TOP:
        {
            originScreenX = distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = true;
            axisRenderingSettings.m_renderYAxis = false;
            axisRenderingSettings.m_renderZAxis = true;
            break;
        }
        case CAMMODE_BOTTOM:
        {
            originScreenX = 2 * distanceFromBorder;
            originScreenY = m_height - distanceFromBorder;
            axisRenderingSettings.m_renderXAxis = true;
            axisRenderingSettings.m_renderYAxis = false;
            axisRenderingSettings.m_renderZAxis = true;
            break;
        }
        default:
            MCORE_ASSERT(false);
        }

        const AZ::Vector3 axisPosition = MCore::UnprojectOrtho(aznumeric_cast<float>(originScreenX), aznumeric_cast<float>(originScreenY), aznumeric_cast<float>(m_width), aznumeric_cast<float>(m_height), 0.0f, camera->GetProjectionMatrix(), camera->GetViewMatrix());

        AZ::Matrix4x4 inverseCameraMatrix = camera->GetViewMatrix();
        inverseCameraMatrix.InvertFull();

        AZ::Transform worldTM = AZ::Transform::CreateIdentity();
        worldTM.SetTranslation(axisPosition);

        axisRenderingSettings.m_size             = size;
        axisRenderingSettings.m_worldTm          = worldTM;
        axisRenderingSettings.m_cameraRight      = MCore::GetRight(inverseCameraMatrix).GetNormalized();
        axisRenderingSettings.m_cameraUp         = MCore::GetUp(inverseCameraMatrix).GetNormalized();
        axisRenderingSettings.m_renderXAxisName  = true;
        axisRenderingSettings.m_renderYAxisName  = true;
        axisRenderingSettings.m_renderZAxisName  = true;

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
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // render the camera mode name at the bottom of the gl widget
        const char*         text                = m_camera->GetTypeString();
        const uint32        textSize            = 10;
        const uint32        cameraNameColor     = MCore::RGBAColor(1.0f, 1.0f, 1.0f, 1.0f).ToInt();
        const uint32        cameraNameX         = aznumeric_cast<uint32>(m_width * 0.5f);
        const uint32        cameraNameY         = m_height - 20;

        renderUtil->RenderText(aznumeric_cast<float>(cameraNameX), aznumeric_cast<float>(cameraNameY), text, cameraNameColor, textSize, true);
        //glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        //renderText(screenX, screenY, text);

        // make sure the lines are rendered directly afterwards
        renderUtil->Render2DLines();
    }


    void RenderWidget::UpdateCharacterFollowModeData()
    {
        if (m_viewWidget->GetIsCharacterFollowModeActive())
        {
            const CommandSystem::SelectionList& selectionList = GetCommandManager()->GetCurrentSelection();
            EMotionFX::ActorInstance* followInstance = selectionList.GetFirstActorInstance();

            if (followInstance && m_camera)
            {
                const AZ::Vector3& localPos = followInstance->GetLocalSpaceTransform().m_position;
                m_plugin->GetTranslateManipulator()->Init(localPos);
                m_plugin->GetRotateManipulator()->Init(localPos);
                m_plugin->GetScaleManipulator()->Init(localPos);

                AZ::Vector3 actorInstancePos;

                EMotionFX::Actor* followActor = followInstance->GetActor();
                const size_t motionExtractionNodeIndex = followActor->GetMotionExtractionNodeIndex();
                if (motionExtractionNodeIndex != InvalidIndex)
                {
                    actorInstancePos = followInstance->GetWorldSpaceTransform().m_position;
                    RenderPlugin::EMStudioRenderActor* emstudioActor = m_plugin->FindEMStudioActor(followActor);
                    if (emstudioActor)
                    {
                        #ifndef EMFX_SCALE_DISABLED
                            const float scaledOffsetFromTrajectoryNode = followInstance->GetWorldSpaceTransform().m_scale.GetZ() * emstudioActor->m_offsetFromTrajectoryNode;
                        #else
                            const float scaledOffsetFromTrajectoryNode = 1.0f;
                        #endif
                        actorInstancePos.SetZ(actorInstancePos.GetZ() + scaledOffsetFromTrajectoryNode);
                    }
                }
                else
                {
                    actorInstancePos = followInstance->GetWorldSpaceTransform().m_position;
                }

                // Calculate movement since last frame.
                AZ::Vector3 deltaPos = actorInstancePos - m_oldActorInstancePos;

                if (m_skipFollowCalcs)
                {
                    deltaPos = AZ::Vector3::CreateZero();
                    m_skipFollowCalcs = false;
                }

                m_oldActorInstancePos = actorInstancePos;

                switch (m_camera->GetType())
                {
                case MCommon::OrbitCamera::TYPE_ID:
                {
                    MCommon::OrbitCamera* orbitCamera = static_cast<MCommon::OrbitCamera*>(m_camera);

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
                    MCommon::OrthographicCamera* orthoCamera = static_cast<MCommon::OrthographicCamera*>(m_camera);

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
            m_oldActorInstancePos.Set(0.0f, 0.0f, 0.0f);
        }
    }


    // render the manipulator gizmos
    void RenderWidget::RenderManipulators()
    {
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        AZStd::vector<MCommon::TransformationManipulator*>* transformationManipulators = GetManager()->GetTransformationManipulators();

        // render all visible gizmos
        for (MCommon::TransformationManipulator* activeManipulator : *transformationManipulators)
        {
            // update the gizmos if there is an active manipulator
            if (activeManipulator == nullptr)
            {
                continue;
            }

            // update the active manipulator depending on camera distance and mode
            UpdateActiveTransformationManipulator(activeManipulator);

            // render the current actor
            activeManipulator->Render(m_camera, renderUtil);
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
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // render custom triangles
        for (const Triangle& curTri : m_triangles)
        {
             renderUtil->AddTriangle(curTri.m_posA, curTri.m_posB, curTri.m_posC, curTri.m_normalA, curTri.m_normalB, curTri.m_normalC, curTri.m_color); // TODO: make renderutil use uint32 colors instead
        }

        ClearTriangles();
        renderUtil->RenderTriangles();
    }


    // iterate through all plugins and render their helper data
    void RenderWidget::RenderCustomPluginData()
    {
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        EMStudioPlugin::RenderInfo renderInfo(renderUtil, m_camera, m_width, m_height);

        const PluginManager::PluginVector& activePlugins = GetPluginManager()->GetActivePlugins();
        for (EMStudioPlugin* plugin : activePlugins)
        {
            plugin->LegacyRender(m_plugin, &renderInfo);
        }

        RenderDebugDraw();

        // render all triangles
        RenderTriangles();
    }


    void RenderWidget::RenderDebugDraw()
    {
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
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
        MCommon::RenderUtil* renderUtil = m_plugin->GetRenderUtil();
        if (renderUtil == nullptr)
        {
            return;
        }

        // backface culling
        const bool backfaceCullingEnabled = m_viewWidget->GetRenderFlag(RenderViewWidget::RENDER_BACKFACECULLING);
        renderUtil->EnableCulling(backfaceCullingEnabled);

        EMotionFX::GetAnimGraphManager().SetAnimGraphVisualizationEnabled(true);

        // Only keep the following line when we do not link to the update system component OnTick anymore.
        /////        EMotionFX::GetEMotionFX().Update(0.0f);

        // render
        const size_t numActorInstances = EMotionFX::GetActorManager().GetNumActorInstances();
        for (size_t i = 0; i < numActorInstances; ++i)
        {
            EMotionFX::ActorInstance* actorInstance = EMotionFX::GetActorManager().GetActorInstance(i);
            if (actorInstance->GetRender() && actorInstance->GetIsVisible() && actorInstance->GetIsOwnedByRuntime() == false)
            {
                m_plugin->RenderActorInstance(actorInstance, 0.0f);
            }
        }
    }


    // prepare the camera
    void RenderWidget::UpdateCamera()
    {
        if (m_camera == nullptr)
        {
            return;
        }

        RenderOptions* renderOptions = m_plugin->GetRenderOptions();

        // update the camera
        m_camera->SetNearClipDistance(renderOptions->GetNearClipPlaneDistance());
        m_camera->SetFarClipDistance(renderOptions->GetFarClipPlaneDistance());
        m_camera->SetFOV(renderOptions->GetFOV());
        m_camera->SetAspectRatio(m_width / (float)m_height);
        m_camera->SetScreenDimensions(m_width, m_height);
        m_camera->AutoUpdateLimits();

        if (m_viewCloseupWaiting != 0 && m_height != 0 && m_width != 0)
        {
            m_viewCloseupWaiting--;
            if (m_viewCloseupWaiting == 0)
            {
                m_camera->ViewCloseup(MCore::AABB(m_viewCloseupAabb.GetMin(), m_viewCloseupAabb.GetMax()), m_viewCloseupFlightTime);
            }
        }

        // update the manipulators, camera, old actor instance position etc. when using the character follow mode
        UpdateCharacterFollowModeData();

        m_camera->Update();
    }


    // render grid in lines and checkerboard
    void RenderWidget::RenderGrid()
    {
        // directly return in case we do not want to render any type of grid
        if (m_viewWidget->GetRenderFlag(RenderViewWidget::RENDER_GRID) == false)
        {
            return;
        }

        // get access to the render utility and render options
        MCommon::RenderUtil*    renderUtil      = m_plugin->GetRenderUtil();
        RenderOptions*          renderOptions   = m_plugin->GetRenderOptions();
        if (renderUtil == nullptr || renderOptions == nullptr)
        {
            return;
        }

        const float unitSize = renderOptions->GetGridUnitSize();
        AZ::Vector3  gridNormal   = AZ::Vector3(0.0f, 0.0f, 1.0f);

        if (m_camera->GetType() == MCommon::OrthographicCamera::TYPE_ID)
        {
            // disable depth writing for ortho views
            renderUtil->SetDepthMaskWrite(false);

            switch (m_cameraMode)
            {
            case CAMMODE_LEFT:
            case CAMMODE_RIGHT:
                gridNormal = MCore::GetForward(m_camera->GetViewMatrix());
                break;

            default:
                gridNormal = MCore::GetUp(m_camera->GetViewMatrix());
            }
            gridNormal.Normalize();
        }


        // render the grid
        AZ::Vector2 gridStart, gridEnd;
        renderUtil->CalcVisibleGridArea(m_camera, m_width, m_height, unitSize, &gridStart, &gridEnd);
        if (m_viewWidget->GetRenderFlag(RenderViewWidget::RENDER_GRID))
        {
            renderUtil->RenderGrid(gridStart, gridEnd, gridNormal, unitSize, renderOptions->GetMainAxisColor(), renderOptions->GetGridColor(), renderOptions->GetSubStepColor(), true);
        }

        renderUtil->SetDepthMaskWrite(true);
    }


    void RenderWidget::closeEvent([[maybe_unused]] QCloseEvent* event)
    {
        if (m_plugin)
        {
            m_plugin->SaveRenderOptions();
        }
    }
} // namespace EMStudio
