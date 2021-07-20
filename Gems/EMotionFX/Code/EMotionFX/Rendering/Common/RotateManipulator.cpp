/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "RotateManipulator.h"
#include <MCore/Source/AzCoreConversions.h>


namespace MCommon
{
    // constructor
    RotateManipulator::RotateManipulator(float scalingFactor, bool isVisible)
        : TransformationManipulator(scalingFactor, isVisible)
    {
        mMode               = ROTATE_NONE;
        mRotation           = AZ::Vector3::CreateZero();
        mRotationQuat       = AZ::Quaternion::CreateIdentity();
        mClickPosition      = AZ::Vector3::CreateZero();
    }


    // destructor
    RotateManipulator::~RotateManipulator()
    {
    }


    // function to update the bounding volumes.
    void RotateManipulator::UpdateBoundingVolumes(MCommon::Camera* camera)
    {
        MCORE_UNUSED(camera);

        // adjust the mSize when in ortho mode
        mSize           = mScalingFactor;
        mInnerRadius    = 0.15f * mSize;
        mOuterRadius    = 0.2f * mSize;
        mArrowBaseRadius = mInnerRadius / 70.0f;
        mAABBWidth      = mInnerRadius / 30.0f;// previous 70.0f
        mAxisSize       = mSize * 0.05f;
        mTextDistance   = mSize * 0.05f;
        mInnerQuadSize  = 0.45f * MCore::Math::Sqrt(2) * mInnerRadius;

        // set the bounding volumes of the axes selection
        mXAxisAABB.SetMax(mPosition + AZ::Vector3(mAABBWidth, mInnerRadius, mInnerRadius));
        mXAxisAABB.SetMin(mPosition - AZ::Vector3(mAABBWidth, mInnerRadius, mInnerRadius));
        mYAxisAABB.SetMax(mPosition + AZ::Vector3(mInnerRadius, mAABBWidth, mInnerRadius));
        mYAxisAABB.SetMin(mPosition - AZ::Vector3(mInnerRadius, mAABBWidth, mInnerRadius));
        mZAxisAABB.SetMax(mPosition + AZ::Vector3(mInnerRadius, mInnerRadius, mAABBWidth));
        mZAxisAABB.SetMin(mPosition - AZ::Vector3(mInnerRadius, mInnerRadius, mAABBWidth));
        mXAxisInnerAABB.SetMax(mPosition + AZ::Vector3(mAABBWidth, mInnerQuadSize, mInnerQuadSize));
        mXAxisInnerAABB.SetMin(mPosition - AZ::Vector3(mAABBWidth, mInnerQuadSize, mInnerQuadSize));
        mYAxisInnerAABB.SetMax(mPosition + AZ::Vector3(mInnerQuadSize, mAABBWidth, mInnerQuadSize));
        mYAxisInnerAABB.SetMin(mPosition - AZ::Vector3(mInnerQuadSize, mAABBWidth, mInnerQuadSize));
        mZAxisInnerAABB.SetMax(mPosition + AZ::Vector3(mInnerQuadSize, mInnerQuadSize, mAABBWidth));
        mZAxisInnerAABB.SetMin(mPosition - AZ::Vector3(mInnerQuadSize, mInnerQuadSize, mAABBWidth));

        // set the bounding spheres for inner and outer circle modifiers
        mInnerBoundingSphere.SetCenter(mPosition);
        mInnerBoundingSphere.SetRadius(mInnerRadius);
        mOuterBoundingSphere.SetCenter(mPosition);
        mOuterBoundingSphere.SetRadius(mOuterRadius);
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
        if (mousePosRay.Intersects(mOuterBoundingSphere))
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

        mSignX = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) >= MCore::Math::halfPi) ? 1.0f : -1.0f;
        mSignY = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) >= MCore::Math::halfPi) ? 1.0f : -1.0f;
        mSignZ = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) >= MCore::Math::halfPi) ? 1.0f : -1.0f;

        // determine the axis visibility, to disable movement for invisible axes
        mXAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        mYAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        mZAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);

        // update the bounding volumes
        UpdateBoundingVolumes();
    }


    // update the RotateManipulator
    void RotateManipulator::Render(MCommon::Camera* camera, RenderUtil* renderUtil)
    {
        // return if no render util is set
        if (renderUtil == nullptr || camera == nullptr || mIsVisible == false)
        {
            return;
        }

        // set mSize variables for the gizmo
        const uint32 screenWidth    = camera->GetScreenWidth();
        const uint32 screenHeight   = camera->GetScreenHeight();

        // set color for the axes, depending on the selection
        MCore::RGBAColor grey               = MCore::RGBAColor(0.5f, 0.5f, 0.5f);
        MCore::RGBAColor redTransparent     = MCore::RGBAColor(0.781f, 0.0, 0.0, 0.2f);
        MCore::RGBAColor greenTransparent   = MCore::RGBAColor(0.0, 0.609f, 0.0, 0.2f);
        MCore::RGBAColor blueTransparent    = MCore::RGBAColor(0.0, 0.0, 0.762f, 0.2f);
        MCore::RGBAColor greyTransparent    = MCore::RGBAColor(0.5f, 0.5f, 0.5f, 0.3f);

        MCore::RGBAColor xAxisColor         = (mMode == ROTATE_X)           ?   ManipulatorColors::mSelectionColor : ManipulatorColors::mRed;
        MCore::RGBAColor yAxisColor         = (mMode == ROTATE_Y)           ?   ManipulatorColors::mSelectionColor : ManipulatorColors::mGreen;
        MCore::RGBAColor zAxisColor         = (mMode == ROTATE_Z)           ?   ManipulatorColors::mSelectionColor : ManipulatorColors::mBlue;
        MCore::RGBAColor camRollAxisColor   = (mMode == ROTATE_CAMROLL)     ?   ManipulatorColors::mSelectionColor : grey;
        //MCore::RGBAColor camPitchYawColor = (mMode == ROTATE_CAMPITCHYAW) ?   ManipulatorColors::mSelectionColor : grey;

        // render axis in the center of the rotation gizmo
        renderUtil->RenderAxis(mAxisSize, mPosition, AZ::Vector3(1.0f, 0.0f, 0.0f), AZ::Vector3(0.0f, 1.0f, 0.0f), AZ::Vector3(0.0f, 0.0f, 1.0f));

        // shoot rays to the plane, to get upwards pointing vector on the plane
        // used for the text positioning and the angle visualization for the view rotation axis
        MCore::Ray      originRay = camera->Unproject(screenWidth / 2, screenHeight / 2);
        MCore::Ray      upVecRay = camera->Unproject(screenWidth / 2, screenHeight / 2 - 10);
        AZ::Vector3     camRollAxis = originRay.GetDirection();

        // calculate the plane perpendicular to the view rotation axis
        MCore::PlaneEq rotationPlane(originRay.GetDirection(), mPosition);

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
        camViewMat.SetTranslation(mPosition);

        // render the view axis rotation manipulator
        const AZ::Transform camViewTransform = AZ::Transform::CreateFromMatrix3x3AndTranslation(
            AZ::Matrix3x3::CreateFromMatrix4x4(camViewMat), camViewMat.GetTranslation());
        renderUtil->RenderCircle(camViewTransform, mOuterRadius, 64, camRollAxisColor);
        renderUtil->RenderCircle(camViewTransform, mInnerRadius, 64, grey);

        if (mMode == ROTATE_CAMPITCHYAW)
        {
            renderUtil->RenderCircle(camViewTransform, mInnerRadius, 64, grey, 0.0f, MCore::Math::twoPi, true, greyTransparent);
        }

        // handle the rotation around the camera roll axis
        /*
        if (mMode == ROTATE_CAMROLL)
        {
            // calculate angle of the click position to the reference directions
            const float angleUp     = Math::ACos( mClickPosition.Dot(upVector) );
            const float angleLeft   = Math::ACos( mClickPosition.Dot(leftVector) );

            // rotate the whole circle around pi, if rotation is in the negative direction
            if (mRotation.Dot(mRotationAxis) < 0)
                camViewMat = camViewMat * camViewMat.RotationMatrixAxisAngle( upVector, Math::pi );

            // handle different dot product results (necessary because dot product only handles a range of [0, pi])
            if (angleLeft > Math::halfPi)
                camViewMat = camViewMat * camViewMat.RotationMatrixAxisAngle( mRotationAxis, -angleUp );
            else
                camViewMat = camViewMat * camViewMat.RotationMatrixAxisAngle( mRotationAxis, angleUp );

            // render the rotated circle segment to represent the current rotation angle around the view axis
            renderUtil->RenderCircle( camViewMat, mOuterRadius, 64, ManipulatorColors::mSelectionColor, 0.0f, mRotation.Length(), true, greyTransparent );
        }
        */

        // calculate the signs of the rotation and the angle between the axes and the click position
        const float signX = (mRotation.GetX() >= 0) ? 1.0f : -1.0f;
        const float signY = (mRotation.GetY() >= 0) ? 1.0f : -1.0f;
        const float signZ = (mRotation.GetZ() >= 0) ? 1.0f : -1.0f;
        const float angleX = MCore::Math::ACos(mClickPosition.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f)));
        const float angleY = MCore::Math::ACos(mClickPosition.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f)));
        const float angleZ = MCore::Math::ACos(mClickPosition.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f)));

        // transformation matrix for the circle for rotation around the x axis
        AZ::Transform rotMatrixX = MCore::GetRotationMatrixAxisAngle(AZ::Vector3(0.0f, 1.0f, 0.0f), signX * MCore::Math::halfPi);

        // set the translation part of the matrix
        rotMatrixX.SetTranslation(mPosition);

        // render the circle for the rotation around the x axis
        if (mMode == ROTATE_X)
        {
            renderUtil->RenderCircle(rotMatrixX, mInnerRadius, 64, grey);
        }

        renderUtil->RenderCircle(rotMatrixX, mInnerRadius, 64, xAxisColor, 0.0f, MCore::Math::twoPi, false, MCore::RGBAColor(), true, camRollAxis);

        // draw current angle if in x rotation mode
        if (mMode == ROTATE_X)
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
            rotMatrixX.SetTranslation(mPosition);

            // render the rotated circle segment to represent the current rotation angle around the x axis
            renderUtil->RenderCircle(rotMatrixX, mInnerRadius, 64, ManipulatorColors::mSelectionColor, 0.0f, MCore::Math::Abs(mRotation.GetX()), true, redTransparent, true, camRollAxis);
        }

        // rotation matrix for rotation around the y axis
        AZ::Transform rotMatrixY = MCore::GetRotationMatrixAxisAngle(AZ::Vector3(1.0f, 0.0f, 0.0f), MCore::Math::halfPi);

        // set the translation part of the matrix
        rotMatrixY.SetTranslation(mPosition);

        // render the circle for rotation around the y axis
        if (mMode == ROTATE_Y)
        {
            renderUtil->RenderCircle(rotMatrixY, mInnerRadius, 64, grey);
        }

        renderUtil->RenderCircle(rotMatrixY, mInnerRadius, 64, yAxisColor, 0.0f, MCore::Math::twoPi, false, MCore::RGBAColor(), true, camRollAxis);

        // draw current angle if in y rotation mode
        if (mMode == ROTATE_Y)
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
            rotMatrixY.SetTranslation(mPosition);

            // render the rotated circle segment to represent the current rotation angle around the y axis
            renderUtil->RenderCircle(rotMatrixY, mInnerRadius, 64, ManipulatorColors::mSelectionColor, 0.0f, MCore::Math::Abs(mRotation.GetY()), true, greenTransparent, true, camRollAxis);
        }

        // the circle for rotation around the z axis
        AZ::Transform rotMatrixZ = AZ::Transform::CreateIdentity();

        // set the translation part of the matrix
        rotMatrixZ.SetTranslation(mPosition);

        // render the circle for rotation around the z axis
        if (mMode == ROTATE_Z)
        {
            renderUtil->RenderCircle(rotMatrixZ, mInnerRadius, 64, grey);
        }

        renderUtil->RenderCircle(rotMatrixZ, mInnerRadius, 64, zAxisColor, 0.0f, MCore::Math::twoPi, false, MCore::RGBAColor(), true, camRollAxis);

        // draw current angle if in z rotation mode
        if (mMode == ROTATE_Z)
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
            rotMatrixZ.SetTranslation(mPosition);

            // render the rotated circle segment to represent the current rotation angle around the z axis
            renderUtil->RenderCircle(rotMatrixZ, mInnerRadius, 64, ManipulatorColors::mSelectionColor, 0.0f, MCore::Math::Abs(mRotation.GetZ()), true, blueTransparent, true, camRollAxis);
        }

        // break if in different projection mode and camera roll rotation mode
        if (mCurrentProjectionMode != camera->GetProjectionMode() && mMode == ROTATE_CAMROLL)
        {
            return;
        }

        // render the absolute rotation if gizmo is hit
        if (mMode != ROTATE_NONE)
        {
            const AZ::Vector3 currRot = MCore::AzQuaternionToEulerAngles(mCallback->GetCurrValueQuat());
            mTempString = AZStd::string::format("Abs. Rotation X: %.3f, Y: %.3f, Z: %.3f", MCore::Math::RadiansToDegrees(currRot.GetX() + MCore::Math::epsilon), MCore::Math::RadiansToDegrees(currRot.GetY() + MCore::Math::epsilon), MCore::Math::RadiansToDegrees(currRot.GetZ() + MCore::Math::epsilon));
            renderUtil->RenderText(10, 10, mTempString.c_str(), ManipulatorColors::mSelectionColor, 9.0f);
        }

        // if the rotation has been changed draw the current direction of the rotation
        if (mRotation.GetLength() > 0.0f)
        {
            // render text with the rotation values of the axes
            float radius = (mMode == ROTATE_CAMROLL) ? mOuterRadius : mInnerRadius;
            mTempString = AZStd::string::format("[%.2f, %.2f, %.2f]", MCore::Math::RadiansToDegrees(mRotation.GetX()), MCore::Math::RadiansToDegrees(mRotation.GetY()), MCore::Math::RadiansToDegrees(mRotation.GetZ()));
            //String rotationValues = String() = AZStd::string::format("[%.2f, %.2f, %.2f]", camera->GetPosition().x, camera->GetPosition().y, camera->GetPosition().z);
            AZ::Vector3 textPosition = MCore::Project(mPosition + (upVector * (mOuterRadius + mTextDistance)), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosition.GetX() - 2.9f * mTempString.size(), textPosition.GetY(), mTempString.c_str(), ManipulatorColors::mSelectionColor);

            // mark the click position with a small cube
            AZ::Vector3 clickPosition = mPosition + mClickPosition * radius;

            // calculate the tangent at the click position
            MCore::RGBAColor rotDirColorNegative = (mRotation.Dot(mRotationAxis) > 0.0f) ? ManipulatorColors::mSelectionColor : grey;
            MCore::RGBAColor rotDirColorPositive = (mRotation.Dot(mRotationAxis) < 0.0f) ? ManipulatorColors::mSelectionColor : grey;

            // render the tangent directions at the click positions
            AZ::Vector3 tangent = mRotationAxis.Cross(mClickPosition).GetNormalized();
            renderUtil->RenderLine(clickPosition, clickPosition + 1.5f * mAxisSize * tangent, rotDirColorPositive);
            renderUtil->RenderLine(clickPosition, clickPosition - 1.5f * mAxisSize * tangent, rotDirColorNegative);
            renderUtil->RenderCylinder(2.0f * mArrowBaseRadius, 0.0f, 0.5f * mAxisSize, clickPosition + 1.5f * mAxisSize * tangent, tangent, rotDirColorPositive);
            renderUtil->RenderCylinder(2.0f * mArrowBaseRadius, 0.0f, 0.5f * mAxisSize, clickPosition - 1.5f * mAxisSize * tangent, -tangent, rotDirColorNegative);
        }
        else
        {
            if (mName.size() > 0)
            {
                AZ::Vector3 textPosition = MCore::Project(mPosition + (upVector * (mOuterRadius + mTextDistance)), camera->GetViewProjMatrix(), screenWidth, screenHeight);
                renderUtil->RenderText(textPosition.GetX(), textPosition.GetY(), mName.c_str(), ManipulatorColors::mSelectionColor, 11.0f, true);
            }
        }
    }


    // unproject screen coordinates to a ray
    void RotateManipulator::ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);
        MCORE_UNUSED(middleButtonPressed);

        // check if camera has been set
        if (camera == nullptr || mIsVisible == false || (leftButtonPressed && rightButtonPressed))
        {
            return;
        }

        // update the axis visibility flags
        UpdateAxisDirections(camera);

        // get screen mSize
        uint32 screenWidth  = camera->GetScreenWidth();
        uint32 screenHeight = camera->GetScreenHeight();

        // generate rays for the collision detection
        MCore::Ray mousePosRay          = camera->Unproject(mousePosX, mousePosY);
        //MCore::Ray mousePrevPosRay    = camera->Unproject( mousePosX-mouseMovementX, mousePosY-mouseMovementY );
        MCore::Ray camRollRay           = camera->Unproject(screenWidth / 2, screenHeight / 2);
        AZ::Vector3 camRollAxis         = camRollRay.GetDirection();
        mRotationQuat                   = AZ::Quaternion::CreateIdentity();

        // check for the selected axis/plane
        if (mSelectionLocked == false || mMode == ROTATE_NONE)
        {
            // update old rotation of the callback
            if (mCallback)
            {
                mCallback->UpdateOldValues();
            }

            // the intersection variables
            AZ::Vector3 intersectA, intersectB;

            // set rotation mode to rotation around the x axis, if the following intersection conditions are fulfilled
            // innerAABB not hit, outerAABB hit, innerBoundingSphere hit and angle between cameraRollAxis and clickPosition > pi/2
            if ((mousePosRay.Intersects(mXAxisInnerAABB) == false ||
                 (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC && mXAxisVisible)) &&
                mousePosRay.Intersects(mXAxisAABB, &intersectA, &intersectB) &&
                mousePosRay.Intersects(mInnerBoundingSphere) &&
                MCore::Math::ACos(camRollAxis.Dot((intersectA - mPosition).GetNormalized())) > MCore::Math::halfPi)
            {
                mMode = ROTATE_X;
                mRotationAxis = AZ::Vector3(1.0f, 0.0f, 0.0f);
                mClickPosition = (intersectA - mPosition).GetNormalized();
                mClickPosition.SetX(0.0f);
            }

            // set rotation mode to rotation around the y axis, if the following intersection conditions are fulfilled
            // innerAABB not hit, outerAABB hit, innerBoundingSphere hit and angle between cameraRollAxis and clickPosition > pi/2
            else if ((mousePosRay.Intersects(mYAxisInnerAABB) == false ||
                      (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC && mYAxisVisible)) &&
                     mousePosRay.Intersects(mYAxisAABB, &intersectA, &intersectB) && mousePosRay.Intersects(mInnerBoundingSphere) &&
                     MCore::Math::ACos(camRollAxis.Dot((intersectA - mPosition).GetNormalized())) > MCore::Math::halfPi)
            {
                mMode = ROTATE_Y;
                mRotationAxis = AZ::Vector3(0.0f, 1.0f, 0.0f);
                mClickPosition = (intersectA - mPosition).GetNormalized();
                mClickPosition.SetY(0.0f);
            }

            // set rotation mode to rotation around the z axis, if the following intersection conditions are fulfilled
            // innerAABB not hit, outerAABB hit, innerBoundingSphere hit and angle between cameraRollAxis and clickPosition > pi/2
            else if ((mousePosRay.Intersects(mZAxisInnerAABB) == false ||
                      (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_ORTHOGRAPHIC && mZAxisVisible)) &&
                     mousePosRay.Intersects(mZAxisAABB, &intersectA, &intersectB) && mousePosRay.Intersects(mInnerBoundingSphere) &&
                     MCore::Math::ACos(camRollAxis.Dot((intersectA - mPosition).GetNormalized())) > MCore::Math::halfPi)
            {
                mMode = ROTATE_Z;
                mRotationAxis = AZ::Vector3(0.0f, 0.0f, 1.0f);
                mClickPosition = (intersectA - mPosition).GetNormalized();
                mClickPosition.SetZ(0.0f);
            }

            // set rotation mode to rotation around the pitch and yaw axis of the camera,
            // if the inner sphere is hit and none of the previous conditions was fulfilled
            else if (mousePosRay.Intersects(mInnerBoundingSphere, &intersectA, &intersectB))
            {
                mMode = ROTATE_CAMPITCHYAW;

                // set the rotation axis to zero, because no single axis exists in this mode
                mRotationAxis = AZ::Vector3::CreateZero();

                // project the click position onto the plane which is perpendicular to the rotation direction
                MCore::PlaneEq rotationPlane(camRollAxis, mPosition);
                mClickPosition = (rotationPlane.Project(intersectA - mPosition)).GetNormalized();
            }

            // set rotation mode to rotation around the roll axis of the camera,
            // if the outer sphere is hit and none of the previous conditions was fulfilled
            else if (mousePosRay.Intersects(mOuterBoundingSphere, &intersectA, &intersectB))
            {
                // set rotation mode to rotate around the view axis
                mMode = ROTATE_CAMROLL;

                // set the rotation axis to the look at ray direction
                mRotationAxis = camRollRay.GetDirection();

                // project the click position onto the plane which is perpendicular to the rotation direction
                MCore::PlaneEq rotationPlane(mRotationAxis, AZ::Vector3::CreateZero());
                mClickPosition = (rotationPlane.Project(intersectA - mPosition)).GetNormalized();
            }

            // no bounding volume is currently hit, therefore do not rotate
            else
            {
                mMode = ROTATE_NONE;
            }
        }

        // set selection lock and current projection mode
        mSelectionLocked = leftButtonPressed;
        mCurrentProjectionMode = camera->GetProjectionMode();

        // reset the gizmo if no rotation mode is selected
        if (mSelectionLocked == false || mMode == ROTATE_NONE)
        {
            mRotation = AZ::Vector3::CreateZero();
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
        if (mMode == ROTATE_CAMPITCHYAW)
        {
            // the yaw axis of the camera view
            MCore::Ray camYawRay = camera->Unproject(screenWidth / 2, screenHeight / 2 - 10);

            // calculate the plane perpendicular to the view rotation axis
            MCore::PlaneEq rotationPlane(camRollAxis, mPosition);

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
            AZ::Vector3 projectedCenter          = MCore::Project(mPosition, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projectedClickPosYaw     = MCore::Project(mPosition - leftVector, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projectedClickPosPitch   = MCore::Project(mPosition - upVector, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projDirClickPosYaw       = (projectedClickPosYaw - projectedCenter).GetNormalized();
            AZ::Vector3 projDirClickPosPitch     = (projectedClickPosPitch - projectedCenter).GetNormalized();

            // calculate the angle between mouse movement and projected rotation axis
            float angleYaw      = projDirClickPosYaw.Dot(mouseMovementV3) * mScalingFactor * movementLength * 0.00005f;
            float anglePitch    = projDirClickPosPitch.Dot(mouseMovementV3) * mScalingFactor * movementLength * 0.00005f;

            // perform rotation arround the cam yaw and pitch axis
            AZ::Quaternion rotation = MCore::CreateFromAxisAndAngle(upVector, -angleYaw);
            rotation = rotation * MCore::CreateFromAxisAndAngle(leftVector, anglePitch);

            // set euler angles of the rotation variable
            mRotation += MCore::AzQuaternionToEulerAngles(rotation);
            mRotationQuat = rotation;
        }
        else
        {
            /*
            // HINT: uncommented stuff is the exact rotation, used in maya
            // generate current translation plane and calculate mouse intersections
            MCore::PlaneEq movementPlane( mRotationAxis, mPosition );
            Vector3 mousePosIntersect, mousePrevPosIntersect;
            mousePosRay.Intersects( movementPlane, &mousePosIntersect );
            mousePrevPosRay.Intersects( movementPlane, &mousePrevPosIntersect );

            // normalize the intersection points, as only the angle between them is needed
            mousePosIntersect = (mousePosIntersect - mPosition).Normalize();
            mousePrevPosIntersect = (mousePrevPosIntersect - mPosition).Normalize();

            // distance of the mouse intersections is the actual movement on the plane
            float angleSign = MCore::Sgn((mRotationAxis.Cross(mousePosIntersect-mousePrevPosIntersect)).Dot(mousePosIntersect));
            float angle = MCore::Math::ACos( (mousePrevPosIntersect).Dot((mousePosIntersect)) ) * angleSign;

            mRotation += mRotationAxis * angle;
            mRotation = Vector3( MCore::Clamp(mRotation.x, -Math::twoPi, Math::twoPi), MCore::Clamp(mRotation.y, -Math::twoPi, Math::twoPi), MCore::Clamp(mRotation.z, -Math::twoPi, Math::twoPi) );
            mRotationQuat = Quaternion( mRotationAxis, MCore::Clamp(-angle, -Math::twoPi, Math::twoPi) );
            */
            // calculate the projected center and click position to determine the rotation angle
            AZ::Vector3 tangent          = (mRotationAxis.Cross(mClickPosition)).GetNormalized();
            AZ::Vector3 projectedCenter  = MCore::Project(mPosition, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projectedClickPos = MCore::Project(mPosition - tangent, camera->GetViewProjMatrix(), screenWidth, screenHeight);
            AZ::Vector3 projDirClickPos  = (projectedClickPos - projectedCenter);

            // calculate the angle between mouse movement and projected rotation axis
            //float angle = Math::DegreesToRadians(Math::Floor(Math::RadiansToDegrees((projDirClickPos.Dot( mouseMovementV3 ) * mScalingFactor * 0.00002f) * movementLength)));
            float angle = MCore::Math::DegreesToRadians(MCore::Sgn<float>(projDirClickPos.Dot(mouseMovementV3)) * 0.2f * MCore::Math::Floor(movementLength + 0.5f));

            // adjust rotation
            mRotation += mRotationAxis * angle;
            //mRotation = Vector3( MCore::Clamp(mRotation.x, -Math::twoPi, Math::twoPi), MCore::Clamp(mRotation.y, -Math::twoPi, Math::twoPi), MCore::Clamp(mRotation.z, -Math::twoPi, Math::twoPi) );
            mRotationQuat = AZ::Quaternion::CreateFromAxisAngle(mRotationAxis, MCore::Math::FMod(-angle, MCore::Math::twoPi));
            mRotationQuat.Normalize();
        }

        // update the callback
        if (mCallback)
        {
            const AZ::Quaternion curRot = mCallback->GetCurrValueQuat();
            const AZ::Quaternion newRot = (curRot * mRotationQuat).GetNormalized();
            mCallback->Update(newRot);
        }
    }
} // namespace MCommon
