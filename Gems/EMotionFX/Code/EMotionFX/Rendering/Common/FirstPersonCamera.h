/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MCOMMON_FIRSTPERSONCAMERA_H
#define __MCOMMON_FIRSTPERSONCAMERA_H

// include required headers
#include "Camera.h"


namespace MCommon
{
    /**
     * First person camera
     */
    class MCOMMON_API FirstPersonCamera
        : public Camera
    {
        MCORE_MEMORYOBJECTCATEGORY(FirstPersonCamera, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON)

    public:
        enum
        {
            TYPE_ID = 0x00000001
        };

        /**
         * Keyboard button wrapper flags used to be independent from the OS or any input SDK.
         */
        enum EKeyboardButtonState
        {
            FORWARD             = 1 << 0,   /**< If enabled the camera will move forward. */
            BACKWARD            = 1 << 1,   /**< If enabled the camera will move backward. */
            LEFT                = 1 << 2,   /**< If enabled the camera will strafe left. */
            RIGHT               = 1 << 3,   /**< If enabled the camera will strafe right. */
            UP                  = 1 << 4,   /**< If enabled the camera will fly up. */
            DOWN                = 1 << 5,   /**< If enabled the camera will fly down. */
            ENABLE_MOUSELOOK    = 1 << 6    /**< If enabled the camera will be rotated by mouse movement, else only moving is possible. */
        };

        /**
         * Default constructor.
         */
        FirstPersonCamera();

        /**
         * Destructor.
         */
        ~FirstPersonCamera();

        uint32 GetType() const                                      { return TYPE_ID; }
        const char* GetTypeString() const                       { return "First Person"; }

        /**
         * Set the pitch angle in degrees. Looking up and down is limited to 90°. (0=Straight Ahead, +Up, -Down)
         * @param The pitch angle in degrees, range[-90.0°, 90.0°].
         */
        MCORE_INLINE void SetPitch(float pitch)                     { mPitch = pitch; }

        /**
         * Set the yaw angle in degrees. Vertical rotation. (0=East, +North, -South).
         * @param yaw The yaw angle in degrees.
         */
        MCORE_INLINE void SetYaw(float yaw)                         { mYaw = yaw; }

        /**
         * Set the roll angle in degrees. Rotation around the direction axis (0=Straight, +Clockwise, -CCW).
         * @param roll The roll angle in degrees.
         */
        MCORE_INLINE void SetRoll(float roll)                       { mRoll = roll; }

        /**
         * Get the pitch angle in degrees. Looking up and down is limited to 90°. (0=Straight Ahead, +Up, -Down)
         * @return The pitch angle in degrees, range[-90.0°, 90.0°].
         */
        MCORE_INLINE float GetPitch() const                         { return mPitch; }

        /**
         * Get the yaw angle in degrees. Vertical rotation. (0=East, +North, -South).
         * @return The yaw angle in degrees.
         */
        MCORE_INLINE float GetYaw() const                           { return mYaw; }

        /**
         * Get the roll angle in degrees. Rotation around the direction axis (0=Straight, +Clockwise, -CCW).
         * @return The roll angle in degrees.
         */
        MCORE_INLINE float GetRoll() const                          { return mRoll; }

        /**
         * Update the camera transformation.
         * Recalculate the view frustum, the projection and the view matrix.
         * @param timeDelta The time (in seconds) passed since the last call.
         */
        void Update(float timeDelta = 0.0f);

        /**
         * Process input and update the camera transformation based on that.
         * This function has been implemented to make the camera fully independent from any other classes and will
         * be compatible to any platform and SDK.
         * @param mouseMovementX The x delta mouse movement in pixels which the mouse moved since the last camera update.
         * @param mouseMovementY The y delta mouse movement in pixels which the mouse moved since the last camera update.
         * @param leftButtonPressed True if the left mouse button currently is being pressed, false if not.
         * @param middleButtonPressed True if the middle mouse button currently is being pressed, false if not.
         * @param right ButtonPressed True if the right mouse button currently is being pressed, false if not.
         * @param keyboardKeyFlags Integer used as bit array for 32 different camera specific keyboard button states.
         */
        void ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags);

        /**
         * Reset all camera attributes to their default settings.
         * @param flightTime The time of the interpolated flight between the actual camera position and the reset target.
         */
        void Reset(float flightTime = 0.0f);

    private:
        float mPitch;               /**< Up and down. (0=straight ahead, +up, -down) */
        float mYaw;                 /**< Steering. (0=east, +north, -south) */
        float mRoll;                /**< Rotation around axis of screen. (0=straight, +clockwise, -CCW) */
    };
} // namespace MCommon


#endif
