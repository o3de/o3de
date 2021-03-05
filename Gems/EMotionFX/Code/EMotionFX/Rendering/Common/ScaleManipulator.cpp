/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "ScaleManipulator.h"


namespace MCommon
{
    // constructor
    ScaleManipulator::ScaleManipulator(float scalingFactor, bool isVisible)
        : TransformationManipulator(scalingFactor, isVisible)
    {
        // set the initial values
        mMode               = SCALE_NONE;
        mSelectionLocked    = false;
        mCallback           = nullptr;
        mPosition           = AZ::Vector3::CreateZero();
        mScaleDirection     = AZ::Vector3::CreateZero();
        mScale              = AZ::Vector3::CreateZero();
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

        mSize           = mScalingFactor;
        mScaledSize     = AZ::Vector3(mSize, mSize, mSize) + AZ::Vector3(MCore::Max(float(mScale.GetX()), -mSize), MCore::Max(float(mScale.GetY()), -mSize), MCore::Max(float(mScale.GetZ()), -mSize));
        mDiagScale      = 0.5f;
        mArrowLength    = mSize / 10.0f;
        mBaseRadius     = mSize / 15.0f;

        // positions for the plane selectors
        mFirstPlaneSelectorPos  = mScaledSize * 0.3f;
        mSecPlaneSelectorPos    = mScaledSize * 0.6f;

        // set the bounding volumes of the axes selection
        mXAxisAABB.SetMax(mPosition + mSignX * AZ::Vector3(mScaledSize.GetX() + mArrowLength, mBaseRadius, mBaseRadius));
        mXAxisAABB.SetMin(mPosition - mSignX * AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));
        mYAxisAABB.SetMax(mPosition + mSignY * AZ::Vector3(mBaseRadius, mScaledSize.GetY() + mArrowLength, mBaseRadius));
        mYAxisAABB.SetMin(mPosition - mSignY * AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));
        mZAxisAABB.SetMax(mPosition + mSignZ * AZ::Vector3(mBaseRadius, mBaseRadius, mScaledSize.GetZ() + mArrowLength));
        mZAxisAABB.SetMin(mPosition - mSignZ * AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));

        // set bounding volumes for the plane selectors
        mXYPlaneAABB.SetMax(mPosition + AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, mSecPlaneSelectorPos.GetY() * mSignY, mBaseRadius * mSignZ));
        mXYPlaneAABB.SetMin(mPosition + 0.3f * AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, mSecPlaneSelectorPos.GetY() * mSignY, 0) - AZ::Vector3(mBaseRadius * mSignX, mBaseRadius * mSignY, mBaseRadius * mSignZ));
        mXZPlaneAABB.SetMax(mPosition + AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, mBaseRadius * mSignY, mSecPlaneSelectorPos.GetZ() * mSignZ));
        mXZPlaneAABB.SetMin(mPosition + 0.3f * AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, 0, mSecPlaneSelectorPos.GetZ() * mSignZ) - AZ::Vector3(mBaseRadius * mSignX, mBaseRadius * mSignY, mBaseRadius * mSignZ));
        mYZPlaneAABB.SetMax(mPosition + AZ::Vector3(mBaseRadius * mSignX, mSecPlaneSelectorPos.GetY() * mSignY, mSecPlaneSelectorPos.GetZ() * mSignZ));
        mYZPlaneAABB.SetMin(mPosition + 0.3f * AZ::Vector3(0, mSecPlaneSelectorPos.GetY() * mSignY, mSecPlaneSelectorPos.GetZ() * mSignZ) - AZ::Vector3(mBaseRadius * mSignX, mBaseRadius * mSignY, mBaseRadius * mSignZ));

        // set bounding volume for the box selector
        mXYZBoxAABB.SetMin(mPosition - AZ::Vector3(mBaseRadius * mSignX, mBaseRadius * mSignY, mBaseRadius * mSignZ));
        mXYZBoxAABB.SetMax(mPosition + mDiagScale * AZ::Vector3(mFirstPlaneSelectorPos.GetX() * mSignX, mFirstPlaneSelectorPos.GetY() * mSignY, mFirstPlaneSelectorPos.GetZ() * mSignZ));
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

        mSignX = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) >= MCore::Math::halfPi - MCore::Math::epsilon) ? 1.0f : -1.0f;
        mSignY = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) >= MCore::Math::halfPi - MCore::Math::epsilon) ? 1.0f : -1.0f;
        mSignZ = (MCore::Math::ACos(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) >= MCore::Math::halfPi - MCore::Math::epsilon) ? 1.0f : -1.0f;

        // determine the axis visibility, to disable movement for invisible axes
        mXAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        mYAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        mZAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
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
        if (mouseRay.Intersects(mXAxisAABB) ||
            mouseRay.Intersects(mYAxisAABB) ||
            mouseRay.Intersects(mZAxisAABB) ||
            mouseRay.Intersects(mXYPlaneAABB) ||
            mouseRay.Intersects(mXZPlaneAABB) ||
            mouseRay.Intersects(mYZPlaneAABB) ||
            mouseRay.Intersects(mXYZBoxAABB))
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
        if (renderUtil == nullptr || camera == nullptr || mIsVisible == false)
        {
            return;
        }

        // set mSize variables for the gizmo
        const uint32 screenWidth        = camera->GetScreenWidth();
        const uint32 screenHeight       = camera->GetScreenHeight();

        // update the camera axis directions
        UpdateAxisDirections(camera);

        // set color for the axes, depending on the selection (TODO: maybe put these into the constructor.)
        MCore::RGBAColor xAxisColor     = (mMode == SCALE_XYZ || mMode == SCALE_X || mMode == SCALE_XY || mMode == SCALE_XZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mRed;
        MCore::RGBAColor yAxisColor     = (mMode == SCALE_XYZ || mMode == SCALE_Y || mMode == SCALE_XY || mMode == SCALE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mGreen;
        MCore::RGBAColor zAxisColor     = (mMode == SCALE_XYZ || mMode == SCALE_Z || mMode == SCALE_XZ || mMode == SCALE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mBlue;
        MCore::RGBAColor xyPlaneColorX  = (mMode == SCALE_XYZ || mMode == SCALE_XY) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mRed;
        MCore::RGBAColor xyPlaneColorY  = (mMode == SCALE_XYZ || mMode == SCALE_XY) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mGreen;
        MCore::RGBAColor xzPlaneColorX  = (mMode == SCALE_XYZ || mMode == SCALE_XZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mRed;
        MCore::RGBAColor xzPlaneColorZ  = (mMode == SCALE_XYZ || mMode == SCALE_XZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mBlue;
        MCore::RGBAColor yzPlaneColorY  = (mMode == SCALE_XYZ || mMode == SCALE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mGreen;
        MCore::RGBAColor yzPlaneColorZ  = (mMode == SCALE_XYZ || mMode == SCALE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mBlue;

        // the x axis with cube at the line end
        AZ::Vector3 firstPlanePosX   = mPosition + mSignX * AZ::Vector3(mFirstPlaneSelectorPos.GetX(), 0.0f, 0.0f);
        AZ::Vector3 secPlanePosX = mPosition + mSignX * AZ::Vector3(mSecPlaneSelectorPos.GetX(), 0.0f, 0.0f);
        if (mXAxisVisible)
        {
            renderUtil->RenderLine(mPosition, mPosition + mSignX * AZ::Vector3(mScaledSize.GetX() + 0.5f * mBaseRadius, 0.0f, 0.0f), xAxisColor);
            //renderUtil->RenderCube( Vector3(mBaseRadius, mBaseRadius, mBaseRadius), mPosition + mSignX * Vector3(mScaledSize.x+mBaseRadius, 0, 0), ManipulatorColors::mRed );
            AZ::Vector3 quadPos = MCore::Project(mPosition + mSignX * AZ::Vector3(mScaledSize.GetX() + mBaseRadius, 0, 0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderBorderedRect(static_cast<int32>(quadPos.GetX() - 2.0f), static_cast<int32>(quadPos.GetX() + 3.0f), static_cast<int32>(quadPos.GetY() - 2.0f), static_cast<int32>(quadPos.GetY() + 3.0f), ManipulatorColors::mRed, ManipulatorColors::mRed);

            // render the plane selector lines
            renderUtil->RenderLine(firstPlanePosX, firstPlanePosX + mDiagScale * (AZ::Vector3(0, mFirstPlaneSelectorPos.GetY() * mSignY, 0) - AZ::Vector3(mFirstPlaneSelectorPos.GetX() * mSignX, 0, 0)), xyPlaneColorX);
            renderUtil->RenderLine(firstPlanePosX, firstPlanePosX + mDiagScale * (AZ::Vector3(0, 0, mFirstPlaneSelectorPos.GetZ() * mSignZ) - AZ::Vector3(mFirstPlaneSelectorPos.GetX() * mSignX, 0, 0)), xzPlaneColorX);
            renderUtil->RenderLine(secPlanePosX, secPlanePosX + mDiagScale * (AZ::Vector3(0, mSecPlaneSelectorPos.GetY() * mSignY, 0) - AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, 0, 0)), xyPlaneColorX);
            renderUtil->RenderLine(secPlanePosX, secPlanePosX + mDiagScale * (AZ::Vector3(0, 0, mSecPlaneSelectorPos.GetZ() * mSignZ) - AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, 0, 0)), xzPlaneColorX);
        }

        // the y axis with cube at the line end
        AZ::Vector3 firstPlanePosY   = mPosition + mSignY * AZ::Vector3(0.0f, mFirstPlaneSelectorPos.GetY(), 0.0f);
        AZ::Vector3 secPlanePosY = mPosition + mSignY * AZ::Vector3(0.0f, mSecPlaneSelectorPos.GetY(), 0.0f);
        if (mYAxisVisible)
        {
            renderUtil->RenderLine(mPosition, mPosition + mSignY * AZ::Vector3(0.0f, mScaledSize.GetY(), 0.0f), yAxisColor);
            //renderUtil->RenderCube( Vector3(mBaseRadius, mBaseRadius, mBaseRadius), mPosition + mSignY * Vector3(0, mScaledSize.y+0.5*mBaseRadius, 0), ManipulatorColors::mGreen );
            AZ::Vector3 quadPos = MCore::Project(mPosition + mSignY * AZ::Vector3(0, mScaledSize.GetY() + 0.5f * mBaseRadius, 0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderBorderedRect(static_cast<int32>(quadPos.GetX() - 2.0f), static_cast<int32>(quadPos.GetX() + 3.0f), static_cast<int32>(quadPos.GetY() - 2.0f), static_cast<int32>(quadPos.GetY() + 3.0f), ManipulatorColors::mGreen, ManipulatorColors::mGreen);

            // render the plane selector lines
            renderUtil->RenderLine(firstPlanePosY, firstPlanePosY + mDiagScale * (AZ::Vector3(mFirstPlaneSelectorPos.GetX() * mSignX, 0, 0) - AZ::Vector3(0, mFirstPlaneSelectorPos.GetY() * mSignY, 0)), xyPlaneColorY);
            renderUtil->RenderLine(firstPlanePosY, firstPlanePosY + mDiagScale * (AZ::Vector3(0, 0, mFirstPlaneSelectorPos.GetZ() * mSignZ) - AZ::Vector3(0, mFirstPlaneSelectorPos.GetY() * mSignY, 0)), yzPlaneColorY);
            renderUtil->RenderLine(secPlanePosY, secPlanePosY + mDiagScale * (AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, 0, 0) - AZ::Vector3(0, mSecPlaneSelectorPos.GetY() * mSignY, 0)), xyPlaneColorY);
            renderUtil->RenderLine(secPlanePosY, secPlanePosY + mDiagScale * (AZ::Vector3(0, 0, mSecPlaneSelectorPos.GetZ() * mSignZ) - AZ::Vector3(0, mSecPlaneSelectorPos.GetY() * mSignY, 0)), yzPlaneColorY);
        }

        // the z axis with cube at the line end
        AZ::Vector3 firstPlanePosZ   = mPosition + mSignZ * AZ::Vector3(0.0f, 0.0f, mFirstPlaneSelectorPos.GetZ());
        AZ::Vector3 secPlanePosZ = mPosition + mSignZ * AZ::Vector3(0.0f, 0.0f, mSecPlaneSelectorPos.GetZ());
        if (mZAxisVisible)
        {
            renderUtil->RenderLine(mPosition, mPosition + mSignZ * AZ::Vector3(0.0f, 0.0f, mScaledSize.GetZ()), zAxisColor);
            //renderUtil->RenderCube( Vector3(mBaseRadius, mBaseRadius, mBaseRadius), mPosition + mSignZ * Vector3(0, 0, mScaledSize.z+0.5*mBaseRadius), ManipulatorColors::mBlue );
            AZ::Vector3 quadPos = MCore::Project(mPosition + mSignZ * AZ::Vector3(0, 0, mScaledSize.GetZ() + 0.5f * mBaseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderBorderedRect(static_cast<int32>(quadPos.GetX() - 2.0f), static_cast<int32>(quadPos.GetX() + 3.0f), static_cast<int32>(quadPos.GetY() - 2.0f), static_cast<int32>(quadPos.GetY() + 3.0f), ManipulatorColors::mBlue, ManipulatorColors::mBlue);

            // render the plane selector lines
            renderUtil->RenderLine(firstPlanePosZ, firstPlanePosZ + mDiagScale * (AZ::Vector3(mFirstPlaneSelectorPos.GetX() * mSignX, 0, 0) - AZ::Vector3(0, 0, mFirstPlaneSelectorPos.GetZ() * mSignZ)), xzPlaneColorZ);
            renderUtil->RenderLine(firstPlanePosZ, firstPlanePosZ + mDiagScale * (AZ::Vector3(0, mFirstPlaneSelectorPos.GetY() * mSignY, 0) - AZ::Vector3(0, 0, mFirstPlaneSelectorPos.GetZ() * mSignZ)), yzPlaneColorZ);
            renderUtil->RenderLine(secPlanePosZ, secPlanePosZ + mDiagScale * (AZ::Vector3(mSecPlaneSelectorPos.GetX() * mSignX, 0, 0) - AZ::Vector3(0, 0, mSecPlaneSelectorPos.GetZ() * mSignZ)), xzPlaneColorZ);
            renderUtil->RenderLine(secPlanePosZ, secPlanePosZ + mDiagScale * (AZ::Vector3(0, mSecPlaneSelectorPos.GetY() * mSignY, 0) - AZ::Vector3(0, 0, mSecPlaneSelectorPos.GetZ() * mSignZ)), yzPlaneColorZ);
        }

        // calculate projected positions for the axis labels and render the text
        AZ::Vector3 textPosY = MCore::Project(mPosition + mSignY * AZ::Vector3(0.0, mScaledSize.GetY() + mArrowLength + mBaseRadius, -mBaseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
        AZ::Vector3 textPosX = MCore::Project(mPosition + mSignX * AZ::Vector3(mScaledSize.GetX() + mArrowLength + mBaseRadius, -mBaseRadius, 0.0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
        AZ::Vector3 textPosZ = MCore::Project(mPosition + mSignZ * AZ::Vector3(0.0, mBaseRadius, mScaledSize.GetZ() + mArrowLength + mBaseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
        renderUtil->RenderText(textPosX.GetX(), textPosX.GetY(), "X", xAxisColor);
        renderUtil->RenderText(textPosY.GetX(), textPosY.GetY(), "Y", yAxisColor);
        renderUtil->RenderText(textPosZ.GetX(), textPosZ.GetY(), "Z", zAxisColor);

        // Render the triangles for plane selection
        if (mMode == SCALE_XY && mXAxisVisible && mYAxisVisible)
        {
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosX, secPlanePosY, ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosY, firstPlanePosY, ManipulatorColors::mSelectionColorDarker);
        }
        else if (mMode == SCALE_XZ && mXAxisVisible && mZAxisVisible)
        {
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosX, secPlanePosZ, ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(firstPlanePosX, secPlanePosZ, firstPlanePosZ, ManipulatorColors::mSelectionColorDarker);
        }
        else if (mMode == SCALE_YZ && mYAxisVisible && mZAxisVisible)
        {
            renderUtil->RenderTriangle(firstPlanePosZ, secPlanePosZ, secPlanePosY, ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(firstPlanePosZ, secPlanePosY, firstPlanePosY, ManipulatorColors::mSelectionColorDarker);
        }
        else if (mMode == SCALE_XYZ)
        {
            renderUtil->RenderTriangle(firstPlanePosX, firstPlanePosY, firstPlanePosZ, ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(mPosition, firstPlanePosX, firstPlanePosZ, ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(mPosition, firstPlanePosX, firstPlanePosY, ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(mPosition, firstPlanePosY, firstPlanePosZ, ManipulatorColors::mSelectionColorDarker);
        }

        // check if callback exists
        if (mCallback == nullptr)
        {
            return;
        }

        // render the current scale factor in percent if gizmo is hit
        if (mMode != SCALE_NONE)
        {
            const AZ::Vector3& currScale = mCallback->GetCurrValueVec();
            mTempString = AZStd::string::format("Abs. Scale X: %.3f, Y: %.3f, Z: %.3f", MCore::Max(float(currScale.GetX()), 0.0f), MCore::Max(float(currScale.GetY()), 0.0f), MCore::Max(float(currScale.GetZ()), 0.0f));
            renderUtil->RenderText(10, 10, mTempString.c_str(), ManipulatorColors::mSelectionColor, 9.0f);
        }

        // calculate the position offset of the relative text
        float yOffset = (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_PERSPECTIVE) ? 80.0f : 50.0f;

        // text position relative scaling or name displayed below the gizmo
        AZ::Vector3 textPos  = MCore::Project(mPosition + (mSize * AZ::Vector3(mSignX, mSignY, mSignZ) / 3.0f), camera->GetViewProjMatrix(), camera->GetScreenWidth(), camera->GetScreenHeight());

        // render the relative scale when moving
        if (mSelectionLocked && mMode != SCALE_NONE)
        {
            // calculate the scale factor
            AZ::Vector3 scaleFactor = ((AZ::Vector3(mSize, mSize, mSize) + mScale) / (float)mSize);

            // render the scaling value below the gizmo
            mTempString = AZStd::string::format("X: %.3f, Y: %.3f, Z: %.3f", MCore::Max(float(scaleFactor.GetX()), 0.0f), MCore::Max(float(scaleFactor.GetY()), 0.0f), MCore::Max(float(scaleFactor.GetZ()), 0.0f));
            renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, mTempString.c_str(), ManipulatorColors::mSelectionColor, 9.0f, true);
        }
        else
        {
            if (mName.size() > 0)
            {
                renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, mName.c_str(), ManipulatorColors::mSelectionColor, 9.0f, true);
            }
        }
    }


    // unproject screen coordinates to a mousePosRay
    void ScaleManipulator::ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);
        MCORE_UNUSED(middleButtonPressed);

        // check if camera has been set
        if (camera == nullptr || mIsVisible == false || (leftButtonPressed && rightButtonPressed))
        {
            return;
        }

        // get screen mSize
        const uint32 screenWidth    = camera->GetScreenWidth();
        const uint32 screenHeight   = camera->GetScreenHeight();

        // update the axis directions with respect to the current camera
        UpdateAxisDirections(camera);

        // generate ray for the collision detection
        MCore::Ray mousePosRay      = camera->Unproject(mousePosX, mousePosY);
        MCore::Ray mousePrevPosRay  = camera->Unproject(mousePosX - mouseMovementX, mousePosY - mouseMovementY);

        // check for the selected axis/plane
        if (mSelectionLocked == false || mMode == SCALE_NONE)
        {
            // update old rotation of the callback
            if (mCallback)
            {
                mCallback->UpdateOldValues();
            }

            // handle different scale cases depending on the bounding volumes
            if (mousePosRay.Intersects(mXYZBoxAABB))
            {
                mMode   = SCALE_XYZ;
                mScaleDirection = AZ::Vector3(mSignX, mSignY, mSignZ);
            }
            else if (mousePosRay.Intersects(mXYPlaneAABB) && mXAxisVisible && mYAxisVisible)
            {
                mMode = SCALE_XY;
                mScaleDirection = AZ::Vector3(mSignX, mSignY, 0.0f);
            }
            else if (mousePosRay.Intersects(mXZPlaneAABB) && mXAxisVisible && mZAxisVisible)
            {
                mMode = SCALE_XZ;
                mScaleDirection = AZ::Vector3(mSignX, 0.0f, mSignZ);
            }
            else if (mousePosRay.Intersects(mYZPlaneAABB) && mYAxisVisible && mZAxisVisible)
            {
                mMode = SCALE_YZ;
                mScaleDirection = AZ::Vector3(0.0f, mSignY, mSignZ);
            }
            else if (mousePosRay.Intersects(mXAxisAABB) && mXAxisVisible)
            {
                mMode = SCALE_X;
                mScaleDirection = AZ::Vector3(mSignX, 0.0f, 0.0f);
            }
            else if (mousePosRay.Intersects(mYAxisAABB) && mYAxisVisible)
            {
                mMode = SCALE_Y;
                mScaleDirection = AZ::Vector3(0.0f, mSignY, 0.0f);
            }
            else if (mousePosRay.Intersects(mZAxisAABB) && mZAxisVisible)
            {
                mMode = SCALE_Z;
                mScaleDirection = AZ::Vector3(0.0f, 0.0f, mSignZ);
            }
            else
            {
                mMode = SCALE_NONE;
            }
        }

        // set selection lock
        mSelectionLocked = leftButtonPressed;

        // move the gizmo
        if (mSelectionLocked == false || mMode == SCALE_NONE)
        {
            mScale = AZ::Vector3::CreateZero();
            return;
        }

        // init the scale change to zero
        AZ::Vector3 scaleChange = AZ::Vector3::CreateZero();

        // calculate the movement of the mouse on a plane located at the gizmo position
        // and perpendicular to the camera direction
        MCore::Ray camRay = camera->Unproject(screenWidth / 2, screenHeight / 2);
        MCore::PlaneEq movementPlane(camRay.GetDirection(), mPosition);

        // calculate the intersection points of the mouse positions with the previously calculated plane
        AZ::Vector3 mousePosIntersect, mousePrevPosIntersect;
        mousePosRay.Intersects(movementPlane, &mousePosIntersect);
        mousePrevPosRay.Intersects(movementPlane, &mousePrevPosIntersect);

        // project the mouse movement onto the scale axis
        scaleChange = (mScaleDirection * mScaleDirection.Dot(mousePosIntersect) - mScaleDirection * mScaleDirection.Dot(mousePrevPosIntersect));

        // update the scale of the gizmo
        scaleChange = AZ::Vector3(scaleChange.GetX() * mSignX, scaleChange.GetY() * mSignY, scaleChange.GetZ() * mSignZ);
        mScale += scaleChange;

        // update the callback actor instance
        if (mCallback)
        {
            AZ::Vector3 updateScale = (AZ::Vector3(mSize, mSize, mSize) + mScale) / (float)mSize;
            mCallback->Update(updateScale);
        }
    }
} // namespace MCommon
