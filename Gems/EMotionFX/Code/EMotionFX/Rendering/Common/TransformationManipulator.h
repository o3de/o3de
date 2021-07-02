/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the Core system
#include <MCore/Source/Attribute.h>
#include <MCore/Source/Vector.h>
#include "MCommonConfig.h"
#include "RenderUtil.h"


namespace MCommon
{
    /**
     * Callback base class used to update actorinstances.
     */
    class MCOMMON_API ManipulatorCallback
    {
        MCORE_MEMORYOBJECTCATEGORY(ManipulatorCallback, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        /**
         * The constructor.
         */
        ManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const AZ::Vector3& oldValue)
        {
            mActorInstance  = actorInstance;
            mOldValueVec    = oldValue;
            mCurrValueVec   = oldValue;
            mCurrValueQuat  = AZ::Quaternion::CreateIdentity();
            mOldValueQuat   = AZ::Quaternion::CreateIdentity();
        }

        /**
         * Constructor for quaternions.
         */
        ManipulatorCallback(EMotionFX::ActorInstance* actorInstance, const AZ::Quaternion& oldValue)
        {
            mActorInstance  = actorInstance;
            mOldValueQuat   = oldValue;
            mCurrValueQuat  = oldValue;
            mOldValueVec    = AZ::Vector3::CreateZero();
            mCurrValueVec   = AZ::Vector3::CreateZero();
        }

        /**
         * The destructor.
         */
        virtual ~ManipulatorCallback() {}

        /**
         * Update the actor instance.
         */
        virtual void Update(const AZ::Vector3& value)           { mCurrValueVec = value; }
        virtual void Update(const AZ::Quaternion& value)     { mCurrValueQuat = value; }

        /**
         * Update old transformation values of the callback
         */
        virtual void UpdateOldValues() {}

        /**
         * Functions to get the current value.
         * @return the position/scale/rotation of the actor instance.
         */
        virtual AZ::Vector3 GetCurrValueVec()           { return mCurrValueVec; }
        virtual AZ::Quaternion GetCurrValueQuat()       { return mCurrValueQuat; }

        /**
         * Return the old value.
         * @return the old value.
         */
        const AZ::Vector3& GetOldValueVec() const       { return mOldValueVec; }
        const AZ::Quaternion& GetOldValueQuat() const   { return mOldValueQuat; }

        /**
         * Apply transformation.
         */
        virtual void ApplyTransformation() { mOldValueVec = mCurrValueVec; mOldValueQuat = mCurrValueQuat; }

        /**
         * returns the actor instance, if there is one assigned to the callback.
         * @return The actor instance.
         */
        EMotionFX::ActorInstance* GetActorInstance()        { return mActorInstance; }

        virtual bool GetResetFollowMode() const             { return false; }

    protected:
        AZ::Quaternion              mOldValueQuat;
        AZ::Quaternion              mCurrValueQuat;
        AZ::Vector3                 mOldValueVec;
        AZ::Vector3                 mCurrValueVec;
        EMotionFX::ActorInstance*   mActorInstance;
    };

    /**
     * Base class for manipulator gizmos.
     */
    class MCOMMON_API TransformationManipulator
    {
        MCORE_MEMORYOBJECTCATEGORY(TransformationManipulator, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        enum EGizmoType
        {
            GIZMOTYPE_UNKNOWN       = 0,
            GIZMOTYPE_TRANSLATION   = 1,
            GIZMOTYPE_ROTATION      = 2,
            GIZMOTYPE_SCALE         = 3
        };

        /**
         * Default constructor.
         */
        TransformationManipulator(float scalingFactor = 1.0f, bool isVisible = true)
        {
            mScalingFactor      = scalingFactor;
            mIsVisible          = isVisible;
            mSelectionLocked    = false;
            mPosition           = AZ::Vector3::CreateZero();
            mRenderOffset       = AZ::Vector3::CreateZero();
            mCallback           = nullptr;
        }

        /**
         * Destructor.
         */
        virtual ~TransformationManipulator()
        {
            delete mCallback;
        }

        /**
         * Function to init the position of the gizmo.
         */
        void Init(const AZ::Vector3& position)                          { mPosition = position + mRenderOffset; UpdateBoundingVolumes(); }

        /**
         * Function to set the name of the gizmo.
         * @param name The name of the gizmo. (e.g. used to identify different parameters)
         */
        void SetName(const AZStd::string& name)                         { mName = name; }

        /**
         * Function to get the gizmo name.
         * @return The name of the gizmo.
         */
        const AZStd::string& GetName() const                            { return mName; }

        /**
         * Get the selection lock state.
         * @return the selection lock state.
         */
        void SetSelectionLocked(bool selectionLocked)                   { mSelectionLocked = selectionLocked; }
        bool GetSelectionLocked()                                       { return mSelectionLocked; }

        /**
         * Set the visible state of the manipulator.
         */
        void SetIsVisible(bool isVisible = true)                          { mIsVisible = isVisible; }

        /**
         * Set the scale of the gizmo.
         * @param scale The new scale value for the gizmo.
         */
        void SetScale(float scale, MCommon::Camera* camera = nullptr)     { mScalingFactor = scale; UpdateBoundingVolumes(camera); }

        /**
         * Set mode of the gizmo.
         */
        void SetMode(uint32 mode)                                       { mMode = mode; }

        /**
         * Set the render offset of the gizmo.
         * Only affects rendering position of the gizmo, not the actual value it modifies.
         * @param offset The offset position of the gizmo.
         */
        void SetRenderOffset(const AZ::Vector3& offset)
        {
            AZ::Vector3 oldPos = GetPosition();
            mRenderOffset = offset;
            Init(oldPos);
        }

        /**
         * Get the position of the gizmo.
         * @return The position of the gizmo.
         */
        AZ::Vector3 GetPosition() const                                 { return mPosition - mRenderOffset; }

        /**
         * Get the position offset of the gizmo.
         * Only affects rendering position of the gizmo, not the actual value it modifies.
         * @return The offset position of the gizmo.
         */
        const AZ::Vector3& GetRenderOffset() const                      { return mRenderOffset; }

        /**
         * Set the callback.
         * @param callback Pointer to the callback used to manipulate the actorinstance.
         */
        void SetCallback(ManipulatorCallback* callback)                 { delete mCallback; mCallback = callback; }

        /**
         * Returns the current callback of the manipulator.
         * Used to apply the transformation upon mouse release for example.
         * @return The manipulator callback.
         */
        ManipulatorCallback* GetCallback()                              { return mCallback; }

        /**
         * Function to get the mode of the transformation manipulator.
         * @return The mode of the manipulator.
         */
        uint32 GetMode()                                                { return mMode; }

        /**
         * Returns the visible state of the gizmo.
         * @return mIsVisible The visible state of the gizmo.
         */
        bool GetIsVisible()                                             { return mIsVisible; }

        /**
         * Function to get the type of a gizmo. Has to be set by the constructor of the inherited classes.
         * @return type The type of the gizmo.
         */
        virtual EGizmoType GetType() const                              { return GIZMOTYPE_UNKNOWN; }

        /**
         * Function to update the bounding volumes.
         */
        virtual void UpdateBoundingVolumes(MCommon::Camera* camera = nullptr) { MCORE_UNUSED(camera); }

        /**
         * Check if the manipulator is hit by the mouse.
         */
        virtual bool Hit(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY) = 0;

        /**
         * Render the translate manipulator.
         */
        virtual void Render(MCommon::Camera* camera, RenderUtil* renderUtil) { MCORE_UNUSED(camera); MCORE_UNUSED(renderUtil); }

        /**
         * Process input and update the translate manipulator based on that.
         * @param mouseMovement The delta mouse movement in pixels which the mouse moved since the last update.
         * @param leftButtonPressed True if the left mouse button currently is being pressed, false if not.
         * @param middleButtonPressed True if the middle mouse button currently is being pressed, false if not.
         * @param right ButtonPressed True if the right mouse button currently is being pressed, false if not.
         * @param keyboardKeyFlags Integer used as bit array for 32 different camera specific keyboard button states.
         */
        virtual void ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags = 0)
        {
            MCORE_UNUSED(camera);
            MCORE_UNUSED(mousePosX);
            MCORE_UNUSED(mousePosY);
            MCORE_UNUSED(mouseMovementX);
            MCORE_UNUSED(mouseMovementY);
            MCORE_UNUSED(leftButtonPressed);
            MCORE_UNUSED(middleButtonPressed);
            MCORE_UNUSED(rightButtonPressed);
            MCORE_UNUSED(keyboardKeyFlags);
        }

    protected:
        AZ::Vector3             mPosition;
        AZ::Vector3             mRenderOffset;
        AZStd::string           mName;
        AZStd::string           mTempString;
        uint32                  mMode;
        float                   mScalingFactor;
        ManipulatorCallback*    mCallback;
        bool                    mSelectionLocked;
        bool                    mIsVisible;
    };
} // namespace MCommon
