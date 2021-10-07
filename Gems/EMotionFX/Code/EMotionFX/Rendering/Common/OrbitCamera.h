/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        bool GetIsFlightActive() const                                  { return m_flightActive; }
        void SetFlightTargetPosition(const AZ::Vector3& targetPos)      { m_flightTargetPosition = targetPos; }
        float FlightTimeLeft() const
        {
            if (m_flightActive == false)
            {
                return 0.0f;
            }
            return m_flightMaxTime - m_flightCurrentTime;
        }

        MCORE_INLINE float GetCurrentDistance() const                   { return m_currentDistance; }
        void SetCurrentDistance(float distance)                         { m_currentDistance = distance; }

        MCORE_INLINE float GetAlpha() const                             { return m_alpha; }
        static float GetDefaultAlpha()                                  { return 110.0f; }
        void SetAlpha(float alpha)                                      { m_alpha = alpha; }

        MCORE_INLINE float GetBeta() const                              { return m_beta; }
        static float GetDefaultBeta()                                   { return 20.0f; }
        void SetBeta(float beta)                                        { m_beta = beta; }

        // automatically updates the camera afterwards
        void Set(float alpha, float beta, float currentDistance, const AZ::Vector3& target);

        void AutoUpdateLimits() override;


    private:
        AZ::Vector2     m_positionDelta;     /**< The position delta which will be applied to the camera position when calling update. After adjusting the position it will be reset again. */
        float           m_minDistance;       /**< The minimum distance from the orbit camera to its target in the orbit sphere. */
        float           m_maxDistance;       /**< The maximum distance from the orbit camera to its target in the orbit sphere. */
        float           m_currentDistance;   /**< The current distance from the orbit camera to its target in the orbit sphere. */
        float           m_alpha;             /**< The horizontal angle in our orbit sphere. */
        float           m_beta;              /**< The vertical angle in our orbit sphere. */

        bool            m_flightActive;
        float           m_flightMaxTime;
        float           m_flightCurrentTime;
        float           m_flightSourceDistance;
        float           m_flightTargetDistance;
        AZ::Vector3     m_flightSourcePosition;
        AZ::Vector3     m_flightTargetPosition;
        float           m_flightSourceAlpha;
        float           m_flightTargetAlpha;
        float           m_flightSourceBeta;
        float           m_flightTargetBeta;
    };
} // namespace MCommon


#endif
