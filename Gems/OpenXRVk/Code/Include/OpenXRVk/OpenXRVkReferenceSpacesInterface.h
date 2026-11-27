/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Interface/Interface.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Transform.h>

#include <Atom/RHI.Reflect/Handle.h>
#include <Atom/RPI.Public/XR/XRRenderingInterface.h>

namespace OpenXRVk
{
    //! REMARK: This class must be instantiated BEFORE OpenXRActionsInterface.
    //! To know more about OpenXR Reference Spaces, see the spec:
    //! https://registry.khronos.org/OpenXR/specs/1.0/html/xrspec.html#spaces
    //! This interface consolidates the API that works with definition of Reference Spaces
    //! and retreival of their Poses as AZ::Transforms.
    class IOpenXRReferenceSpaces
    {
    public:
        AZ_RTTI(IOpenXRReferenceSpaces, "{244D24BE-DD6F-430A-8F99-1D24AC1665B6}");
        AZ_DISABLE_COPY_MOVE(IOpenXRReferenceSpaces);

        IOpenXRReferenceSpaces() = default;
        virtual ~IOpenXRReferenceSpaces() = default;

        // Identifiers for all reference spaces that all OpenXR implementations
        // must support. These are the default system spaces. Most OpenXR implementations
        // support additional system-provided reference spaces, and you can try to use their id to attempt
        // to create an space of them at runtime. This explains why this is an ordinary integral
        // instead of an enum.
        using ReferenceSpaceId = uint32_t;
        static constexpr ReferenceSpaceId ReferenceSpaceIdView = 1;
        static constexpr ReferenceSpaceId ReferenceSpaceIdLocal = 2;
        static constexpr ReferenceSpaceId ReferenceSpaceIdStage = 3;
        // With these names you can refer to the default system spaces.
        static constexpr char ReferenceSpaceNameView[] = "View"; // Typically represents the user's head centroid.
        static constexpr char ReferenceSpaceNameLocal[] = "Local"; 
        static constexpr char ReferenceSpaceNameStage[] = "Stage";

        //! @returns a list of strings, Where each string is the name of each active reference space.
        //! The session will always instantiate the three reference spaces mentioned above: View, Local and Stage.
        //! So this list will always contain at least three strings.
        virtual AZStd::vector<AZStd::string> GetReferenceSpaceNames() const = 0;

        //! Creates a new Reference Space, and the developer will define what name they want to use to address the new space.
        //! @param referenceSpaceType An integral, that identifies a system reference space, that will be used as anchor reference
        //!        for the new space that will be created. The caller can use other integrals, different than the three constants
        //!        mentioned above, but the caller would need to verify on their own that said system reference space is supported
        //!        by the OpenXR Runtime they are working with. OpenXR only guarantess that View, Local and Stage are always supported.
        //! @param spaceName Name of the new space, and must be different than any other reference space previously created.
        //! @param poseInReferenceSpace A transform that defines the default orientation and position of the new space, relative
        //!        to the system reference space defined by @referenceSpaceType.
        //! @returns Success if the space name is unique, and the @referenceSpaceType space id is supported by the runtime.
        //!          In case of failure, an error description is provided.
        virtual AZ::Outcome<bool, AZStd::string> AddReferenceSpace(ReferenceSpaceId referenceSpaceType,
            const AZStd::string& spaceName, const AZ::Transform& poseInReferenceSpace) = 0;

        //! Removes a previously user-created reference space.
        //! @param spaceName The name of the reference space to remove.
        //! @returns Success if a reference space with said name exists and it is NOT one of "View", "Local" or "Stage".
        virtual AZ::Outcome<bool, AZStd::string> RemoveReferenceSpace(const AZStd::string& spaceName) = 0;

        //! REMARK: Maybe it's a good idea to move this into a private interface for the OpenXRVk Gem,
        //! and privately use the native handle (XrSpace)
        virtual const void * GetReferenceSpaceNativeHandle(const AZStd::string& spaceName) const = 0;

        //! Returns the Pose of @spaceName relative to @baseSpaceName.
        //! This function is typically called within the game loop to poll/query the current pose
        //! of a particular reference space, relative to another reference space.
        //! @param spaceName The name of the reference space whose Transform the caller needs to know.
        //! @param baseSpaceName The name of the base reference space that the OpenXR runtime will use to calculate
        //!                      a relative Transform.
        //! @returns If successful, a Transform that defines the pose of @spaceName, relative to @baseSpaceName.
        virtual AZ::Outcome<AZ::Transform, AZStd::string> GetReferenceSpacePose(const AZStd::string& spaceName, const AZStd::string& baseSpaceName) const = 0;

        //! With this method the caller defines the base reference space that will be used each frame
        //! to locate the "View" reference space. Each frame, The "View" reference space is the only
        //! reference space that will be automatically located, and its pose will be internally cached.
        //! The located pose will always be relative to the @spaceName defined the last time this method
        //! was called.
        //! If this method is never called, the runtime will default to the "Local" reference spaces as the base reference space
        //! that will be used to locate the "View" reference space.
        //! This is a stateful  API, you only need to call this method once, or as needed.
        //! Each time GetViewSpacePose() is called, the returned Transform will be relative to @spaceName
        //! @param spaceName The name of the base reference space that will be used to caculate the pose of the View space.
        //! @remark Typically, the "View" reference space represents the User's head centroid.
        //!         Internally, the Pose for each Eye View will be calculated relative to the View Space centroid
        //!         and it will be reported via the notification bus: AZ::RPI::XRSpaceNotificationBus 
        virtual AZ::Outcome<bool, AZStd::string> SetBaseSpaceForViewSpacePose(const AZStd::string& spaceName) = 0;

        //! Returns the name of the current Reference Space that is used as base reference space
        //! when locating the "View" reference space.
        virtual const AZStd::string& GetBaseSpaceForViewSpacePose() const = 0;

        //! @returns The per-frame "calculated and cached" Pose of the "View" Reference Space.
        //! @remark This same Pose is also reported automatically, each frame, via the AZ::RPI::XRSpaceNotificationBus.
        //!         Also the returned Pose is a Transform relative to the base Reference Space defined via
        //!         SetBaseSpaceForViewSpacePose().
        virtual const AZ::Transform& GetViewSpacePose() const = 0;

        //! Useful constants for OpenXR Stereo systems.
        static constexpr uint32_t LeftEyeView = 0;
        static constexpr uint32_t RightEyeView = 1;

        //! For an AR application running on a Phone (Mono) the count will be 1.
        //! For an AR/VR application running on a typical headset (Stereo) the count will be 2.
        //! Some headsets like the Varjo support Quad Views and the eye (view) counts will be 4. 
        virtual uint32_t GetViewCount() const = 0;

        //! Pose of a view(aka eye) relative to the View Space (aka Head) Pose.
        //! For AR applications running on a Phone (aka Mono view configuration) the Eye View Pose
        //! is centered exactly where the View Space Centroid is located, so calling this function wouldn't
        //! make much sense because it'll return an Identity transform.
        //! @remark DO NOT CONFUSE View Pose with "View" Space Pose. This is a confusing aspect
        //!         of the OpenXR API. It is tempting to call this function as GetEyePose.
        virtual const AZ::Transform& GetViewPose(uint32_t eyeIndex) const = 0;

        //! Returns the FovData for a particular View (aka Eye).
        virtual const AZ::RPI::FovData& GetViewFovData(uint32_t eyeIndex) const = 0;

        //! A convenient method that returns the relative pose of all Views (aka Eyes)
        //! relative to the "View" Reference Space, which typically represents the user's head centroid.
        virtual const AZStd::vector<AZ::Transform>& GetViewPoses() const = 0;

        //! Forces updating the cached pose and projection data for all Views (Eyes).
        //! Each frame, all view (aka eye) poses and projections are updated automatically, making this function optional to use.
        //! By default the caller simply gets the per-frame cached version, which should be fine for most applications.
        //! This API was added because according to OpenXR xrLocateViews spec:
        //! "Repeatedly calling xrLocateViews with the same time may not necessarily return the same result.
        //!  Instead the prediction gets increasingly accurate as the function is called closer to the
        //!  given time for which a prediction is made".
        virtual void ForceViewPosesCacheUpdate() = 0;
    };

    using OpenXRReferenceSpacesInterface = AZ::Interface<IOpenXRReferenceSpaces>;

} // namespace OpenXRVk