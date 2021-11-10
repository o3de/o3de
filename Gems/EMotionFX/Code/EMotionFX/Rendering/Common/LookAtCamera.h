/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MCOMMON_LOOKATCAMERA_H
#define __MCOMMON_LOOKATCAMERA_H

// include required headers
#include "Camera.h"


namespace MCommon
{
    /**
     * Look at camera.
     */
    class MCOMMON_API LookAtCamera
        : public Camera
    {
        MCORE_MEMORYOBJECTCATEGORY(LookAtCamera, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON)

    public:
        enum
        {
            TYPE_ID = 0x00000002
        };

        /**
         * Default constructor.
         */
        LookAtCamera();

        /**
         * Destructor.
         */
        virtual ~LookAtCamera();

        uint32 GetType() const                                      { return TYPE_ID; }
        const char* GetTypeString() const                       { return "Look At"; }

        /**
         * Look at target.
         * @param target The target position, so where the will be camera is looking at.
         * @param up The up vector, describing the roll of the camera, where (0,1,0) would mean the camera is straight up and has no roll. (0,-1,0) would be upside down, etc.
         */
        void LookAt(const AZ::Vector3& target, const AZ::Vector3& up = AZ::Vector3( 0.0f, 1.0f, 0.0f ));

        /**
         * Update the camera transformation.
         * Recalculate the view frustum, the projection and the view matrix.
         * @param timeDelta The time (in seconds) passed since the last call.
         */
        virtual void Update(float timeDelta = 0.0f);

        /**
         * Reset all camera attributes to their default settings.
         * @param flightTime The time of the interpolated flight between the actual camera position and the reset target.
         */
        virtual void Reset(float flightTime = 0.0f);

        /**
         * Set the target position. Note that the camera needs an update after setting a new target.
         * @param[in] target The new camera target.
         */
        MCORE_INLINE void SetTarget(const AZ::Vector3& target)   { m_target = target; }

        /**
         * Get the target position.
         * @return The current camera target.
         */
        MCORE_INLINE AZ::Vector3 GetTarget() const               { return m_target; }

        /**
         * Set the up vector for the camera. Note that the camera needs an update after setting a new up vector.
         * @param[in] up The new camera up vector.
         */
        MCORE_INLINE void SetUp(const AZ::Vector3& up)           { m_up = up; }

        /**
         * Get the camera up vector.
         * @return The current up vector.
         */
        MCORE_INLINE AZ::Vector3 GetUp() const                   { return m_up; }

    protected:
        AZ::Vector3 m_target; /**< The camera target. */
        AZ::Vector3 m_up;     /**< The up vector of the camera. */
    };
} // namespace MCommon


#endif
