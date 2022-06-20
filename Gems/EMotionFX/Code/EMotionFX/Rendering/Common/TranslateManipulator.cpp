/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslateManipulator.h"
#include <MCore/Source/PlaneEq.h>

namespace MCommon
{
    // constructor
    TranslateManipulator::TranslateManipulator(float scalingFactor, bool isVisible)
        : TransformationManipulator(scalingFactor, isVisible)
    {
        // set the initial values
        m_mode       = TRANSLATE_NONE;
        m_callback   = nullptr;
    }


    // destructor
    TranslateManipulator::~TranslateManipulator()
    {
    }


    // function to update the bounding volumes.
    void TranslateManipulator::UpdateBoundingVolumes(MCommon::Camera* camera)
    {
        MCORE_UNUSED(camera);

        // set the new proportions
        m_size               = m_scalingFactor;
        m_arrowLength        = m_size / 5.0f;
        m_baseRadius         = m_size / 20.0f;
        m_planeSelectorPos   = m_size / 2;

        // set the bounding volumes of the axes selection
        m_xAxisAabb.SetMax(m_position + AZ::Vector3(m_size + m_arrowLength, m_baseRadius, m_baseRadius));
        m_xAxisAabb.SetMin(m_position - AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));
        m_yAxisAabb.SetMax(m_position + AZ::Vector3(m_baseRadius, m_size + m_arrowLength, m_baseRadius));
        m_yAxisAabb.SetMin(m_position - AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));
        m_zAxisAabb.SetMax(m_position + AZ::Vector3(m_baseRadius, m_baseRadius, m_size + m_arrowLength));
        m_zAxisAabb.SetMin(m_position - AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));

        // set bounding volumes for the plane selectors
        m_xyPlaneAabb.SetMax(m_position + AZ::Vector3(m_planeSelectorPos, m_planeSelectorPos, m_baseRadius));
        m_xyPlaneAabb.SetMin(m_position + 0.3f * AZ::Vector3(m_planeSelectorPos, m_planeSelectorPos, 0) - AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));
        m_xzPlaneAabb.SetMax(m_position + AZ::Vector3(m_planeSelectorPos, m_baseRadius, m_planeSelectorPos));
        m_xzPlaneAabb.SetMin(m_position + 0.3f * AZ::Vector3(m_planeSelectorPos, 0, m_planeSelectorPos) - AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));
        m_yzPlaneAabb.SetMax(m_position + AZ::Vector3(m_baseRadius, m_planeSelectorPos, m_planeSelectorPos));
        m_yzPlaneAabb.SetMin(m_position + 0.3f * AZ::Vector3(0, m_planeSelectorPos, m_planeSelectorPos) - AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));
    }


    // update the visibility flags for the axes.
    void TranslateManipulator::UpdateAxisVisibility(MCommon::Camera* camera)
    {
        // return if camera does not exist
        if (camera == nullptr)
        {
            return;
        }

        // get the screen dimensions
        const uint32 screenWidth        = camera->GetScreenWidth();
        const uint32 screenHeight       = camera->GetScreenHeight();

        // calculate the camera roll ray
        MCore::Ray camRollRay = camera->Unproject(screenWidth / 2, screenHeight / 2);
        AZ::Vector3 camDir = camRollRay.GetDirection();

        // determine the axis visibility, to disable movement for invisible axes
        m_xAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        m_yAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        m_zAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
    }


    // check if the manipulator is hit by the mouse.
    bool TranslateManipulator::Hit(Camera* camera, int32 mousePosX, int32 mousePosY)
    {
        // check if camera exists
        if (camera == nullptr)
        {
            return false;
        }

        // update the bounding volumes
        UpdateBoundingVolumes();

        // shoot ray ar the mouse position
        MCore::Ray mouseRay = camera->Unproject(mousePosX, mousePosY);

        // check if one of the AABBs is hit
        if (mouseRay.Intersects(m_xAxisAabb) || mouseRay.Intersects(m_yAxisAabb) || mouseRay.Intersects(m_zAxisAabb) ||
            mouseRay.Intersects(m_xyPlaneAabb) || mouseRay.Intersects(m_xzPlaneAabb) || mouseRay.Intersects(m_yzPlaneAabb))
        {
            return true;
        }

        // return false if the manipulator is not hit
        return false;
    }


    // update the TranslateManipulator
    void TranslateManipulator::Render(MCommon::Camera* camera, RenderUtil* renderUtil)
    {
        // return if no render util is set
        if (renderUtil == nullptr || camera == nullptr || m_isVisible == false)
        {
            return;
        }

        // get the screen dimensions
        const uint32 screenWidth        = camera->GetScreenWidth();
        const uint32 screenHeight       = camera->GetScreenHeight();

        // update the axis visibility flags
        UpdateAxisVisibility(camera);

        // set color for the axes, depending on the selection
        MCore::RGBAColor xAxisColor     = (m_mode == TRANSLATE_X || m_mode == TRANSLATE_XY || m_mode == TRANSLATE_XZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_red;
        MCore::RGBAColor yAxisColor     = (m_mode == TRANSLATE_Y || m_mode == TRANSLATE_XY || m_mode == TRANSLATE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_green;
        MCore::RGBAColor zAxisColor     = (m_mode == TRANSLATE_Z || m_mode == TRANSLATE_XZ || m_mode == TRANSLATE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_blue;
        MCore::RGBAColor xyPlaneColorX  = (m_mode == TRANSLATE_XY) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_red;
        MCore::RGBAColor xyPlaneColorY  = (m_mode == TRANSLATE_XY) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_green;
        MCore::RGBAColor xzPlaneColorX  = (m_mode == TRANSLATE_XZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_red;
        MCore::RGBAColor xzPlaneColorZ  = (m_mode == TRANSLATE_XZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_blue;
        MCore::RGBAColor yzPlaneColorY  = (m_mode == TRANSLATE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_green;
        MCore::RGBAColor yzPlaneColorZ  = (m_mode == TRANSLATE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_blue;

        if (m_xAxisVisible)
        {
            // the x axis consisting of a line, cylinder and plane selectors
            renderUtil->RenderLine(m_position, m_position + AZ::Vector3(m_size, 0.0, 0.0), xAxisColor);
            renderUtil->RenderCylinder(m_baseRadius, 0, m_arrowLength, m_position + AZ::Vector3(m_size, 0, 0), AZ::Vector3(1, 0, 0), ManipulatorColors::s_red);
            renderUtil->RenderLine(m_position + AZ::Vector3(m_planeSelectorPos, 0.0, 0.0), m_position + AZ::Vector3(m_planeSelectorPos, m_planeSelectorPos, 0.0), xyPlaneColorX);
            renderUtil->RenderLine(m_position + AZ::Vector3(m_planeSelectorPos, 0.0, 0.0), m_position + AZ::Vector3(m_planeSelectorPos, 0.0, m_planeSelectorPos), xzPlaneColorX);

            // render the axis label for the x axis
            AZ::Vector3 textPosX = MCore::Project(m_position + AZ::Vector3(m_size + m_arrowLength + m_baseRadius, -m_baseRadius, 0.0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosX.GetX(), textPosX.GetY(), "X", xAxisColor);
        }

        if (m_yAxisVisible)
        {
            // the y axis consisting of a line, cylinder and plane selectors
            renderUtil->RenderLine(m_position, m_position + AZ::Vector3(0.0, m_size, 0.0), yAxisColor);
            renderUtil->RenderCylinder(m_baseRadius, 0, m_arrowLength, m_position + AZ::Vector3(0, m_size, 0), AZ::Vector3(0, 1, 0), ManipulatorColors::s_green);
            renderUtil->RenderLine(m_position + AZ::Vector3(0.0, m_planeSelectorPos, 0.0), m_position + AZ::Vector3(m_planeSelectorPos, m_planeSelectorPos, 0.0), xyPlaneColorY);
            renderUtil->RenderLine(m_position + AZ::Vector3(0.0, m_planeSelectorPos, 0.0), m_position + AZ::Vector3(0.0, m_planeSelectorPos, m_planeSelectorPos), yzPlaneColorY);

            // render the axis label for the y axis
            AZ::Vector3 textPosY = MCore::Project(m_position + AZ::Vector3(0.0, m_size + m_arrowLength + m_baseRadius, -m_baseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosY.GetX(), textPosY.GetY(), "Y", yAxisColor);
        }

        if (m_zAxisVisible)
        {
            // the z axis consisting of a line, cylinder and plane selectors
            renderUtil->RenderLine(m_position, m_position + AZ::Vector3(0.0, 0.0, m_size), zAxisColor);
            renderUtil->RenderCylinder(m_baseRadius, 0, m_arrowLength, m_position + AZ::Vector3(0, 0, m_size), AZ::Vector3(0, 0, 1), ManipulatorColors::s_blue);
            renderUtil->RenderLine(m_position + AZ::Vector3(0.0, 0.0, m_planeSelectorPos), m_position + AZ::Vector3(m_planeSelectorPos, 0.0, m_planeSelectorPos), xzPlaneColorZ);
            renderUtil->RenderLine(m_position + AZ::Vector3(0.0, 0.0, m_planeSelectorPos), m_position + AZ::Vector3(0.0, m_planeSelectorPos, m_planeSelectorPos), yzPlaneColorZ);

            // render the axis label for the z axis
            AZ::Vector3 textPosZ = MCore::Project(m_position + AZ::Vector3(0.0, m_baseRadius, m_size + m_arrowLength + m_baseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosZ.GetX(), textPosZ.GetY(), "Z", zAxisColor);
        }

        // draw transparent quad for the plane selectors
        if (m_mode == TRANSLATE_XY)
        {
            renderUtil->RenderTriangle(m_position, m_position + AZ::Vector3(m_planeSelectorPos, 0.0f, 0.0f), m_position + AZ::Vector3(m_planeSelectorPos, m_planeSelectorPos, 0.0f), ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(m_position, m_position + AZ::Vector3(m_planeSelectorPos, m_planeSelectorPos, 0.0f), m_position + AZ::Vector3(0.0f, m_planeSelectorPos, 0.0f), ManipulatorColors::s_selectionColorDarker);
        }
        else if (m_mode == TRANSLATE_XZ)
        {
            renderUtil->RenderTriangle(m_position, m_position + AZ::Vector3(m_planeSelectorPos, 0.0f, 0.0f), m_position + AZ::Vector3(m_planeSelectorPos, 0.0f, m_planeSelectorPos), ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(m_position, m_position + AZ::Vector3(m_planeSelectorPos, 0.0f, m_planeSelectorPos), m_position + AZ::Vector3(0.0f, 0.0f, m_planeSelectorPos), ManipulatorColors::s_selectionColorDarker);
        }
        else if (m_mode == TRANSLATE_YZ)
        {
            renderUtil->RenderTriangle(m_position + AZ::Vector3(0.0f, 0.0f, m_planeSelectorPos), m_position, m_position + AZ::Vector3(0.0f, m_planeSelectorPos, 0.0f), ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(m_position + AZ::Vector3(0.0f, m_planeSelectorPos, 0.0f), m_position + AZ::Vector3(0.0f, m_planeSelectorPos, m_planeSelectorPos), m_position + AZ::Vector3(0.0f, 0.0f, m_planeSelectorPos), ManipulatorColors::s_selectionColorDarker);
        }

        // render the relative position when moving
        if (m_callback)
        {
            // calculate the y-offset of the text position
            AZ::Vector3 deltaPos = GetPosition() - m_callback->GetOldValueVec();
            float yOffset       = (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_PERSPECTIVE) ? 60.0f * ((float)screenHeight / 720.0f) : 40.0f;

            // render the relative movement
            AZ::Vector3 textPos  = MCore::Project(m_position + (AZ::Vector3(m_size, m_size, m_size) / 3.0f), camera->GetViewProjMatrix(), camera->GetScreenWidth(), camera->GetScreenHeight());

            // render delta position of the gizmo of the name if not dragging at the moment
            if (m_selectionLocked && m_mode != TRANSLATE_NONE)
            {
                m_tempString = AZStd::string::format("X: %.3f, Y: %.3f, Z: %.3f", static_cast<float>(deltaPos.GetX()), static_cast<float>(deltaPos.GetY()), static_cast<float>(deltaPos.GetZ()));
                renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, m_tempString.c_str(), ManipulatorColors::s_selectionColor, 9.0f, true);
            }
            else
            {
                renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, m_name.c_str(), ManipulatorColors::s_selectionColor, 9.0f, true);
            }
        }

        // render the absolute position of the gizmo/actor instance
        if (m_mode != TRANSLATE_NONE)
        {
            const AZ::Vector3 offsetPos = GetPosition();
            m_tempString = AZStd::string::format("Abs Pos X: %.3f, Y: %.3f, Z: %.3f", static_cast<float>(offsetPos.GetX()), static_cast<float>(offsetPos.GetY()), static_cast<float>(offsetPos.GetZ()));
            renderUtil->RenderText(10, 10, m_tempString.c_str(), ManipulatorColors::s_selectionColor, 9.0f);
        }
    }


    // unproject screen coordinates to a ray
    void TranslateManipulator::ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);
        MCORE_UNUSED(middleButtonPressed);

        // check if camera has been set
        if (camera == nullptr || m_isVisible == false || (leftButtonPressed && rightButtonPressed))
        {
            return;
        }

        // only allow transformation when mouse is inside the widget
        mousePosX = MCore::Clamp<int32>(mousePosX, 0, camera->GetScreenWidth());
        mousePosY = MCore::Clamp<int32>(mousePosY, 0, camera->GetScreenHeight());

        // generate rays for the collision detection
        MCore::Ray mousePosRay      = camera->Unproject(mousePosX, mousePosY);
        MCore::Ray mousePrevPosRay  = camera->Unproject(mousePosX - mouseMovementX, mousePosY - mouseMovementY);

        // update the axis visibility flags
        UpdateAxisVisibility(camera);

        // check for the selected axis/plane
        if (m_selectionLocked == false || m_mode == TRANSLATE_NONE)
        {
            // update old values of the callback
            if (m_callback)
            {
                m_callback->UpdateOldValues();
            }

            // handle different translation modes
            if (mousePosRay.Intersects(m_xyPlaneAabb) && m_xAxisVisible && m_yAxisVisible)
            {
                m_movementDirection = AZ::Vector3(1.0f, 1.0f, 0.0f);
                m_movementPlaneNormal = AZ::Vector3(0.0f, 0.0f, 1.0f);
                m_mode = TRANSLATE_XY;
            }
            else if (mousePosRay.Intersects(m_xzPlaneAabb) && m_xAxisVisible && m_zAxisVisible)
            {
                m_movementDirection = AZ::Vector3(1.0f, 0.0f, 1.0f);
                m_movementPlaneNormal = AZ::Vector3(0.0f, 1.0f, 0.0f);
                m_mode = TRANSLATE_XZ;
            }
            else if (mousePosRay.Intersects(m_yzPlaneAabb) && m_yAxisVisible && m_zAxisVisible)
            {
                m_movementDirection = AZ::Vector3(0.0f, 1.0f, 1.0f);
                m_movementPlaneNormal = AZ::Vector3(1.0f, 0.0f, 0.0f);
                m_mode = TRANSLATE_YZ;
            }
            else if (mousePosRay.Intersects(m_xAxisAabb) && m_xAxisVisible)
            {
                m_movementDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
                m_movementPlaneNormal = AZ::Vector3(0.0f, 1.0f, 1.0f).GetNormalized();
                m_mode = TRANSLATE_X;
            }
            else if (mousePosRay.Intersects(m_yAxisAabb) && m_yAxisVisible)
            {
                m_movementDirection = AZ::Vector3(0.0f, 1.0f, 0.0f);
                m_movementPlaneNormal = AZ::Vector3(1.0f, 0.0f, 1.0f).GetNormalized();
                m_mode = TRANSLATE_Y;
            }
            else if (mousePosRay.Intersects(m_zAxisAabb) && m_zAxisVisible)
            {
                m_movementDirection = AZ::Vector3(0.0f, 0.0f, 1.0f);
                m_movementPlaneNormal = AZ::Vector3(1.0f, 1.0f, 0.0f).GetNormalized();
                m_mode = TRANSLATE_Z;
            }
            else
            {
                m_mode = TRANSLATE_NONE;
            }
        }

        // set selection lock
        m_selectionLocked = leftButtonPressed;

        // move the gizmo
        if (m_selectionLocked == false || m_mode == TRANSLATE_NONE)
        {
            m_mousePosRelative = AZ::Vector3::CreateZero();
            return;
        }

        // init the movement change to zero
        AZ::Vector3 movement = AZ::Vector3::CreateZero();

        // handle plane movement
        if (m_mode == TRANSLATE_XY || m_mode == TRANSLATE_XZ || m_mode == TRANSLATE_YZ)
        {
            // generate current translation plane and calculate mouse intersections
            MCore::PlaneEq movementPlane(m_movementPlaneNormal, m_position);
            AZ::Vector3 mousePosIntersect, mousePrevPosIntersect;
            mousePosRay.Intersects(movementPlane, &mousePosIntersect);
            mousePrevPosRay.Intersects(movementPlane, &mousePrevPosIntersect);

            // calculate the mouse position relative to the gizmo
            if (MCore::Math::IsFloatZero(MCore::SafeLength(m_mousePosRelative)))
            {
                m_mousePosRelative = mousePosIntersect - m_position;
            }

            // distance of the mouse intersections is the actual movement on the plane
            movement = mousePosIntersect - m_mousePosRelative;
        }

        // handle axis movement
        // TODO: Fix the infinity bug for axis movement!!!!!
        else
        {
            // calculate the movement of the mouse on a plane located at the gizmo position
            // and perpendicular to the move direction
            AZ::Vector3 camDir = camera->Unproject(camera->GetScreenWidth() / 2, camera->GetScreenHeight() / 2).GetDirection();
            AZ::Vector3 thirdAxis = m_movementDirection.Cross(camDir).GetNormalized();
            m_movementPlaneNormal = thirdAxis.Cross(m_movementDirection).GetNormalized();
            thirdAxis = m_movementPlaneNormal.Cross(m_movementDirection).GetNormalized();

            MCore::PlaneEq movementPlane(m_movementPlaneNormal, m_position);
            MCore::PlaneEq movementPlane2(thirdAxis, m_position);

            // calculate the intersection points of the mouse positions with the previously calculated plane
            AZ::Vector3 mousePosIntersect, mousePosIntersect2;
            mousePosRay.Intersects(movementPlane, &mousePosIntersect);
            mousePosRay.Intersects(movementPlane2, &mousePosIntersect2);

            if (mousePosIntersect.GetLength() < camera->GetFarClipDistance())
            {
                if (MCore::Math::IsFloatZero(MCore::SafeLength(m_mousePosRelative)))
                {
                    m_mousePosRelative = movementPlane2.Project(mousePosIntersect) - m_position;
                }

                mousePosIntersect = movementPlane2.Project(mousePosIntersect);
            }
            else
            {
                if (MCore::Math::IsFloatZero(MCore::SafeLength(m_mousePosRelative)))
                {
                    m_mousePosRelative = movementPlane.Project(mousePosIntersect2) - m_position;
                }

                mousePosIntersect = movementPlane.Project(mousePosIntersect2);
            }

            // adjust the movement vector
            movement = mousePosIntersect - m_mousePosRelative;
        }

        // update the position of the gizmo
        movement = movement - m_position;
        movement = AZ::Vector3(movement.GetX() * m_movementDirection.GetX(), movement.GetY() * m_movementDirection.GetY(), movement.GetZ() * m_movementDirection.GetZ());
        m_position += movement;

        // update the callback
        if (m_callback)
        {
            // reset the callback position, if the position is too far away from the camera
            float farClip = camera->GetFarClipDistance();
            if (m_position.GetLength() >= farClip)
            {
                m_position = m_callback->GetOldValueVec() + m_renderOffset;
            }

            // update the callback
            m_callback->Update(GetPosition());
        }
    }
} // namespace MCommon
