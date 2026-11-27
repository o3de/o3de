/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/OpenXRVkSpace.h>
#include <OpenXRVk/OpenXRVkUtils.h>
#include <AzCore/Debug/Trace.h>

namespace OpenXRVk
{
    XR::Ptr<Space> Space::Create()
    {
        return aznew Space;
    }

    AZ::RHI::ResultCode Space::InitInternal()
    {
        return AZ::RHI::ResultCode::Success;
    }

    void Space::CreateVisualizedSpaces(XrSession xrSession)
    {
        AZ_Assert(xrSession != XR_NULL_HANDLE, "XR session is null");

        for (uint32_t i = 0; i < static_cast<uint32_t>(SpaceType::Count); i++)
        {
            XrReferenceSpaceCreateInfo referenceSpaceCreateInfo = GetXrReferenceSpaceCreateInfo(static_cast<SpaceType>(i));
            XrSpace space;
            XrResult result = xrCreateReferenceSpace(xrSession, &referenceSpaceCreateInfo, &space);
            if (IsSuccess(result))
            {
                m_xrSpaces.push_back(space);
            }
            else
            {
                AZ_Warning("OpenXrVK", false, "Failed to create reference space %s with error %d", ToString(static_cast<SpaceType>(i)).data(), result);
            }
        }
    }

    XrReferenceSpaceCreateInfo Space::GetXrReferenceSpaceCreateInfo(SpaceType spaceType)
    {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        referenceSpaceCreateInfo.poseInReferenceSpace = Identity();
        switch (spaceType)
        {
            case SpaceType::View:
            {
                //Track the view origin used to generate view transforms for the primary viewer (or centroid of
                //view origins if stereo), with +Y up, +X to the right, and -Z forward.
                referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
                break;
            }
            case SpaceType::ViewFront:
            {
                // Track view head-locked 5m in front of device.
                referenceSpaceCreateInfo.poseInReferenceSpace = Translation({ 0.f, 0.f, -5.f }),
                    referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_VIEW;
                break;
            }
            case SpaceType::Local:
            {
                //Track center Local space which is world-locked origin, gravity-aligned to exclude
                //pitch and roll, with +Y up, +X to the right, and -Z forward.
                referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_LOCAL;
                break;
            }
            case SpaceType::Stage:
            {
                //Track center Stage space which is defined flat, rectangular space that is empty and can be walked around on.
                referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
                break;
            }
            case SpaceType::StageLeft:
            {
                //Track Left Stage space which is basically the center stage translated to the left and down by 5m.
                referenceSpaceCreateInfo.poseInReferenceSpace = RotateCCWAboutYAxis(0.f, { -5.f, 0.f, -5.f });
                referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
                break;
            }
            case SpaceType::StageRight:
            {
                //Track Right Stage space which is basically the center stage translated to the right and down by 5m.
                referenceSpaceCreateInfo.poseInReferenceSpace = RotateCCWAboutYAxis(0.f, { 5.f, 0.f, -5.f });
                referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
                break;
            }
            case SpaceType::StageLeftRotated:
            {
                //Track Left Rotated Stage space which is basically the center stage translated and Rotated by 60 deg (i.e pi/3). Remove if not used in future
                referenceSpaceCreateInfo.poseInReferenceSpace = RotateCCWAboutYAxis(AZ::Constants::Pi / 3.f, { -5.f, 0.5f, -5.f });
                referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
                break;
            }
            case SpaceType::StageRightRotated:
            {
                //Track Right Rotated Stage space which is basically the center stage translated and Rotated by 60 deg (i.e pi/3). Remove if not used in future
                referenceSpaceCreateInfo.poseInReferenceSpace = RotateCCWAboutYAxis(-AZ::Constants::Pi / 3.f, { 5.f, 0.5f, -5.f });
                referenceSpaceCreateInfo.referenceSpaceType = XR_REFERENCE_SPACE_TYPE_STAGE;
                break;
            }
            default:
            {
                AZ_Assert(false, "Unknown reference space type '%s'", ToString(static_cast<SpaceType>(spaceType)).data());
            }
        }
        return referenceSpaceCreateInfo;
    }

    XrPosef Space::Identity()
    {
        XrPosef t{};
        t.orientation.w = 1;
        return t;
    }

    XrPosef Space::Translation(const XrVector3f& translation)
    {
        XrPosef t = Identity();
        t.position = translation;
        return t;
    }

    XrPosef Space::RotateCCWAboutYAxis(float radians, XrVector3f translation)
    {
        XrPosef t = Identity();
        t.orientation.x = 0.f;
        t.orientation.y = AZStd::sin(radians * 0.5f);
        t.orientation.z = 0.f;
        t.orientation.w = AZStd::cos(radians * 0.5f);
        t.position = translation;
        return t;
    }

    XrSpace Space::GetXrSpace(SpaceType spaceType) const
    {
        return m_xrSpaces[static_cast<uint32_t>(spaceType)];
    }

    void Space::ShutdownInternal()
    {
        for (XrSpace& space : m_xrSpaces)
        {
            if (space != XR_NULL_HANDLE)
            {
                xrDestroySpace(space);
            }
        }
        m_xrSpaces.clear();
    }
}
