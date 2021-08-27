/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MCore/Source/AABB.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/BoundingSphere.h>
#include <MCore/Source/Ray.h>
#include "MCommonConfig.h"
#include "RenderUtil.h"
#include "TransformationManipulator.h"


namespace MCommon
{
    /**
     * Camera base class.
     */
    class MCOMMON_API RotateManipulator
        : public TransformationManipulator
    {
        MCORE_MEMORYOBJECTCATEGORY(RotateManipulator, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        /**
         * The rotation mode.
         */
        enum RotationMode
        {
            ROTATE_NONE         = 0,    /**< No rotation.                           */
            ROTATE_X            = 1,    /**< ROTATE in x direction.                 */
            ROTATE_Y            = 2,    /**< ROTATE in y direction.                 */
            ROTATE_Z            = 3,    /**< ROTATE in z direction.                 */
            ROTATE_CAMROLL      = 4,    /**< ROTATE arround cam roll vector.        */
            ROTATE_CAMPITCHYAW  = 5,    /**< ROTATE arround cam pitch, yaw vector.  */
        };

        /**
         * Default constructor.
         */
        RotateManipulator(float scalingFactor = 1.0f, bool isVisible = true);

        /**
         * Destructor.
         */
        virtual ~RotateManipulator();

        /**
         * Function to get the type of a gizmo.
         * @return type The type of the gizmo.
         */
        EGizmoType GetType() const { return GIZMOTYPE_ROTATION; }

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
        AZ::Vector3             m_rotation;
        AZ::Quaternion          m_rotationQuat;
        AZ::Vector3             m_rotationAxis;
        AZ::Vector3             m_clickPosition;

        // bounding volumes for the axes
        MCore::BoundingSphere   m_innerBoundingSphere;
        MCore::BoundingSphere   m_outerBoundingSphere;
        MCore::AABB             m_xAxisAabb;
        MCore::AABB             m_yAxisAabb;
        MCore::AABB             m_zAxisAabb;
        MCore::AABB             m_xAxisInnerAabb;
        MCore::AABB             m_yAxisInnerAabb;
        MCore::AABB             m_zAxisInnerAabb;

        // the proportions of the rotation manipulator
        float                   m_size;
        float                   m_innerRadius;
        float                   m_outerRadius;
        float                   m_arrowBaseRadius;
        float                   m_aabbWidth;
        float                   m_axisSize;
        float                   m_textDistance;
        float                   m_innerQuadSize;

        // orientation information
        float                   m_signX;
        float                   m_signY;
        float                   m_signZ;
        bool                    m_xAxisVisible;
        bool                    m_yAxisVisible;
        bool                    m_zAxisVisible;

        // store the projection mode of the current render widget
        MCommon::Camera::ProjectionMode m_currentProjectionMode;
    };
} // namespace MCommon
