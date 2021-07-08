/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MCOMMON_TRANSLATEMANIPULATOR_H
#define __MCOMMON_TRANSLATEMANIPULATOR_H

// include the Core system
#include <MCore/Source/Vector.h>
#include <MCore/Source/Ray.h>
#include "MCommonConfig.h"
#include "RenderUtil.h"
#include "TransformationManipulator.h"


namespace MCommon
{
    /**
     * Translate manipulator gizmo class.
     */
    class MCOMMON_API TranslateManipulator
        : public TransformationManipulator
    {
        MCORE_MEMORYOBJECTCATEGORY(TranslateManipulator, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        /**
         * The translation direction/plane.
         */
        enum TranslationMode
        {
            TRANSLATE_NONE          = 0,    /**< No translation. */
            TRANSLATE_X             = 1,    /**< Translate in x direction. */
            TRANSLATE_Y             = 2,    /**< Translate in y direction. */
            TRANSLATE_Z             = 3,    /**< Translate in z direction. */
            TRANSLATE_XY            = 4,    /**< Translate in x-y-plane. */
            TRANSLATE_XZ            = 5,    /**< Translate in x-z-plane. */
            TRANSLATE_YZ            = 6     /**< Translate in y-z-plane. */
        };

        /**
         * Default constructor.
         */
        TranslateManipulator(float scalingFactor = 1.0f, bool isVisible = true);

        /**
         * Destructor.
         */
        virtual ~TranslateManipulator();

        /**
         * Function to get the type of a gizmo.
         * @return type The type of the gizmo.
         */
        EGizmoType GetType() const { return GIZMOTYPE_TRANSLATION; }

        /**
         * function to update the bounding volumes.
         */
        void UpdateBoundingVolumes(MCommon::Camera* camera = NULL);

        /**
         * update the visibility flags for the axes.
         * @param camera The camera, used to calculate visibility of the axes.
         */
        void UpdateAxisVisibility(MCommon::Camera* camera);

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
        // bounding volumes for the axes
        MCore::AABB             mXAxisAABB;
        MCore::AABB             mYAxisAABB;
        MCore::AABB             mZAxisAABB;
        MCore::AABB             mXYPlaneAABB;
        MCore::AABB             mXZPlaneAABB;
        MCore::AABB             mYZPlaneAABB;

        // the scaling factors for the translate manipulator
        float                   mSize;
        float                   mArrowLength;
        float                   mBaseRadius;
        float                   mPlaneSelectorPos;
        AZ::Vector3             mMovementPlaneNormal;
        AZ::Vector3             mMovementDirection;
        AZ::Vector3             mMousePosRelative;
        bool                    mXAxisVisible;
        bool                    mYAxisVisible;
        bool                    mZAxisVisible;
    };
} // namespace MCommon


#endif
