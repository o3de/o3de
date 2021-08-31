/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScaleManipulator.h"
#include <MCore/Source/PlaneEq.h>

namespace MCommon
{
    // constructor
    ScaleManipulator::ScaleManipulator(float scalingFactor, bool isVisible)
        : TransformationManipulator(scalingFactor, isVisible)
    {
        // set the initial values
        m_mode               = SCALE_NONE;
        m_selectionLocked    = false;
        m_callback           = nullptr;
        m_position           = AZ::Vector3::CreateZero();
        m_scaleDirection     = AZ::Vector3::CreateZero();
        m_scale              = AZ::Vector3::CreateZero();
    }


    // destructor
    ScaleManipulator::~ScaleManipulator()
    {
    }


    // update the bounding volumes
    void ScaleManipulator::UpdateBoundingVolumes(MCommon::Camera* camera)
    {
        // update the axes before updating the bounding volumes
        if (camera)
        {
            UpdateAxisDirections(camera);
        }

        m_size           = m_scalingFactor;
        m_scaledSize     = AZ::Vector3(m_size, m_size, m_size) + AZ::Vector3(MCore::Max(float(m_scale.GetX()), -m_size), MCore::Max(float(m_scale.GetY()), -m_size), MCore::Max(float(m_scale.GetZ()), -m_size));
        m_diagScale      = 0.5f;
        m_arrowLength    = m_size / 10.0f;
        m_baseRadius     = m_size / 15.0f;

        // positions for the plane selectors
        m_firstPlaneSelectorPos  = m_scaledSize * 0.3f;
        m_secPlaneSelectorPos    = m_scaledSize * 0.6f;

        // set the bounding volumes of the axes selection
        m_xAxisAabb.SetMax(m_position + m_signX * AZ::Vector3(m_scaledSize.GetX() + m_arrowLength, m_baseRadius, m_baseRadius));
        m_xAxisAabb.SetMin(m_position - m_signX * AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));
        m_yAxisAabb.SetMax(m_position + m_signY * AZ::Vector3(m_baseRadius, m_scaledSize.GetY() + m_arrowLength, m_baseRadius));
        m_yAxisAabb.SetMin(m_position - m_signY * AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));
        m_zAxisAabb.SetMax(m_position + m_signZ * AZ::Vector3(m_baseRadius, m_baseRadius, m_scaledSize.GetZ() + m_arrowLength));
        m_zAxisAabb.SetMin(m_position - m_signZ * AZ::Vector3(m_baseRadius, m_baseRadius, m_baseRadius));

        // set bounding volumes for the plane selectors
        m_xyPlaneAabb.SetMax(m_position + AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, m_secPlaneSelectorPos.GetY() * m_signY, m_baseRadius * m_signZ));
        m_xyPlaneAabb.SetMin(m_position + 0.3f * AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, m_secPlaneSelectorPos.GetY() * m_signY, 0) - AZ::Vector3(m_baseRadius * m_signX, m_baseRadius * m_signY, m_baseRadius * m_signZ));
        m_xzPlaneAabb.SetMax(m_position + AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, m_baseRadius * m_signY, m_secPlaneSelectorPos.GetZ() * m_signZ));
        m_xzPlaneAabb.SetMin(m_position + 0.3f * AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, 0, m_secPlaneSelectorPos.GetZ() * m_signZ) - AZ::Vector3(m_baseRadius * m_signX, m_baseRadius * m_signY, m_baseRadius * m_signZ));
        m_yzPlaneAabb.SetMax(m_position + AZ::Vector3(m_baseRadius * m_signX, m_secPlaneSelectorPos.GetY() * m_signY, m_secPlaneSelectorPos.GetZ() * m_signZ));
        m_yzPlaneAabb.SetMin(m_position + 0.3f * AZ::Vector3(0, m_secPlaneSelectorPos.GetY() * m_signY, m_secPlaneSelectorPos.GetZ() * m_signZ) - AZ::Vector3(m_baseRadius * m_signX, m_baseRadius * m_signY, m_baseRadius * m_signZ));

        // set bounding volume for the box selector
        m_xyzBoxAabb.SetMin(m_position - AZ::Vector3(m_baseRadius * m_signX, m_baseRadius * m_signY, m_baseRadius * m_signZ));
        m_xyzBoxAabb.SetMax(m_position + m_diagScale * AZ::Vector3(m_firstPlaneSelectorPos.GetX() * m_signX, m_firstPlaneSelectorPos.GetY() * m_signY, m_firstPlaneSelectorPos.GetZ() * m_signZ));
    }


    // updates the axis directions with respect to the camera position
    void ScaleManipulator::UpdateAxisDirections(Camera* camera)
    {
        // check if the camera exists
        if (camera == nullptr)
        {
            return;
        }

        // get camera dimensions
        const uint32 screenWidth    = camera->GetScreenWidth();
        const uint32 screenHeight   = camera->GetScreenHeight();

        // shoot ray to the camera center
        MCore::Ray camRay = camera->Unproject(screenWidth / 2, screenHeight / 2);
        AZ::Vector3 camDir = camRay.GetDirection();

        m_signX = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) >= MCore::Math::halfPi - MCore::Math::epsilon) ? 1.0f : -1.0f;
        m_signY = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) >= MCore::Math::halfPi - MCore::Math::epsilon) ? 1.0f : -1.0f;
        m_signZ = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) >= MCore::Math::halfPi - MCore::Math::epsilon) ? 1.0f : -1.0f;

        // determine the axis visibility, to disable movement for invisible axes
        m_xAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        m_yAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        m_zAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
    }


    // check if the manipulator is hit by the mouse.
    bool ScaleManipulator::Hit(Camera* camera, int32 mousePosX, int32 mousePosY)
    {
        // check if camera exists
        if (camera == nullptr)
        {
            return false;
        }

        // update the bounding volumes
        UpdateBoundingVolumes();

        // shoot mousePosRay ar the mouse position
        MCore::Ray mouseRay = camera->Unproject(mousePosX, mousePosY);

        // check if one of the AABBs is hit
        if (mouseRay.Intersects(m_xAxisAabb) ||
            mouseRay.Intersects(m_yAxisAabb) ||
            mouseRay.Intersects(m_zAxisAabb) ||
            mouseRay.Intersects(m_xyPlaneAabb) ||
            mouseRay.Intersects(m_xzPlaneAabb) ||
            mouseRay.Intersects(m_yzPlaneAabb) ||
            mouseRay.Intersects(m_xyzBoxAabb))
        {
            return true;
        }

        // return false if the gizmo is not hit by the mouse
        return false;
    }


    // update the ScaleManipulator
    void ScaleManipulator::Render(MCommon::Camera* camera, RenderUtil* renderUtil)
    {
        // return if no render util is set
        if (renderUtil == nullptr || camera == nullptr || m_isVisible == false)
        {
            return;
        }

        // set m_size variables for the gizmo
        const uint32 screenWidth        = camera->GetScreenWidth();
        const uint32 screenHeight       = camera->GetScreenHeight();

        // update the camera axis directions
        UpdateAxisDirections(camera);

        // set color for the axes, depending on the selection (TODO: maybe put these into the constructor.)
        MCore::RGBAColor xAxisColor     = (m_mode == SCALE_XYZ || m_mode == SCALE_X || m_mode == SCALE_XY || m_mode == SCALE_XZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_red;
        MCore::RGBAColor yAxisColor     = (m_mode == SCALE_XYZ || m_mode == SCALE_Y || m_mode == SCALE_XY || m_mode == SCALE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_green;
        MCore::RGBAColor zAxisColor     = (m_mode == SCALE_XYZ || m_mode == SCALE_Z || m_mode == SCALE_XZ || m_mode == SCALE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_blue;
        MCore::RGBAColor xyPlaneColorX  = (m_mode == SCALE_XYZ || m_mode == SCALE_XY) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_red;
        MCore::RGBAColor xyPlaneColorY  = (m_mode == SCALE_XYZ || m_mode == SCALE_XY) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_green;
        MCore::RGBAColor xzPlaneColorX  = (m_mode == SCALE_XYZ || m_mode == SCALE_XZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_red;
        MCore::RGBAColor xzPlaneColorZ  = (m_mode == SCALE_XYZ || m_mode == SCALE_XZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_blue;
        MCore::RGBAColor yzPlaneColorY  = (m_mode == SCALE_XYZ || m_mode == SCALE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_green;
        MCore::RGBAColor yzPlaneColorZ  = (m_mode == SCALE_XYZ || m_mode == SCALE_YZ) ? ManipulatorColors::s_selectionColor : ManipulatorColors::s_blue;

        // the x axis with cube at the line end
        AZ::Vector3 firstPlanePosX   = m_position + m_signX * AZ::Vector3(m_firstPlaneSelectorPos.GetX(), 0.0f, 0.0f);
        AZ::Vector3 secPlanePosX = m_position + m_signX * AZ::Vector3(m_secPlaneSelectorPos.GetX(), 0.0f, 0.0f);
        if (m_xAxisVisible)
        {
            renderUtil->RenderLine(m_position, m_position + m_signX * AZ::Vector3(m_scaledSize.GetX() + 0.5f * m_baseRadius, 0.0f, 0.0f), xAxisColor);
            AZ::Vector3 quadPos = MCore::Project(m_position + m_signX * AZ::Vector3(m_scaledSize.GetX() + m_baseRadius, 0, 0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderBorderedRect(static_cast<int32>(quadPos.GetX() - 2.0f), static_cast<int32>(quadPos.GetX() + 3.0f), static_cast<int32>(quadPos.GetY() - 2.0f), static_cast<int32>(quadPos.GetY() + 3.0f), ManipulatorColors::s_red, ManipulatorColors::s_red);

            // render the plane selector lines
            renderUtil->RenderLine(firstPlanePosX, firstPlanePosX + m_diagScale * (AZ::Vector3(0, m_firstPlaneSelectorPos.GetY() * m_signY, 0) - AZ::Vector3(m_firstPlaneSelectorPos.GetX() * m_signX, 0, 0)), xyPlaneColorX);
            renderUtil->RenderLine(firstPlanePosX, firstPlanePosX + m_diagScale * (AZ::Vector3(0, 0, m_firstPlaneSelectorPos.GetZ() * m_signZ) - AZ::Vector3(m_firstPlaneSelectorPos.GetX() * m_signX, 0, 0)), xzPlaneColorX);
            renderUtil->RenderLine(secPlanePosX, secPlanePosX + m_diagScale * (AZ::Vector3(0, m_secPlaneSelectorPos.GetY() * m_signY, 0) - AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, 0, 0)), xyPlaneColorX);
            renderUtil->RenderLine(secPlanePosX, secPlanePosX + m_diagScale * (AZ::Vector3(0, 0, m_secPlaneSelectorPos.GetZ() * m_signZ) - AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, 0, 0)), xzPlaneColorX);
        }

        // the y axis with cube at the line end
        AZ::Vector3 firstPlanePosY   = m_position + m_signY * AZ::Vector3(0.0f, m_firstPlaneSelectorPos.GetY(), 0.0f);
        AZ::Vector3 secPlanePosY = m_position + m_signY * AZ::Vector3(0.0f, m_secPlaneSelectorPos.GetY(), 0.0f);
        if (m_yAxisVisible)
        {
            renderUtil->RenderLine(m_position, m_position + m_signY * AZ::Vector3(0.0f, m_scaledSize.GetY(), 0.0f), yAxisColor);
            AZ::Vector3 quadPos = MCore::Project(m_position + m_signY * AZ::Vector3(0, m_scaledSize.GetY() + 0.5f * m_baseRadius, 0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderBorderedRect(static_cast<int32>(quadPos.GetX() - 2.0f), static_cast<int32>(quadPos.GetX() + 3.0f), static_cast<int32>(quadPos.GetY() - 2.0f), static_cast<int32>(quadPos.GetY() + 3.0f), ManipulatorColors::s_green, ManipulatorColors::s_green);

            // render the plane selector lines
            renderUtil->RenderLine(firstPlanePosY, firstPlanePosY + m_diagScale * (AZ::Vector3(m_firstPlaneSelectorPos.GetX() * m_signX, 0, 0) - AZ::Vector3(0, m_firstPlaneSelectorPos.GetY() * m_signY, 0)), xyPlaneColorY);
            renderUtil->RenderLine(firstPlanePosY, firstPlanePosY + m_diagScale * (AZ::Vector3(0, 0, m_firstPlaneSelectorPos.GetZ() * m_signZ) - AZ::Vector3(0, m_firstPlaneSelectorPos.GetY() * m_signY, 0)), yzPlaneColorY);
            renderUtil->RenderLine(secPlanePosY, secPlanePosY + m_diagScale * (AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, 0, 0) - AZ::Vector3(0, m_secPlaneSelectorPos.GetY() * m_signY, 0)), xyPlaneColorY);
            renderUtil->RenderLine(secPlanePosY, secPlanePosY + m_diagScale * (AZ::Vector3(0, 0, m_secPlaneSelectorPos.GetZ() * m_signZ) - AZ::Vector3(0, m_secPlaneSelectorPos.GetY() * m_signY, 0)), yzPlaneColorY);
        }

        // the z axis with cube at the line end
        AZ::Vector3 firstPlanePosZ   = m_position + m_signZ * AZ::Vector3(0.0f, 0.0f, m_firstPlaneSelectorPos.GetZ());
        AZ::Vector3 secPlanePosZ = m_position + m_signZ * AZ::Vector3(0.0f, 0.0f, m_secPlaneSelectorPos.GetZ());
        if (m_zAxisVisible)
        {
            renderUtil->RenderLine(m_position, m_position + m_signZ * AZ::Vector3(0.0f, 0.0f, m_scaledSize.GetZ()), zAxisColor);
            AZ::Vector3 quadPos = MCore::Project(m_position + m_signZ * AZ::Vector3(0, 0, m_scaledSize.GetZ() + 0.5f * m_baseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderBorderedRect(static_cast<int32>(quadPos.GetX() - 2.0f), static_cast<int32>(quadPos.GetX() + 3.0f), static_cast<int32>(quadPos.GetY() - 2.0f), static_cast<int32>(quadPos.GetY() + 3.0f), ManipulatorColors::s_blue, ManipulatorColors::s_blue);

            // render the plane selector lines
            renderUtil->RenderLine(firstPlanePosZ, firstPlanePosZ + m_diagScale * (AZ::Vector3(m_firstPlaneSelectorPos.GetX() * m_signX, 0, 0) - AZ::Vector3(0, 0, m_firstPlaneSelectorPos.GetZ() * m_signZ)), xzPlaneColorZ);
            renderUtil->RenderLine(firstPlanePosZ, firstPlanePosZ + m_diagScale * (AZ::Vector3(0, m_firstPlaneSelectorPos.GetY() * m_signY, 0) - AZ::Vector3(0, 0, m_firstPlaneSelectorPos.GetZ() * m_signZ)), yzPlaneColorZ);
            renderUtil->RenderLine(secPlanePosZ, secPlanePosZ + m_diagScale * (AZ::Vector3(m_secPlaneSelectorPos.GetX() * m_signX, 0, 0) - AZ::Vector3(0, 0, m_secPlaneSelectorPos.GetZ() * m_signZ)), xzPlaneColorZ);
            renderUtil->RenderLine(secPlanePosZ, secPlanePosZ + m_diagScale * (AZ::Vector3(0, m_secPlaneSelectorPos.GetY() * m_signY, 0) - AZ::Vector3(0, 0, m_secPlaneSelectorPos.GetZ() * m_signZ)), yzPlaneColorZ);
        }

        // calculate projected positions for the axis labels and render the text
        AZ::Vector3 textPosY = MCore::Project(m_position + m_signY * AZ::Vector3(0.0, m_scaledSize.GetY() + m_arrowLength + m_baseRadius, -m_baseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
        AZ::Vector3 textPosX = MCore::Project(m_position + m_signX * AZ::Vector3(m_scaledSize.GetX() + m_arrowLength + m_baseRadius, -m_baseRadius, 0.0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
        AZ::Vector3 textPosZ = MCore::Project(m_position + m_signZ * AZ::Vector3(0.0, m_baseRadius, m_scaledSize.GetZ() + m_arrowLength + m_baseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
        renderUtil->RenderText(textPosX.GetX(), textPosX.GetY(), "X", xAxisColor);
        renderUtil->RenderText(textPosY.GetX(), textPosY.GetY(), "Y", yAxisColor);
        renderUtil->RenderText(textPosZ.GetX(), textPosZ.GetY(), "Z", zAxisColor);

        // Render the triangles for plane selection
        if (m_mode == SCALE_XY && m_xAxisVisible && m_yAxisVisible)
        {
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosX, secPlanePosY, ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosY, firstPlanePosY, ManipulatorColors::s_selectionColorDarker);
        }
        else if (m_mode == SCALE_XZ && m_xAxisVisible && m_zAxisVisible)
        {
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosX, secPlanePosZ, ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosZ, firstPlanePosZ, ManipulatorColors::s_selectionColorDarker);
        }
        else if (m_mode == SCALE_YZ && m_yAxisVisible && m_zAxisVisible)
        {
            renderUtil->RenderTriangle(firstPlanePosZ, secPlanePosZ, secPlanePosY, ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(firstPlanePosZ, secPlanePosY, firstPlanePosY, ManipulatorColors::s_selectionColorDarker);
        }
        else if (m_mode == SCALE_XYZ)
        {
            renderUtil->RenderTriangle(firstPlanePosX, firstPlanePosY, firstPlanePosZ, ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(m_position, firstPlanePosX, firstPlanePosZ, ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(m_position, firstPlanePosX, firstPlanePosY, ManipulatorColors::s_selectionColorDarker);
            renderUtil->RenderTriangle(m_position, firstPlanePosY, firstPlanePosZ, ManipulatorColors::s_selectionColorDarker);
        }

        // check if callback exists
        if (m_callback == nullptr)
        {
            return;
        }

        // render the current scale factor in percent if gizmo is hit
        if (m_mode != SCALE_NONE)
        {
            const AZ::Vector3& currScale = m_callback->GetCurrValueVec();
            m_tempString = AZStd::string::format("Abs. Scale X: %.3f, Y: %.3f, Z: %.3f", MCore::Max(float(currScale.GetX()), 0.0f), MCore::Max(float(currScale.GetY()), 0.0f), MCore::Max(float(currScale.GetZ()), 0.0f));
            renderUtil->RenderText(10, 10, m_tempString.c_str(), ManipulatorColors::s_selectionColor, 9.0f);
        }

        // calculate the position offset of the relative text
        float yOffset = (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_PERSPECTIVE) ? 80.0f : 50.0f;

        // text position relative scaling or name displayed below the gizmo
        AZ::Vector3 textPos  = MCore::Project(m_position + (m_size * AZ::Vector3(m_signX, m_signY, m_signZ) / 3.0f), camera->GetViewProjMatrix(), camera->GetScreenWidth(), camera->GetScreenHeight());

        // render the relative scale when moving
        if (m_selectionLocked && m_mode != SCALE_NONE)
        {
            // calculate the scale factor
            AZ::Vector3 scaleFactor = ((AZ::Vector3(m_size, m_size, m_size) + m_scale) / (float)m_size);

            // render the scaling value below the gizmo
            m_tempString = AZStd::string::format("X: %.3f, Y: %.3f, Z: %.3f", MCore::Max(float(scaleFactor.GetX()), 0.0f), MCore::Max(float(scaleFactor.GetY()), 0.0f), MCore::Max(float(scaleFactor.GetZ()), 0.0f));
            renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, m_tempString.c_str(), ManipulatorColors::s_selectionColor, 9.0f, true);
        }
        else
        {
            if (m_name.size() > 0)
            {
                renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, m_name.c_str(), ManipulatorColors::s_selectionColor, 9.0f, true);
            }
        }
    }


    // unproject screen coordinates to a mousePosRay
    void ScaleManipulator::ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);
        MCORE_UNUSED(middleButtonPressed);

        // check if camera has been set
        if (camera == nullptr || m_isVisible == false || (leftButtonPressed && rightButtonPressed))
        {
            return;
        }

        // get screen m_size
        const uint32 screenWidth    = camera->GetScreenWidth();
        const uint32 screenHeight   = camera->GetScreenHeight();

        // update the axis directions with respect to the current camera
        UpdateAxisDirections(camera);

        // generate ray for the collision detection
        MCore::Ray mousePosRay      = camera->Unproject(mousePosX, mousePosY);
        MCore::Ray mousePrevPosRay  = camera->Unproject(mousePosX - mouseMovementX, mousePosY - mouseMovementY);

        // check for the selected axis/plane
        if (m_selectionLocked == false || m_mode == SCALE_NONE)
        {
            // update old rotation of the callback
            if (m_callback)
            {
                m_callback->UpdateOldValues();
            }

            // handle different scale cases depending on the bounding volumes
            if (mousePosRay.Intersects(m_xyzBoxAabb))
            {
                m_mode   = SCALE_XYZ;
                m_scaleDirection = AZ::Vector3(m_signX, m_signY, m_signZ);
            }
            else if (mousePosRay.Intersects(m_xyPlaneAabb) && m_xAxisVisible && m_yAxisVisible)
            {
                m_mode = SCALE_XY;
                m_scaleDirection = AZ::Vector3(m_signX, m_signY, 0.0f);
            }
            else if (mousePosRay.Intersects(m_xzPlaneAabb) && m_xAxisVisible && m_zAxisVisible)
            {
                m_mode = SCALE_XZ;
                m_scaleDirection = AZ::Vector3(m_signX, 0.0f, m_signZ);
            }
            else if (mousePosRay.Intersects(m_yzPlaneAabb) && m_yAxisVisible && m_zAxisVisible)
            {
                m_mode = SCALE_YZ;
                m_scaleDirection = AZ::Vector3(0.0f, m_signY, m_signZ);
            }
            else if (mousePosRay.Intersects(m_xAxisAabb) && m_xAxisVisible)
            {
                m_mode = SCALE_X;
                m_scaleDirection = AZ::Vector3(m_signX, 0.0f, 0.0f);
            }
            else if (mousePosRay.Intersects(m_yAxisAabb) && m_yAxisVisible)
            {
                m_mode = SCALE_Y;
                m_scaleDirection = AZ::Vector3(0.0f, m_signY, 0.0f);
            }
            else if (mousePosRay.Intersects(m_zAxisAabb) && m_zAxisVisible)
            {
                m_mode = SCALE_Z;
                m_scaleDirection = AZ::Vector3(0.0f, 0.0f, m_signZ);
            }
            else
            {
                m_mode = SCALE_NONE;
            }
        }

        // set selection lock
        m_selectionLocked = leftButtonPressed;

        // move the gizmo
        if (m_selectionLocked == false || m_mode == SCALE_NONE)
        {
            m_scale = AZ::Vector3::CreateZero();
            return;
        }

        // init the scale change to zero
        AZ::Vector3 scaleChange = AZ::Vector3::CreateZero();

        // calculate the movement of the mouse on a plane located at the gizmo position
        // and perpendicular to the camera direction
        MCore::Ray camRay = camera->Unproject(screenWidth / 2, screenHeight / 2);
        MCore::PlaneEq movementPlane(camRay.GetDirection(), m_position);

        // calculate the intersection points of the mouse positions with the previously calculated plane
        AZ::Vector3 mousePosIntersect, mousePrevPosIntersect;
        mousePosRay.Intersects(movementPlane, &mousePosIntersect);
        mousePrevPosRay.Intersects(movementPlane, &mousePrevPosIntersect);

        // project the mouse movement onto the scale axis
        scaleChange = (m_scaleDirection * m_scaleDirection.Dot(mousePosIntersect) - m_scaleDirection * m_scaleDirection.Dot(mousePrevPosIntersect));

        // update the scale of the gizmo
        scaleChange = AZ::Vector3(scaleChange.GetX() * m_signX, scaleChange.GetY() * m_signY, scaleChange.GetZ() * m_signZ);
        m_scale += scaleChange;

        // update the callback actor instance
        if (m_callback)
        {
            AZ::Vector3 updateScale = (AZ::Vector3(m_size, m_size, m_size) + m_scale) / (float)m_size;
            m_callback->Update(updateScale);
        }
    }
} // namespace MCommon
