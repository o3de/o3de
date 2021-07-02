/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "TranslateManipulator.h"


namespace MCommon
{
    // constructor
    TranslateManipulator::TranslateManipulator(float scalingFactor, bool isVisible)
        : TransformationManipulator(scalingFactor, isVisible)
    {
        // set the initial values
        mMode       = TRANSLATE_NONE;
        mCallback   = nullptr;
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
        mSize               = mScalingFactor;
        mArrowLength        = mSize / 5.0f;
        mBaseRadius         = mSize / 20.0f;
        mPlaneSelectorPos   = mSize / 2;

        // set the bounding volumes of the axes selection
        mXAxisAABB.SetMax(mPosition + AZ::Vector3(mSize + mArrowLength, mBaseRadius, mBaseRadius));
        mXAxisAABB.SetMin(mPosition - AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));
        mYAxisAABB.SetMax(mPosition + AZ::Vector3(mBaseRadius, mSize + mArrowLength, mBaseRadius));
        mYAxisAABB.SetMin(mPosition - AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));
        mZAxisAABB.SetMax(mPosition + AZ::Vector3(mBaseRadius, mBaseRadius, mSize + mArrowLength));
        mZAxisAABB.SetMin(mPosition - AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));

        // set bounding volumes for the plane selectors
        mXYPlaneAABB.SetMax(mPosition + AZ::Vector3(mPlaneSelectorPos, mPlaneSelectorPos, mBaseRadius));
        mXYPlaneAABB.SetMin(mPosition + 0.3f * AZ::Vector3(mPlaneSelectorPos, mPlaneSelectorPos, 0) - AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));
        mXZPlaneAABB.SetMax(mPosition + AZ::Vector3(mPlaneSelectorPos, mBaseRadius, mPlaneSelectorPos));
        mXZPlaneAABB.SetMin(mPosition + 0.3f * AZ::Vector3(mPlaneSelectorPos, 0, mPlaneSelectorPos) - AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));
        mYZPlaneAABB.SetMax(mPosition + AZ::Vector3(mBaseRadius, mPlaneSelectorPos, mPlaneSelectorPos));
        mYZPlaneAABB.SetMin(mPosition + 0.3f * AZ::Vector3(0, mPlaneSelectorPos, mPlaneSelectorPos) - AZ::Vector3(mBaseRadius, mBaseRadius, mBaseRadius));
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
        mXAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(1.0f, 0.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        mYAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 1.0f, 0.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
        mZAxisVisible = (MCore::InRange(MCore::Math::Abs(camDir.Dot(AZ::Vector3(0.0f, 0.0f, 1.0f))) - 1.0f, -MCore::Math::epsilon, MCore::Math::epsilon) == false);
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
        if (mouseRay.Intersects(mXAxisAABB) || mouseRay.Intersects(mYAxisAABB) || mouseRay.Intersects(mZAxisAABB) ||
            mouseRay.Intersects(mXYPlaneAABB) || mouseRay.Intersects(mXZPlaneAABB) || mouseRay.Intersects(mYZPlaneAABB))
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
        if (renderUtil == nullptr || camera == nullptr || mIsVisible == false)
        {
            return;
        }

        // get the screen dimensions
        const uint32 screenWidth        = camera->GetScreenWidth();
        const uint32 screenHeight       = camera->GetScreenHeight();

        // update the axis visibility flags
        UpdateAxisVisibility(camera);

        // set color for the axes, depending on the selection
        MCore::RGBAColor xAxisColor     = (mMode == TRANSLATE_X || mMode == TRANSLATE_XY || mMode == TRANSLATE_XZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mRed;
        MCore::RGBAColor yAxisColor     = (mMode == TRANSLATE_Y || mMode == TRANSLATE_XY || mMode == TRANSLATE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mGreen;
        MCore::RGBAColor zAxisColor     = (mMode == TRANSLATE_Z || mMode == TRANSLATE_XZ || mMode == TRANSLATE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mBlue;
        MCore::RGBAColor xyPlaneColorX  = (mMode == TRANSLATE_XY) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mRed;
        MCore::RGBAColor xyPlaneColorY  = (mMode == TRANSLATE_XY) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mGreen;
        MCore::RGBAColor xzPlaneColorX  = (mMode == TRANSLATE_XZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mRed;
        MCore::RGBAColor xzPlaneColorZ  = (mMode == TRANSLATE_XZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mBlue;
        MCore::RGBAColor yzPlaneColorY  = (mMode == TRANSLATE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mGreen;
        MCore::RGBAColor yzPlaneColorZ  = (mMode == TRANSLATE_YZ) ? ManipulatorColors::mSelectionColor : ManipulatorColors::mBlue;

        if (mXAxisVisible)
        {
            // the x axis consisting of a line, cylinder and plane selectors
            renderUtil->RenderLine(mPosition, mPosition + AZ::Vector3(mSize, 0.0, 0.0), xAxisColor);
            renderUtil->RenderCylinder(mBaseRadius, 0, mArrowLength, mPosition + AZ::Vector3(mSize, 0, 0), AZ::Vector3(1, 0, 0), ManipulatorColors::mRed);
            renderUtil->RenderLine(mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0, 0.0), mPosition + AZ::Vector3(mPlaneSelectorPos, mPlaneSelectorPos, 0.0), xyPlaneColorX);
            renderUtil->RenderLine(mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0, 0.0), mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0, mPlaneSelectorPos), xzPlaneColorX);

            // render the axis label for the x axis
            AZ::Vector3 textPosX = MCore::Project(mPosition + AZ::Vector3(mSize + mArrowLength + mBaseRadius, -mBaseRadius, 0.0), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosX.GetX(), textPosX.GetY(), "X", xAxisColor);
        }

        if (mYAxisVisible)
        {
            // the y axis consisting of a line, cylinder and plane selectors
            renderUtil->RenderLine(mPosition, mPosition + AZ::Vector3(0.0, mSize, 0.0), yAxisColor);
            renderUtil->RenderCylinder(mBaseRadius, 0, mArrowLength, mPosition + AZ::Vector3(0, mSize, 0), AZ::Vector3(0, 1, 0), ManipulatorColors::mGreen);
            renderUtil->RenderLine(mPosition + AZ::Vector3(0.0, mPlaneSelectorPos, 0.0), mPosition + AZ::Vector3(mPlaneSelectorPos, mPlaneSelectorPos, 0.0), xyPlaneColorY);
            renderUtil->RenderLine(mPosition + AZ::Vector3(0.0, mPlaneSelectorPos, 0.0), mPosition + AZ::Vector3(0.0, mPlaneSelectorPos, mPlaneSelectorPos), yzPlaneColorY);

            // render the axis label for the y axis
            AZ::Vector3 textPosY = MCore::Project(mPosition + AZ::Vector3(0.0, mSize + mArrowLength + mBaseRadius, -mBaseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosY.GetX(), textPosY.GetY(), "Y", yAxisColor);
        }

        if (mZAxisVisible)
        {
            // the z axis consisting of a line, cylinder and plane selectors
            renderUtil->RenderLine(mPosition, mPosition + AZ::Vector3(0.0, 0.0, mSize), zAxisColor);
            renderUtil->RenderCylinder(mBaseRadius, 0, mArrowLength, mPosition + AZ::Vector3(0, 0, mSize), AZ::Vector3(0, 0, 1), ManipulatorColors::mBlue);
            renderUtil->RenderLine(mPosition + AZ::Vector3(0.0, 0.0, mPlaneSelectorPos), mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0, mPlaneSelectorPos), xzPlaneColorZ);
            renderUtil->RenderLine(mPosition + AZ::Vector3(0.0, 0.0, mPlaneSelectorPos), mPosition + AZ::Vector3(0.0, mPlaneSelectorPos, mPlaneSelectorPos), yzPlaneColorZ);

            // render the axis label for the z axis
            AZ::Vector3 textPosZ = MCore::Project(mPosition + AZ::Vector3(0.0, mBaseRadius, mSize + mArrowLength + mBaseRadius), camera->GetViewProjMatrix(), screenWidth, screenHeight);
            renderUtil->RenderText(textPosZ.GetX(), textPosZ.GetY(), "Z", zAxisColor);
        }

        // draw transparent quad for the plane selectors
        if (mMode == TRANSLATE_XY)
        {
            renderUtil->RenderTriangle(mPosition, mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0f, 0.0f), mPosition + AZ::Vector3(mPlaneSelectorPos, mPlaneSelectorPos, 0.0f), ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(mPosition, mPosition + AZ::Vector3(mPlaneSelectorPos, mPlaneSelectorPos, 0.0f), mPosition + AZ::Vector3(0.0f, mPlaneSelectorPos, 0.0f), ManipulatorColors::mSelectionColorDarker);
        }
        else if (mMode == TRANSLATE_XZ)
        {
            renderUtil->RenderTriangle(mPosition, mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0f, 0.0f), mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0f, mPlaneSelectorPos), ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(mPosition, mPosition + AZ::Vector3(mPlaneSelectorPos, 0.0f, mPlaneSelectorPos), mPosition + AZ::Vector3(0.0f, 0.0f, mPlaneSelectorPos), ManipulatorColors::mSelectionColorDarker);
        }
        else if (mMode == TRANSLATE_YZ)
        {
            renderUtil->RenderTriangle(mPosition + AZ::Vector3(0.0f, 0.0f, mPlaneSelectorPos), mPosition, mPosition + AZ::Vector3(0.0f, mPlaneSelectorPos, 0.0f), ManipulatorColors::mSelectionColorDarker);
            renderUtil->RenderTriangle(mPosition + AZ::Vector3(0.0f, mPlaneSelectorPos, 0.0f), mPosition + AZ::Vector3(0.0f, mPlaneSelectorPos, mPlaneSelectorPos), mPosition + AZ::Vector3(0.0f, 0.0f, mPlaneSelectorPos), ManipulatorColors::mSelectionColorDarker);
        }

        // render the relative position when moving
        if (mCallback)
        {
            // calculate the y-offset of the text position
            AZ::Vector3 deltaPos = GetPosition() - mCallback->GetOldValueVec();
            float yOffset       = (camera->GetProjectionMode() == MCommon::Camera::PROJMODE_PERSPECTIVE) ? 60.0f * ((float)screenHeight / 720.0f) : 40.0f;

            // render the relative movement
            AZ::Vector3 textPos  = MCore::Project(mPosition + (AZ::Vector3(mSize, mSize, mSize) / 3.0f), camera->GetViewProjMatrix(), camera->GetScreenWidth(), camera->GetScreenHeight());

            // render delta position of the gizmo of the name if not dragging at the moment
            if (mSelectionLocked && mMode != TRANSLATE_NONE)
            {
                mTempString = AZStd::string::format("X: %.3f, Y: %.3f, Z: %.3f", static_cast<float>(deltaPos.GetX()), static_cast<float>(deltaPos.GetY()), static_cast<float>(deltaPos.GetZ()));
                renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, mTempString.c_str(), ManipulatorColors::mSelectionColor, 9.0f, true);
            }
            else
            {
                renderUtil->RenderText(textPos.GetX(), textPos.GetY() + yOffset, mName.c_str(), ManipulatorColors::mSelectionColor, 9.0f, true);
            }
        }

        // render aabbs (for debug issues. remove this)
        /*      renderUtil->RenderAABB( mXAxisAABB, ManipulatorColors::mSelectionColor, true );
                renderUtil->RenderAABB( mYAxisAABB, ManipulatorColors::mSelectionColor, true );
                renderUtil->RenderAABB( mZAxisAABB, ManipulatorColors::mSelectionColor, true );
        */

        // render the absolute position of the gizmo/actor instance
        if (mMode != TRANSLATE_NONE)
        {
            const AZ::Vector3 offsetPos = GetPosition();
            mTempString = AZStd::string::format("Abs Pos X: %.3f, Y: %.3f, Z: %.3f", static_cast<float>(offsetPos.GetX()), static_cast<float>(offsetPos.GetY()), static_cast<float>(offsetPos.GetZ()));
            renderUtil->RenderText(10, 10, mTempString.c_str(), ManipulatorColors::mSelectionColor, 9.0f);
        }
    }


    // unproject screen coordinates to a ray
    void TranslateManipulator::ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags)
    {
        MCORE_UNUSED(keyboardKeyFlags);
        MCORE_UNUSED(middleButtonPressed);

        // check if camera has been set
        if (camera == nullptr || mIsVisible == false || (leftButtonPressed && rightButtonPressed))
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
        if (mSelectionLocked == false || mMode == TRANSLATE_NONE)
        {
            // update old values of the callback
            if (mCallback)
            {
                mCallback->UpdateOldValues();
            }

            // handle different translation modes
            if (mousePosRay.Intersects(mXYPlaneAABB) && mXAxisVisible && mYAxisVisible)
            {
                mMovementDirection = AZ::Vector3(1.0f, 1.0f, 0.0f);
                mMovementPlaneNormal = AZ::Vector3(0.0f, 0.0f, 1.0f);
                mMode = TRANSLATE_XY;
            }
            else if (mousePosRay.Intersects(mXZPlaneAABB) && mXAxisVisible && mZAxisVisible)
            {
                mMovementDirection = AZ::Vector3(1.0f, 0.0f, 1.0f);
                mMovementPlaneNormal = AZ::Vector3(0.0f, 1.0f, 0.0f);
                mMode = TRANSLATE_XZ;
            }
            else if (mousePosRay.Intersects(mYZPlaneAABB) && mYAxisVisible && mZAxisVisible)
            {
                mMovementDirection = AZ::Vector3(0.0f, 1.0f, 1.0f);
                mMovementPlaneNormal = AZ::Vector3(1.0f, 0.0f, 0.0f);
                mMode = TRANSLATE_YZ;
            }
            else if (mousePosRay.Intersects(mXAxisAABB) && mXAxisVisible)
            {
                mMovementDirection = AZ::Vector3(1.0f, 0.0f, 0.0f);
                mMovementPlaneNormal = AZ::Vector3(0.0f, 1.0f, 1.0f).GetNormalized();
                mMode = TRANSLATE_X;
            }
            else if (mousePosRay.Intersects(mYAxisAABB) && mYAxisVisible)
            {
                mMovementDirection = AZ::Vector3(0.0f, 1.0f, 0.0f);
                mMovementPlaneNormal = AZ::Vector3(1.0f, 0.0f, 1.0f).GetNormalized();
                mMode = TRANSLATE_Y;
            }
            else if (mousePosRay.Intersects(mZAxisAABB) && mZAxisVisible)
            {
                mMovementDirection = AZ::Vector3(0.0f, 0.0f, 1.0f);
                mMovementPlaneNormal = AZ::Vector3(1.0f, 1.0f, 0.0f).GetNormalized();
                mMode = TRANSLATE_Z;
            }
            else
            {
                mMode = TRANSLATE_NONE;
            }
        }

        // set selection lock
        mSelectionLocked = leftButtonPressed;

        // move the gizmo
        if (mSelectionLocked == false || mMode == TRANSLATE_NONE)
        {
            mMousePosRelative = AZ::Vector3::CreateZero();
            return;
        }

        // init the movement change to zero
        AZ::Vector3 movement = AZ::Vector3::CreateZero();

        // handle plane movement
        if (mMode == TRANSLATE_XY || mMode == TRANSLATE_XZ || mMode == TRANSLATE_YZ)
        {
            // generate current translation plane and calculate mouse intersections
            MCore::PlaneEq movementPlane(mMovementPlaneNormal, mPosition);
            AZ::Vector3 mousePosIntersect, mousePrevPosIntersect;
            mousePosRay.Intersects(movementPlane, &mousePosIntersect);
            mousePrevPosRay.Intersects(movementPlane, &mousePrevPosIntersect);

            // calculate the mouse position relative to the gizmo
            if (MCore::Math::IsFloatZero(MCore::SafeLength(mMousePosRelative)))
            {
                mMousePosRelative = mousePosIntersect - mPosition;
            }

            // distance of the mouse intersections is the actual movement on the plane
            movement = mousePosIntersect - mMousePosRelative;
        }

        // handle axis movement
        // TODO: Fix the infinity bug for axis movement!!!!!
        else
        {
            // calculate the movement of the mouse on a plane located at the gizmo position
            // and perpendicular to the move direction
            AZ::Vector3 camDir = camera->Unproject(camera->GetScreenWidth() / 2, camera->GetScreenHeight() / 2).GetDirection();
            AZ::Vector3 thirdAxis = mMovementDirection.Cross(camDir).GetNormalized();
            mMovementPlaneNormal = thirdAxis.Cross(mMovementDirection).GetNormalized();
            thirdAxis = mMovementPlaneNormal.Cross(mMovementDirection).GetNormalized();

            MCore::PlaneEq movementPlane(mMovementPlaneNormal, mPosition);
            MCore::PlaneEq movementPlane2(thirdAxis, mPosition);

            // calculate the intersection points of the mouse positions with the previously calculated plane
            AZ::Vector3 mousePosIntersect, mousePosIntersect2;
            mousePosRay.Intersects(movementPlane, &mousePosIntersect);
            mousePosRay.Intersects(movementPlane2, &mousePosIntersect2);

            if (mousePosIntersect.GetLength() < camera->GetFarClipDistance())
            {
                if (MCore::Math::IsFloatZero(MCore::SafeLength(mMousePosRelative)))
                {
                    mMousePosRelative = movementPlane2.Project(mousePosIntersect) - mPosition;
                }

                mousePosIntersect = movementPlane2.Project(mousePosIntersect);
            }
            else
            {
                if (MCore::Math::IsFloatZero(MCore::SafeLength(mMousePosRelative)))
                {
                    mMousePosRelative = movementPlane.Project(mousePosIntersect2) - mPosition;
                }

                mousePosIntersect = movementPlane.Project(mousePosIntersect2);
            }

            // adjust the movement vector
            movement = mousePosIntersect - mMousePosRelative;
        }

        // update the position of the gizmo
        movement = movement - mPosition;
        movement = AZ::Vector3(movement.GetX() * mMovementDirection.GetX(), movement.GetY() * mMovementDirection.GetY(), movement.GetZ() * mMovementDirection.GetZ());
        mPosition += movement;

        // update the callback
        if (mCallback)
        {
            // reset the callback position, if the position is too far away from the camera
            float farClip = camera->GetFarClipDistance();
            if (mPosition.GetLength() >= farClip)
            {
                mPosition = mCallback->GetOldValueVec() + mRenderOffset;
            }

            // update the callback
            mCallback->Update(GetPosition());
        }
    }
} // namespace MCommon
