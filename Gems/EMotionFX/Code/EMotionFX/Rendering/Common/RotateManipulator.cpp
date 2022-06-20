/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotateManipulator.h"
#include <MCore/Source/AzCoreConversions.h>
#include <MCore/Source/PlaneEq.h>

namespace MCommon
{
    // constructor
    RotateManipulator::RotateManipulator(float scalingFactor, bool isVisible)
        : TransformationManipulator(scalingFactor, isVisible)
    {
        m_mode               = ROTATE_NONE;
        m_rotation           = AZ::Vector3::CreateZero();
        m_rotationQuat       = AZ::Quaternion::CreateIdentity();
        m_clickPosition      = AZ::Vector3::CreateZero();
    }


    // destructor
    RotateManipulator::~RotateManipulator()
    {
    }


    // function to update the bounding volumes.
    void RotateManipulator::UpdateBoundingVolumes(MCommon::Camera* camera)
    {
        MCORE_UNUSED(camera);

        // adjust the m_size when in ortho mode
        m_size           = m_scalingFactor;
        m_innerRadius    = 0.15f * m_size;
        m_outerRadius    = 0.2f * m_size;
        m_arrowBaseRadius = m_innerRadius / 70.0f;
        m_aabbWidth      = m_innerRadius / 30.0f;// previous 70.0f
        m_axisSize       = m_size * 0.05f;
        m_textDistance   = m_size * 0.05f;
        m_innerQuadSize  = 0.45f * MCore::Math::Sqrt(2) * m_innerRadius;

        // set the bounding volumes of the axes selection
        m_xAxisAabb.SetMax(m_position + AZ::Vector3(m_aabbWidth, m_innerRadius, m_innerRadius));
        m_xAxisAabb.SetMin(m_position - AZ::Vector3(m_aabbWidth, m_innerRadius, m_innerRadius));
        m_yAxisAabb.SetMax(m_position + AZ::Vector3(m_innerRadius, m_aabbWidth, m_innerRadius));
        m_yAxisAabb.SetMin(m_position - AZ::Vector3(m_innerRadius, m_aabbWidth, m_innerRadius));
        m_zAxisAabb.SetMax(m_position + AZ::Vector3(m_innerRadius, m_innerRadius, m_aabbWidth));
        m_zAxisAabb.SetMin(m_position - AZ::Vector3(m_innerRadius, m_innerRadius, m_aabbWidth));
        m_xAxisInnerAabb.SetMax(m_position + AZ::Vector3(m_aabbWidth, m_innerQuadSize, m_innerQuadSize));
        m_xAxisInnerAabb.SetMin(m_position - AZ::Vector3(m_aabbWidth, m_innerQuadSize, m_innerQuadSize));
        m_yAxisInnerAabb.SetMax(m_position + AZ::Vector3(m_innerQuadSize, m_aabbWidth, m_innerQuadSize));
        m_yAxisInnerAabb.SetMin(m_position - AZ::Vector3(m_innerQuadSize, m_aabbWidth, m_innerQuadSize));
        m_zAxisInnerAabb.SetMax(m_position + AZ::Vector3(m_innerQuadSize, m_innerQuadSize, m_aabbWidth));
        m_zAxisInnerAabb.SetMin(m_position - AZ::Vector3(m_innerQuadSize, m_innerQuadSize, m_aabbWidth));

        // set the bounding spheres for inner and outer circle modifiers
        m_innerBoundingSphere.SetCenter(m_position);
        m_innerBoundingSphere.SetRadius(m_innerRadius);
        m_outerBoundingSphere.SetCenter(m_position);
        m_outerBoundingSphere.SetRadius(m_outerRadius);
    }


    // check if the manipulator is hit by the mouse.
    bool RotateManipulator::Hit(Camera* camera, int32 mousePosX, int32 mousePosY)
    {
        // check if camera exists
        if (camera == nullptr)
        {
            return false;
        }

        // update the bounding volumes
        UpdateBoundingVolumes();

        // shoot ray ar the mouse position
        MCore::Ray mousePosRay  = camera->Unproject(mousePosX, mousePosY);

        // check if mouse ray hits the outer sphere of the manipulator
        if (mousePosRay.Intersects(m_outerBoundingSphere))
        {
            return true;
        }

        // return false if manipulator is not hit
        return false;
    }


    // updates the axis directions with respect to the camera position
    void RotateManipulator::UpdateAxisDirections(Camera* camera)
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

        m_signX = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) >= MCore::Math::halfPi) ? 1.0f : -1.0f;
        m_signY = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) >= MCore::Math::halfPi) ? 1.0f : -1.0f;
        m_signZ = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) >= MCore::Math::halfPi) ? 1.0f : -1.0f;

        // determine the axis visibility, to disable movement for invisible axes
        m_xAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        m_yAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        m_zAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);

        // update the bounding volumes
        UpdateBoundingVolumes();
    }


    // update the RotateManipulator
    void RotateManipulator::Render(MCommon::Camera* camera, RenderUtil* renderUtil)
    {
        // return if no render util is set
        if (renderUtil == nullptr || camera == nullptr || m_isVisible == false)
        {
            return;
        }

        // set m_size variables for the gizmo
        const uint32 screenWidth    = camera->GetScreenWidth();
        const uint32 screenHeight   = camera->GetScreenHeight();

        // set color for the axes, depending on the selection
        MCore::RGBAColor grey               = MCore::RGBAColor(0.5f, 0.5f, 0.5f);
        MCore::RGBAColor redTransparent     = MCore::RGBAColor(0.781f, 0.0, 0.0, 0.2f);
        MCore::RGBAColor greenTransparent   = MCore::RGBAColor(0.0, 0.609f, 0.0, 0.2f);
        MCore::RGBAColor blueTransparent    = MCore::RGBAColor(0.0, 0.0, 0.762f, 0.2f);
        MCore::RGBAColor greyTransparent    = MCore::RGBAColor(0.5f, 0.5f, 0.5f, 0.3f);

        MCore::RGBAColor xAxisColor         = (m_mode == ROTATE_X)           ?   ManipulatorColors::s_selectionColor : ManipulatorColors::s_red;
        MCore::RGBAColor yAxisColor         = (m_mode == ROTATE_Y)           ?   ManipulatorColors::s_selectionColor : ManipulatorColors::s_green;
        MCore::RGBAColor zAxisColor         = (m_mode == ROTATE_Z)           ?   ManipulatorColors::s_selectionColor : ManipulatorColors::s_blue;
        MCore::RGBAColor camRollAxisColor   = (m_mode == ROTATE_CAMROLL)     ?   ManipulatorColors::s_selectionColor : grey;

        // render axis in the center of the rotation gizmo
        renderUtil->RenderAxis(m_axisSize, m_position, AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f));

        // shoot rays to the plane, to get upwards pointing vector on the plane
        // used for the text positioning and the angle visualization for the view rotation axis
        MCore::Ray      originRay = camera->Unproject(screenWidth / 2, screenHeight / 2);
        MCore::Ray      upVecRay = camera->Unproject(screenWidth / 2, screenHeight / 2 - 10);
        AZ::Vector3     camRollAxis = originRay.GetDirection();

        // calculate the plane perpendicular to the view rotation axis
        MCore::PlaneEq rotationPlane(originRay.GetDirection(), m_position);

        // get the intersection points of the rays and the plane
        AZ::Vector3 originRayIntersect, upVecRayIntersect;
        originRay.Intersects(rotationPlane, &originRayIntersect);
        upVecRay.Intersects(rotationPlane, &upVecRayIntersect);

        // use the cross product to calculate left vector from view direction and upVector
        AZ::Vector3 upVector = (upVecRayIntersect - originRayIntersect).GetNormalized();
        AZ::Vector3 leftVector   = originRay.GetDirection().Cross(upVector);
        leftVector.Normalize();

        // get the view matrix of the camera and calculate inverse,
        // which is used for the transformation of the cam axis rotation manipulator
        AZ::Matrix4x4 camViewMat = camera->GetViewMatrix();
        camViewMat.InvertFull();

        // set the translation part of the matrix
        camViewMat.SetTranslation(m_position);

        // render the view axis rotation manipulator
        const AZ::Transform camViewTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateFromMatrix4x4(camViewMat), camViewMat.GetTranslation());
        renderUtil->RenderCircle(camViewTransform, m_outerRadius, 64, camRollAxisColor);
        renderUtil->RenderCircle(camViewTransform, m_innerRadius, 64, grey);

        if (m_mode == ROTATE_CAMPITCHYAW)
        {
            renderUtil->RenderCircle(camViewTransform, m_innerRadius, 64, grey, 0.0f, MCore::Math::twoPi, true, greyTransparent);
        }

        // calculate the signs of the rotation and the angle between the axes and the click position
        const float signX = (m_rotation.GetX() >= 0) ? 1.0f : -1.0f;
        const float signY = (m_rotation.GetY() >= 0) ? 1.0f : -1.0f;
        const float signZ = (m_rotation.GetZ() >= 0) ? 1.0f : -1.0f;
        const float angleX = MCore::Math::ACos(m_clickPosition.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f)));
        const float angleY = MCore::Math::ACos(m_clickPosition.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f)));
        const float angleZ = MCore::Math::ACos(m_clickPosition.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f)));

        // transformation matrix for the circle for rotation around the x axis
        AZ::Transform rotMatrixX = MCore::GetRotationMatrixAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), signX * MCore::Math::halfPi);

        // set the translation part of the matrix
        rotMatrixX.SetTranslation(m_position);

        // render the circle for the rotation around the x axis
        if (m_mode == ROTATE_X)
        {
            renderUtil->RenderCircle(rotMatrixX, m_innerRadius, 64, grey);
        }

        renderUtil->RenderCircle(rotMatrixX, m_innerRadius, 64, xAxisColor, 0.0f, MCore::Math::twoPi, false, MCore::RGBAColor(), true, camRollAxis);

        // draw current angle if in x rotation mode
        if (m_mode == ROTATE_X)
        {
            // handle different dot product results (necessary because dot product only handles a range of [0, pi])
            if (angleZ > MCore::Math::halfPi)
            {
                rotMatrixX = AZ::Transform::CreateRotationZ(-1.0f * signX * angleY) * rotMatrixX;
            }
            else
            {
                rotMatrixX = AZ::Transform::CreateRotationZ(signX * angleY) * rotMatrixX;
            }

            // set the translation part of the matrix
            rotMatrixX.SetTranslation(m_position);

            // render the rotated circle segment to represent the current rotation angle around the x axis
            renderUtil->RenderCircle(rotMatrixX, m_innerRadius, 64, ManipulatorColors::s_selectionColor, 0.0f, MCore::Math::Abs(m_rotation.GetX()), true, redTransparent, true, camRollAxis);
        }

        // rotation matrix for rotation around the y axis
        AZ::Transform rotMatrixY = MCore::GetRotationMatrixAxisAngle(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::halfPi);

        // set the translation part of the matrix
        rotMatrixY.SetTranslation(m_position);

        // render the circle for rotation around the y axis
        if (m_mode == ROTATE_Y)
        {
            renderUtil->RenderCircle(rotMatrixY, m_innerRadius, 64, grey);
        }

        renderUtil->RenderCircle(rotMatrixY, m_innerRadius, 64, yAxisColor, 0.0f, MCore::Math::twoPi, false, MCore::RGBAColor(), true, camRollAxis);

        // draw current angle if in y rotation mode
        if (m_mode == ROTATE_Y)
        {
            // render current rotation angle depending on the dot product results calculated above
            if (signY > 0)
            {
                rotMatrixY = AZ::Transform::CreateRotationY(MCore::Math::pi) * rotMatrixY;
            }

            // handle different dot product results (necessary because dot product only handles a range of [0, pi])
            if (angleX > MCore::Math::halfPi)
            {
                rotMatrixY = AZ::Transform::CreateRotationZ(-1.0f * signY * angleZ) * rotMatrixY;
            }
            else if ((angleZ < MCore::Math::halfPi) && (angleX < MCore::Math::halfPi))
            {
                rotMatrixY = AZ::Transform::CreateRotationZ(MCore::Math::twoPi + signY * angleZ) * rotMatrixY;
            }
            else
            {
                rotMatrixY = AZ::Transform::CreateRotationZ(signY * angleZ) * rotMatrixY;
            }

            // set the translation part of the matrix
            rotMatrixY.SetTranslation(m_position);

            // render the rotated circle segment to represent the current rotation angle around the y axis
            renderUtil->RenderCircle(rotMatrixY, m_innerRadius, 64, ManipulatorColors::s_selectionColor, 0.0f, MCore::Math::Abs(m_rotation.GetY()), true, greenTransparent, true, camRollAxis);
        }

        // the circle for rotation around the z axis
        AZ::Transform rotMatrixZ = AZ::Transform::CreateIdentity();

        // set the translation part of the matrix
        rotMatrixZ.SetTranslation(m_position);

        // render the circle for rotation around the z axis
        if (m_mode == ROTATE_Z)
        {
            renderUtil->RenderCircle(rotMatrixZ, m_innerRadius, 64, grey);
        }

        renderUtil->RenderCircle(rotMatrixZ, m_innerRadius, 64, zAxisColor, 0.0f, MCore::Math::twoPi, false, MCore::RGBAColor(), true, camRollAxis);

        // draw current angle if in z rotation mode
        if (m_mode == ROTATE_Z)
        {
            // render current rotation angle depending on the dot product results calculated above
            if (signZ < 0.0f)
            {
                rotMatrixZ = MCore::GetRotationMatrixAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), MCore::Math::pi);
            }

            // handle different dot product results (necessary because dot product only handles a range of [0, pi])
            if (angleX > MCore::Math::halfPi)
            {
                rotMatrixZ = AZ::Transform::CreateRotationZ(signZ * angleY) * rotMatrixZ;
            }
            else if ((angleX < MCore::Math::halfPi) && (angleY < MCore::Math::halfPi))
            {
                rotMatrixZ = AZ::Transform::CreateRotationZ(MCore::Math::twoPi - signZ * angleY) * rotMatrixZ;
            }
            else
            {
                rotMatrixZ = AZ::Transform::CreateRotationZ(-1.0f * signZ * angleY) * rotMatrixZ;
            }

            // set the translation part of the matrix
            rotMatrixZ.SetTranslation(m_position);

            // render the rotated circle segment to represent the current rotation angle around the z axis
            renderUtil->RenderCircle(rotMatrixZ, m_innerRadius, 64, ManipulatorColors::s_selectionColor, 0.0f, MCore::Math::Abs(m_rotation.GetZ()), true, blueTransparent, true, camRollAxis);
        }

        // break if in different projection mode and camera roll rotation mode
        if (m_currentProjectionMode != camera->GetProjectionMode() && m_mode == ROTATE_CAMROLL)
        {
            return;
        }

        // render the absolute rotation if gizmo is hit
        if (m_mode != ROTATE_NONE)
        {
            const AZ::Vector3 currRot = MCore::AzQuaternionToEulerAngles(m_callback->GetCurrValueQuat());
            m_tempString = AZStd::string::format("Abs. Rotation X: %.3f, Y: %.3f, Z: %.3f", MCore::Math::RadiansToDegrees(currRot.GetX() + MCore::Math::epsilon), MCore::Math::RadiansToDegrees(currRot.GetY() + MCore::Math::epsilon), MCore::Math::RadiansToDegrees(currRot.GetZ() + MCore::Math::epsilon));
            renderUtil->RenderText(10, 10, m_tempString.c_str(), ManipulatorColors::s_selectionColor, 9.0f);
        }

        // if the rotation has been changed draw the current direction of the rotation
        if (m_rotation.GetLength() > 0.0f)
        {
            // render text with the rotation values of the axes
            float radius = (m_mode == ROTATE_CAMROLL) ? m_outerRadius : m_innerRadius;
            m_tempString = AZStd::string::format("[%.2f, %.2f, %.2f]", MCore::Math::RadiansToDegrees(m_rotation.GetX()), MCore::Math::RadiansToDegrees(m_rotation.GetY()), MCore::Math::RadiansToDegrees(m_rotation.GetZ()));
            //String rotationValues = String() = AZStd::string::format("[%.2f, %.2f, %.2f]", camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z);
            AZ::Vector3 textPosition = MCore::Project(m_position + (upVector * (m_outerRadius + m_textDistance)), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosition.GetX() - 2.9f * m_tempString.size(), textPosition.GetY(), m_tempString.c_str(), ManipulatorColors::s_selectionColor);

            // mark the click position with a small cube
            AZ::Vector3 clickPosition = m_position + m_clickPosition * radius;

            // calculate the tangent at the click position
            MCore::RGBAColor rotDirColorNegative = (m_rotation.Dot(m_rotationAxis) > 0.0f) ? ManipulatorColors::s_selectionColor : grey;
            MCore::RGBAColor rotDirColorPositive = (m_rotation.Dot(m_rotationAxis) < 0.0f) ? ManipulatorColors::s_selectionColor : grey;

            // render the tangent directions at the click positions
            AZ::Vector3 tangent = m_rotationAxis.Cross(m_clickPosition).GetNormalized();
            renderUtil->RenderLine(clickPosition, clickPosition + 1.5f * m_axisSize * tangent, rotDirColorPositive);
            renderUtil->RenderLine(clickPosition, clickPosition - 1.5f * m_axisSize * tangent, rotDirColorNegative);
            renderUtil->RenderCylinder(2.0f * m_arrowBaseRadius, 0.0f, 0.5f * m_axisSize, clickPosition + 1.5f * m_axisSize * tangent, tangent, rotDirColorPositive);
            renderUtil->RenderCylinder(2.0f * m_arrowBaseRadius, 0.0f, 0.5f * m_axisSize, clickPosition - 1.5f * m_axisSize * tangent, -tangent, rotDirColorNegative);
        }
        else
        {
            if (m_name.size() > 0)
            {
                AZ::Vector3 textPosition = MCore::Project(m_position + (upVector * (m_outerRadius + m_textDistance)), camera->GetViewProjMatrix(), screenWidth, screenHeight);
                renderUtil->RenderText(textPosition.GetX(), textPosition.GetY(), m_name.c_str(), ManipulatorColors::s_selectionColor, 11.0f, true);
            }
        }
    }


    // unproject screen coordinates to a ray
    void RotateManipulator::ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);
        MCORE_UNUSED(middleButtonPressed);

        // check if camera has been set
        if (camera == nullptr || m_isVisible == false || (leftButtonPressed && rightButtonPressed))
        {
            return;
        }

        // update the axis visibility flags
        UpdateAxisDirections(camera);

        // get screen m_size
        uint32 screenWidth  = camera->GetScreenWidth();
        uint32 screenHeight = camera->GetScreenHeight();

        // generate rays for the collision detection
        MCore::Ray mousePosRay          = camera->Unproject(mousePosX, mousePosY);
        //MCore::Ray mousePrevPosRay    = camera->Unproject( mousePosX-mouseMovementX, mousePosY-mouseMovementY );
        MCore::Ray camRollRay           = camera->Unproject(screenWidth / 2, screenHeight / 2);
        AZ::Vector3 camRollAxis         = camRollRay.GetDirection();
        m_rotationQuat                   = AZ::Quaternion::CreateIdentity();

        // check for the selected axis/plane
        if (m_selectionLocked == false || m_mode == ROTATE_NONE)
        {
            // update old rotation of the callback
            if (m_callback)
            {
                m_callback->UpdateOldValues();
            }

            // the intersection variables
            AZ::Vector3 intersectA, intersectB;

            // set rotation mode to rotation around the x axis, if the following intersection conditions are fulfilled
            // innerAABB not hit, outerAABB hit, innerBoundingSphere hit and angle between cameraRollAxis and clickPosition > pi/2
            if ((mousePosRay.Intersects(m_xAxisInnerAabb) == false ||
                 (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC && m_xAxisVisible)) &&
                mousePosRay.Intersects(m_xAxisAabb, &intersectA, &intersectB) &&
                mousePosRay.Intersects(m_innerBoundingSphere) &&
                MCore::Math::ACos(camRollAxis.Dot((intersectA - m_position).GetNormalized())) > MCore::Math::halfPi)
            {
                m_mode = ROTATE_X;
                m_rotationAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);
                m_clickPosition = (intersectA - m_position).GetNormalized();
                m_clickPosition.SetX(0.0f);
            }

            // set rotation mode to rotation around the y axis, if the following intersection conditions are fulfilled
            // innerAABB not hit, outerAABB hit, innerBoundingSphere hit and angle between cameraRollAxis and clickPosition > pi/2
            else if ((mousePosRay.Intersects(m_yAxisInnerAabb) == false ||
                      (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC && m_yAxisVisible)) &&
                     mousePosRay.Intersects(m_yAxisAabb, &intersectA, &intersectB) && mousePosRay.Intersects(m_innerBoundingSphere) &&
                     MCore::Math::ACos(camRollAxis.Dot((intersectA - m_position).GetNormalized())) > MCore::Math::halfPi)
            {
                m_mode = ROTATE_Y;
                m_rotationAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);
                m_clickPosition = (intersectA - m_position).GetNormalized();
                m_clickPosition.SetY(0.0f);
            }

            // set rotation mode to rotation around the z axis, if the following intersection conditions are fulfilled
            // innerAABB not hit, outerAABB hit, innerBoundingSphere hit and angle between cameraRollAxis and clickPosition > pi/2
            else if ((mousePosRay.Intersects(m_zAxisInnerAabb) == false ||
                      (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC && m_zAxisVisible)) &&
                     mousePosRay.Intersects(m_zAxisAabb, &intersectA, &intersectB) && mousePosRay.Intersects(m_innerBoundingSphere) &&
                     MCore::Math::ACos(camRollAxis.Dot((intersectA - m_position).GetNormalized())) > MCore::Math::halfPi)
            {
                m_mode = ROTATE_Z;
                m_rotationAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);
                m_clickPosition = (intersectA - m_position).GetNormalized();
                m_clickPosition.SetZ(0.0f);
            }

            // set rotation mode to rotation around the pitch and yaw axis of the camera,
            // if the inner sphere is hit and none of the previous conditions was fulfilled
            else if (mousePosRay.Intersects(m_innerBoundingSphere, &intersectA, &intersectB))
            {
                m_mode = ROTATE_CAMPITCHYAW;

                // set the rotation axis to zero, because no single axis exists in this mode
                m_rotationAxis = AZ::Vector3::CreateZero();

                // project the click position onto the plane which is perpendicular to the rotation direction
                MCore::PlaneEq rotationPlane(camRollAxis, m_position);
                m_clickPosition = (rotationPlane.Project(intersectA - m_position)).GetNormalized();
            }

            // set rotation mode to rotation around the roll axis of the camera,
            // if the outer sphere is hit and none of the previous conditions was fulfilled
            else if (mousePosRay.Intersects(m_outerBoundingSphere, &intersectA, &intersectB))
            {
                // set rotation mode to rotate around the view axis
                m_mode = ROTATE_CAMROLL;

                // set the rotation axis to the look at ray direction
                m_rotationAxis = camRollRay.GetDirection();

                // project the click position onto the plane which is perpendicular to the rotation direction
                MCore::PlaneEq rotationPlane(m_rotationAxis, AZ::Vector3::CreateZero());
                m_clickPosition = (rotationPlane.Project(intersectA - m_position)).GetNormalized();
            }

            // no bounding volume is currently hit, therefore do not rotate
            else
            {
                m_mode = ROTATE_NONE;
            }
        }

        // set selection lock and current projection mode
        m_selectionLocked = leftButtonPressed;
        m_currentProjectionMode = camera->GetProjectionMode();

        // reset the gizmo if no rotation mode is selected
        if (m_selectionLocked == false || m_mode == ROTATE_NONE)
        {
            m_rotation = AZ::Vector3::CreateZero();
            return;
        }

        //  calculate movement length and convert the mouse movement into a normalized vector
        AZ::Vector3 mouseMovementV3 = AZ::Vector3((float)mouseMovementX, (float)mouseMovementY, 0.0);
        float movementLength = mouseMovementV3.GetLength();
        mouseMovementV3.Normalize();
        if (movementLength <= MCore::Math::epsilon)
        {
            return;
        }

        // set the rotation depending on the rotation mode
        if (m_mode == ROTATE_CAMPITCHYAW)
        {
            // the yaw axis of the camera view
            MCore::Ray camYawRay = camera->Unproject(screenWidth / 2, screenHeight / 2 - 10);

            // calculate the plane perpendicular to the view rotation axis
            MCore::PlaneEq rotationPlane(camRollAxis, m_position);

            // get the intersection points of the rays and the plane
            AZ::Vector3 originRayIntersect, upVecRayIntersect;
            camRollRay.Intersects(rotationPlane, &originRayIntersect);
            camYawRay.Intersects(rotationPlane, &upVecRayIntersect);

            // use the cross product to calculate left vector from view direction and upVector
            AZ::Vector3 upVector = (upVecRayIntersect - originRayIntersect).GetNormalized();
            AZ::Vector3 leftVector   = camRollAxis.Cross(upVector);
            upVector.Normalize();
            leftVector.Normalize();

            // calculate the projected axes, used to determine the angle between click position
            // and the axes. This allows weighting the angles by the movement direction.
            AZ::Vector3 projectedCenter          = MCore::Project(m_position, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projectedClickPosYaw     = MCore::Project(m_position - leftVector, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projectedClickPosPitch   = MCore::Project(m_position - upVector, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projDirClickPosYaw       = (projectedClickPosYaw - projectedCenter).GetNormalized();
            AZ::Vector3 projDirClickPosPitch     = (projectedClickPosPitch - projectedCenter).GetNormalized();

            // calculate the angle between mouse movement and projected rotation axis
            float angleYaw      = projDirClickPosYaw.Dot(mouseMovementV3) * m_scalingFactor * movementLength * 0.00005f;
            float anglePitch    = projDirClickPosPitch.Dot(mouseMovementV3) * m_scalingFactor * movementLength * 0.00005f;

            // perform rotation arround the cam yaw and pitch axis
            AZ::Quaternion rotation = MCore::CreateFromAxisAndAngle(upVector, -angleYaw);
            rotation = rotation * MCore::CreateFromAxisAndAngle(leftVector, anglePitch);

            // set euler angles of the rotation variable
            m_rotation += MCore::AzQuaternionToEulerAngles(rotation);
            m_rotationQuat = rotation;
        }
        else
        {
            // calculate the projected center and click position to determine the rotation angle
            AZ::Vector3 tangent          = (m_rotationAxis.Cross(m_clickPosition)).GetNormalized();
            AZ::Vector3 projectedCenter  = MCore::Project(m_position, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projectedClickPos = MCore::Project(m_position - tangent, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projDirClickPos  = (projectedClickPos - projectedCenter);

            float angle = MCore::Math::DegreesToRadians(MCore::Sgn<float>(projDirClickPos.Dot(mouseMovementV3)) * 0.2f * MCore::Math::Floor(movementLength + 0.5f));

            // adjust rotation
            m_rotation += m_rotationAxis * angle;
            m_rotationQuat = AZ::Quaternion::CreateFromAxisAngle(m_rotationAxis, MCore::Math::FMod(-angle, MCore::Math::twoPi));
            m_rotationQuat.Normalize();
        }

        // update the callback
        if (m_callback)
        {
            const AZ::Quaternion curRot = m_callback->GetCurrValueQuat();
            const AZ::Quaternion newRot = (curRot * m_rotationQuat).GetNormalized();
            m_callback->Update(newRot);
        }
    }
} // namespace MCommon
