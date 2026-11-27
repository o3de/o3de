/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <XR/XRSpace.h>
#include <XR/XRBase.h>
#include <OpenXRVk_Platform.h>
#include <AzCore/Preprocessor/Enum.h>

namespace OpenXRVk
{
    AZ_ENUM(SpaceType,
        View,
        ViewFront,
        Local,
        Stage,
        StageLeft,
        StageRight,
        StageLeftRotated,
        StageRightRotated,
        Count);

    //!This class is responsible for managing specific space coordinates tracked by the device
    class Space final
        : public XR::Space
    {
    public:
        AZ_CLASS_ALLOCATOR(Space, AZ::SystemAllocator);
        AZ_RTTI(Space, "{E99557D0-9061-4691-9524-CE0ACC3A14FA}", XR::Space);

        static XR::Ptr<Space> Create();
        AZ::RHI::ResultCode InitInternal() override;
        void ShutdownInternal() override;

        //!Initialize XrSpace per SpaceType we want to track
        void CreateVisualizedSpaces(XrSession xrSession);

        //! Return the XrReferenceSpaceCreateInfo associated with each SpaceType
        XrReferenceSpaceCreateInfo GetXrReferenceSpaceCreateInfo(SpaceType spaceType);

        //! Get the XrSpace for a given SpaceType
        XrSpace GetXrSpace(SpaceType spaceType) const;

    private:
        //! XrPose specific matrix translation, Rotation functions
        XrPosef Identity();
        XrPosef Translation(const XrVector3f& translation);
        XrPosef RotateCCWAboutYAxis(float radians, XrVector3f translation);

        AZStd::vector<XrSpace> m_xrSpaces;
    };
}
