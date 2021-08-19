/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __MCOMMON_CAMERA_H
#define __MCOMMON_CAMERA_H

#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Math/Vector2.h>
#include <MCore/Source/AABB.h>
#include <MCore/Source/Vector.h>
#include <MCore/Source/Ray.h>
#include "MCommonConfig.h"


namespace MCommon
{
    /**
     * Camera base class.
     */
    class MCOMMON_API Camera
    {
        MCORE_MEMORYOBJECTCATEGORY(Camera, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_MCOMMON);

    public:
        /**
         * The projection type.
         */
        enum ProjectionMode
        {
            PROJMODE_PERSPECTIVE    = 0,    /**< Perspective projection. */
            PROJMODE_ORTHOGRAPHIC   = 1     /**< Orthographic projection. */
        };

        /**
         * Default constructor.
         */
        Camera();

        /**
         * Destructor.
         */
        virtual ~Camera();

        /**
         * Returns the type identification number of the camera class.
         * @result The type identification number.
         */
        virtual uint32 GetType() const = 0;

        /**
         * Gets the type as a description. This for example could be "Perspective Camera" or "First Person Camers".
         * @result The string containing the type of the camera.
         */
        virtual const char* GetTypeString() const = 0;

        /**
         * Set the position of the camera.
         * @param position The new position of the camera.
         */
        MCORE_INLINE void SetPosition(const AZ::Vector3& position);

        /**
         * Get the position of the camera.
         * @return The current position of the camera.
         */
        MCORE_INLINE const AZ::Vector3& GetPosition() const;

        /**
         * Set the projection type.
         * @projectionMode The projection type to assign to the camera.
         */
        MCORE_INLINE void SetProjectionMode(ProjectionMode projectionMode);

        /**
         * Get the projection type.
         * @return The assigned projection type.
         */
        MCORE_INLINE ProjectionMode GetProjectionMode() const;

        /**
         * Set the field of view in degrees.
         * @param fieldOfView The view angle in degrees.
         */
        MCORE_INLINE void SetFOV(float fieldOfView);

        /**
         * Set the clipping plane distances for the orthographic projection mode.
         * @clipDimensions A two component vector which defines the distance to the left (x component) and to the top (y component) from the view origin.
         */
        MCORE_INLINE void SetOrthoClipDimensions(const AZ::Vector2& clipDimensions);

        /**
         * Set the screen dimensions where this camera is used in.
         * @width The screen width in pixels.
         * @height The screen height in pixels.
         */
        MCORE_INLINE void SetScreenDimensions(uint32 width, uint32 height);

        /**
         * Set near clip plane distance.
         * @param nearClipDistance The distance to the near clip plane.
         */
        MCORE_INLINE void SetNearClipDistance(float nearClipDistance);

        /**
         * Set far clip plane distance.
         * @param farClipDistance The distance to the far clip plane.
         */
        MCORE_INLINE void SetFarClipDistance(float farClipDistance);

        /**
         * Set the aspect ratio. The aspect ratio is either calculated by width/height or height/width.
         * @param aspect The aspect ratio.
         */
        MCORE_INLINE void SetAspectRatio(float aspect);

        /**
         * Get the field of view in degrees.
         * @return The view angle in degrees.
         */
        MCORE_INLINE float GetFOV() const;

        /**
         * Get the near clip plane distance.
         * @return The distance to the near clip plane.
         */
        MCORE_INLINE float GetNearClipDistance() const;

        /**
         * Get the far clip plane distance.
         * @return The distance to the far clip plane.
         */
        MCORE_INLINE float GetFarClipDistance() const;

        /**
         * Get the aspect ratio.
         * @return The aspect ratio which is either width/height or height/width.
         */
        MCORE_INLINE float GetAspectRatio() const;

        /**
         * Get the projection matrix of the camera.
         * The projection matrix will be calculated every Update().
         * @return The projection matrix.
         */
        MCORE_INLINE AZ::Matrix4x4& GetProjectionMatrix() { return m_projectionMatrix; }
        MCORE_INLINE const AZ::Matrix4x4& GetProjectionMatrix() const { return m_projectionMatrix; }

        /**
         * Get the view matrix of the camera.
         * The view matrix will be calculated every Update().
         * @return The view matrix.
         */
        MCORE_INLINE AZ::Matrix4x4& GetViewMatrix() { return m_viewMatrix; }
        MCORE_INLINE const AZ::Matrix4x4& GetViewMatrix() const { return m_viewMatrix; }

        /**
         * Get the precalculated viewMatrix * projectionMatrix of the camera.
         * The viewproj matrix will be calculated every Update().
         * @return The precalculated matrix containing the result of viewMatrix * projectionMatrix.
         */
        MCORE_INLINE AZ::Matrix4x4& GetViewProjMatrix() { return m_viewProjMatrix; }
        MCORE_INLINE const AZ::Matrix4x4& GetViewProjMatrix() const { return m_viewProjMatrix; }

        /**
         * Get the translation speed.
         * @return The value which determines how fast the camera will move based on the user input.
         */
        MCORE_INLINE float GetTranslationSpeed() const;

        /**
         * Set the translation speed.
         * @param translationSpeed The value which determines how fast the camera will move based on the user input.
         */
        MCORE_INLINE void SetTranslationSpeed(float translationSpeed);

        /**
         * Get the rotation speed in degrees.
         * @return The angle which determines how fast the camera will rotate based on the user input.
         */
        MCORE_INLINE float GetRotationSpeed() const;

        /**
         * Set the rotation speed in degrees.
         * @param rotationSpeed The angle which determines how fast the camera will rotate based on the user input.
         */
        MCORE_INLINE void SetRotationSpeed(float rotationSpeed);

        /**
         * Get the screen width.
         * @return The screen width.
         */
        MCORE_INLINE uint32 GetScreenWidth();

        /**
         * Get the screen height.
         * @return The screen height.
         */
        MCORE_INLINE uint32 GetScreenHeight();

        /**
         * Unproject screen coordinates to a ray in world space.
         * @param screenX The mouse position x value or another horizontal screen coordinate in range [0, screenWidth].
         * @param screenY The mouse position y value or another vertical screen coordinate in range [0, screenHeight].
         * @return The unprojected ray.
         */
        virtual MCore::Ray Unproject(int32 screenX, int32 screenY);

        /**
         * Update the camera transformation.
         * Recalculate the view frustum, the projection and the view matrix.
         * Overload this function for inherited cameras and make sure calling the base class update
         * at the very end of the overloaded one.
         * @param timeDelta The time (in seconds) passed since the last call.
         */
        virtual void Update(float timeDelta = 0.0f);

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
        virtual void ProcessMouseInput(int32 mouseMovementX, int32 mouseMovementY, bool leftButtonPressed, bool middleButtonPressed, bool rightButtonPressed, uint32 keyboardKeyFlags = 0)
        {
            MCORE_UNUSED(mouseMovementX);
            MCORE_UNUSED(mouseMovementY);
            MCORE_UNUSED(leftButtonPressed);
            MCORE_UNUSED(middleButtonPressed);
            MCORE_UNUSED(rightButtonPressed);
            MCORE_UNUSED(keyboardKeyFlags);
        }

        /**
         * Reset all camera attributes to their default settings.
         * @param flightTime The time of the interpolated flight between the actual camera position and the reset target.
         */
        virtual void Reset(float flightTime = 0.0f);

        /**
         * Translate, rotate and zoom the camera so that we are seeing a closeup of our given bounding box.
         * @param boundingBox The bounding box around the object we want to look at.
         * @param flightTime The time of the interpolated flight between the actual camera position and the target.
         */
        virtual void ViewCloseup(const MCore::AABB& boundingBox, float flightTime)
        {
            MCORE_UNUSED(boundingBox);
            MCORE_UNUSED(flightTime);
        }

        virtual void AutoUpdateLimits() {}

    protected:
        AZ::Matrix4x4           m_projectionMatrix;          /**< The projection matrix. */
        AZ::Matrix4x4           m_viewMatrix;                /**< The view matrix. */
        AZ::Matrix4x4           m_viewProjMatrix;            /**< ViewMatrix * projectionMatrix. Will be recalculated every update call. */
        AZ::Vector3             m_position;                  /**< The camera position. */
        AZ::Vector2             m_orthoClipDimensions;       /**< A two component vector which defines the distance to the left (x component) and to the top (y component) from the view origin. */
        float                   m_fov;                       /**< The vertical field-of-view in degrees. */
        float                   m_nearClipDistance;          /**< Distance to the near clipping plane. */
        float                   m_farClipDistance;           /**< Distance to the far clipping plane. */
        float                   m_aspect;                    /**< x/y viewport ratio. */
        float                   m_rotationSpeed;             /**< The angle in degrees that will be applied to the current rotation when the mouse is moving one pixel. */
        float                   m_translationSpeed;          /**< The value that will be applied to the current camera position when moving the mouse one pixel. */
        ProjectionMode          m_projectionMode;            /**< The projection mode. The camera supports either perspective or orthographic projection. */
        uint32                  m_screenWidth;               /**< The screen width in pixels where the camera is used. */
        uint32                  m_screenHeight;              /**< The screen height in pixels where the camera is used. */
    };

    // include inline code
#include "Camera.inl"
} // namespace MCommon


#endif
