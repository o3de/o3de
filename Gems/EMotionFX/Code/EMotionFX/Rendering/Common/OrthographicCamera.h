/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include required headers
#include "Camera.h"


namespace MCommon
{
    /**
     * Orthographic camera
     */
    class MCOMMON_API OrthographicCamera
        : public Camera
    {
        MCORE_MEMORYOBJECTCATEGORY(OrthographicCamera, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        enum
        {
            TYPE_ID = 0x00000004
        };

        enum ViewMode
        {
            VIEWMODE_FRONT  = 0,    /**< Orthographic projection front view. */
            VIEWMODE_BACK   = 1,    /**< Orthographic projection back view. */
            VIEWMODE_TOP    = 2,    /**< Orthographic projection top view. */
            VIEWMODE_BOTTOM = 3,    /**< Orthographic projection bottom view. */
            VIEWMODE_LEFT   = 4,    /**< Orthographic projection left view. */
            VIEWMODE_RIGHT  = 5     /**< Orthographic projection right view. */
        };

        /**
         * Constructor.
         */
        OrthographicCamera(ViewMode viewMode);

        /**
         * Destructor.
         */
        ~OrthographicCamera();

        /**
         * Returns the type identification number of the orthographic camera class.
         * @result The type identification number.
         */
        uint32 GetType() const override;

        /**
         * Gets the type as a description. This for example could be "Front" or "Top".
         * @result The string containing the type of the camera.
         */
        const char* GetTypeString() const override;

        MCORE_INLINE void SetMode(ViewMode viewMode)                { m_mode = viewMode; }
        MCORE_INLINE ViewMode GetMode() const                       { return m_mode; }

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
        void ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags) override;

        /**
         * Reset all camera attributes to their default settings.
         * @param flightTime The time of the interpolated flight between the actual camera position and the reset target.
         */
        void Reset(float flightTime = 0.0f) override;

        /**
         * Translate and zoom the camera so that we are seeing a closeup of our given bounding box.
         * @param boundingBox The bounding box around the object we want to look at.
         * @param flightTime The time of the interpolated flight between the actual camera position and the target.
         */
        void ViewCloseup(const MCore::AABB& boundingBox, float flightTime) override;

        void AutoUpdateLimits() override;

        void StartFlight(float distance, const AZ::Vector3& position, float flightTime);
        bool GetIsFlightActive() const                                          { return m_flightActive; }
        void SetFlightTargetPosition(const AZ::Vector3& targetPos)              { m_flightTargetPosition = targetPos; }
        float FlightTimeLeft() const
        {
            if (m_flightActive == false)
            {
                return 0.0f;
            }
            return m_flightMaxTime - m_flightCurrentTime;
        }

        /**
         * Unproject screen coordinates to a ray in world space.
         * @param screenX The mouse position x value or another horizontal screen coordinate in range [0, screenWidth].
         * @param screenY The mouse position y value or another vertical screen coordinate in range [0, screenHeight].
         * @return The unprojected ray.
         */
        MCore::Ray Unproject(int32 screenX, int32 screenY) override;

        MCORE_INLINE void SetCurrentDistance(float distance)                    { m_currentDistance = distance; }
        MCORE_INLINE float GetCurrentDistance() const                           { return m_currentDistance; }

    private:
        ViewMode m_mode;
        AZ::Vector2     m_positionDelta;     /**< The position delta which will be applied to the camera position when calling update. After adjusting the position it will be reset again. */
        float           m_minDistance;       /**< The minimum distance from the orbit camera to its target in the orbit sphere. */
        float           m_maxDistance;       /**< The maximum distance from the orbit camera to its target in the orbit sphere. */
        float           m_currentDistance;   /**< The current distance from the orbit camera to its target in the orbit sphere. */
        bool            m_flightActive;
        float           m_flightMaxTime;
        float           m_flightCurrentTime;
        float           m_flightSourceDistance;
        AZ::Vector3     m_flightSourcePosition;
        float           m_flightTargetDistance;
        AZ::Vector3     m_flightTargetPosition;
    };
} // namespace MCommon

