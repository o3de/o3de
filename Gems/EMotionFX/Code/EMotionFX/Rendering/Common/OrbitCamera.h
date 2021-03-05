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

#ifndef __MCOMMON_ORBITCAMERA_H
#define __MCOMMON_ORBITCAMERA_H

// include required headers
#include "LookAtCamera.h"


namespace MCommon
{
    /**
     * Orbit camera
     */
    class MCOMMON_API OrbitCamera
        : public LookAtCamera
    {
        MCORE_MEMORYOBJECTCATEGORY(OrbitCamera, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        enum
        {
            TYPE_ID = 0x00000003
        };

        /**
         * Default constructor.
         */
        OrbitCamera();

        /**
         * Destructor.
         */
        ~OrbitCamera();

        uint32 GetType() const override                         { return TYPE_ID; }
        const char* GetTypeString() const override              { return "Perspective"; }

        /**
         * Update the camera transformation.
         * Recalculate the view frustum, the projection and the view matrix.
         * @param timeDelta The time (in seconds) passed since the last call.
         */
        void Update(float timeDelta = 0.0f) override;

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
        void ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags = 0) override;

        /**
         * Reset all camera attributes to their default settings.
         * @param flightTime The time of the interpolated flight between the actual camera position and the reset target.
         */
        void Reset(float flightTime = 0.0f) override;

        /**
         * Translate, rotate and zoom the camera so that we are seeing a closeup of our given bounding box.
         * @param boundingBox The bounding box around the object we want to look at.
         * @param flightTime The time of the interpolated flight between the actual camera position and the target.
         */
        void ViewCloseup(const MCore::AABB& boundingBox, float flightTime) override;

        void StartFlight(float distance, const AZ::Vector3& position, float alpha, float beta, float flightTime);
        bool GetIsFlightActive() const                                  { return mFlightActive; }
        void SetFlightTargetPosition(const AZ::Vector3& targetPos)      { mFlightTargetPosition = targetPos; }
        float FlightTimeLeft() const
        {
            if (mFlightActive == false)
            {
                return 0.0f;
            }
            return mFlightMaxTime - mFlightCurrentTime;
        }

        MCORE_INLINE float GetCurrentDistance() const                   { return mCurrentDistance; }
        void SetCurrentDistance(float distance)                         { mCurrentDistance = distance; }

        MCORE_INLINE float GetAlpha() const                             { return mAlpha; }
        static float GetDefaultAlpha()                                  { return 110.0f; }
        void SetAlpha(float alpha)                                      { mAlpha = alpha; }

        MCORE_INLINE float GetBeta() const                              { return mBeta; }
        static float GetDefaultBeta()                                   { return 20.0f; }
        void SetBeta(float beta)                                        { mBeta = beta; }

        // automatically updates the camera afterwards
        void Set(float alpha, float beta, float currentDistance, const AZ::Vector3& target);

        void AutoUpdateLimits() override;


    private:
        AZ::Vector2     mPositionDelta;     /**< The position delta which will be applied to the camera position when calling update. After adjusting the position it will be reset again. */
        float           mMinDistance;       /**< The minimum distance from the orbit camera to its target in the orbit sphere. */
        float           mMaxDistance;       /**< The maximum distance from the orbit camera to its target in the orbit sphere. */
        float           mCurrentDistance;   /**< The current distance from the orbit camera to its target in the orbit sphere. */
        float           mAlpha;             /**< The horizontal angle in our orbit sphere. */
        float           mBeta;              /**< The vertical angle in our orbit sphere. */

        bool            mFlightActive;
        float           mFlightMaxTime;
        float           mFlightCurrentTime;
        float           mFlightSourceDistance;
        float           mFlightTargetDistance;
        AZ::Vector3     mFlightSourcePosition;
        AZ::Vector3     mFlightTargetPosition;
        float           mFlightSourceAlpha;
        float           mFlightTargetAlpha;
        float           mFlightSourceBeta;
        float           mFlightTargetBeta;
    };
} // namespace MCommon


#endif
