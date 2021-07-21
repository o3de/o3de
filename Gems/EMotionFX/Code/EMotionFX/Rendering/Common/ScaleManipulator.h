/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include the Core system
#include <MCore/Source/Vector.h>
#include <MCore/Source/Ray.h>
#include "MCommonConfig.h"
#include "RenderUtil.h"
#include "TransformationManipulator.h"


namespace MCommon
{
    /**
     * Scale manipulator gizmo class.
     */
    class MCOMMON_API ScaleManipulator
        : public TransformationManipulator
    {
        MCORE_MEMORYOBJECTCATEGORY(ScaleManipulator, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        /**
         * The scale direction/plane.
         */
        enum ScaleMode
        {
            SCALE_NONE          = 0,    /**< No scaling.            */
            SCALE_X             = 1,    /**< Scale in x direction.  */
            SCALE_Y             = 2,    /**< Scale in y direction.  */
            SCALE_Z             = 3,    /**< Scale in z direction.  */
            SCALE_XY            = 4,    /**< Scale in x-y-plane.    */
            SCALE_XZ            = 5,    /**< Scale in x-z-plane.    */
            SCALE_YZ            = 6,    /**< Scale in y-z-plane.    */
            SCALE_XYZ           = 7     /**< Scale all directions.  */
        };

        /**
         * Default constructor.
         */
        ScaleManipulator(float scalingFactor = 1.0f, bool isVisible = true);

        /**
         * Destructor.
         */
        virtual ~ScaleManipulator();

        /**
         * Function to get the type of a gizmo.
         * @return type The type of the gizmo.
         */
        EGizmoType GetType() const { return GIZMOTYPE_SCALE; }

        /**
         * function to update the bounding volumes.
         */
        void UpdateBoundingVolumes(MCommon::Camera* camera = nullptr);

        /**
         * updates the axis directions with respect to the camera position.
         */
        void UpdateAxisDirections(Camera* camera);

        /**
         * check if the manipulator is hit by the mouse.
         */
        bool Hit(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY);

        /**
         * Render the translate manipulator.
         */
        void Render(MCommon::Camera* camera, RenderUtil* renderUtil);

        /**
         * Process input and update the translate manipulator based on that.
         * @param mouseMovement The delta mouse movement in pixels which the mouse moved since the last update.
         * @param leftButtonPressed True if the left mouse button currently is being pressed, false if not.
         * @param middleButtonPressed True if the middle mouse button currently is being pressed, false if not.
         * @param right ButtonPressed True if the right mouse button currently is being pressed, false if not.
         * @param keyboardKeyFlags Integer used as bit array for 32 different camera specific keyboard button states.
         */
        void ProcessMouseInput(MCommon::Camera* camera, int32 mousePosX, int32 mousePosY, int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags = 0);

    protected:
        // scale vectors
        AZ::Vector3             mScaleDirection;
        AZ::Vector3             mScale;

        // bounding volumes for the axes
        MCore::AABB             mXAxisAABB;
        MCore::AABB             mYAxisAABB;
        MCore::AABB             mZAxisAABB;
        MCore::AABB             mXYPlaneAABB;
        MCore::AABB             mXZPlaneAABB;
        MCore::AABB             mYZPlaneAABB;
        MCore::AABB             mXYZBoxAABB;

        // size properties of the scale manipulator
        float                   mSize;
        AZ::Vector3             mScaledSize;
        float                   mDiagScale;
        float                   mArrowLength;
        float                   mBaseRadius;
        AZ::Vector3             mFirstPlaneSelectorPos;
        AZ::Vector3             mSecPlaneSelectorPos;
        float                   mSignX;
        float                   mSignY;
        float                   mSignZ;
        bool                    mXAxisVisible;
        bool                    mYAxisVisible;
        bool                    mZAxisVisible;
    };
} // namespace MCommon
