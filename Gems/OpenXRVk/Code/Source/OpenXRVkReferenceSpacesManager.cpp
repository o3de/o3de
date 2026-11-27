/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/containers/array.h>
#include <AzCore/std/utility/pair.h>

#include <OpenXRVk/OpenXRVkUtils.h>

#include "OpenXRVkReferenceSpacesManager.h"

namespace OpenXRVk
{
    bool ReferenceSpacesManager::Init(XrInstance xrInstance, XrSession xrSession, XrViewConfigurationType xrViewConfigurationType, uint32_t numEyeViews)
    {
        m_xrInstance = xrInstance;
        m_xrSession = xrSession;
        m_xrViewConfigurationType = xrViewConfigurationType;
        m_xrViews.resize(numEyeViews, { XR_TYPE_VIEW });

        m_eyeViewPoses.resize_no_construct(numEyeViews);
        m_eyeViewFovDatas.resize_no_construct(numEyeViews);
        m_viewSpacePose = AZ::Transform::Identity();
        for (uint32_t eyeViewIdx = 0; eyeViewIdx < numEyeViews; eyeViewIdx++)
        {
            m_eyeViewPoses[eyeViewIdx] = AZ::Transform::Identity();
        }

        const auto identityTm = AZ::Transform::CreateIdentity();
        AZStd::array<AZStd::pair<uint32_t, const char*>, 3> ReferenceSpaces = {
            {
                {IOpenXRReferenceSpaces::ReferenceSpaceIdView,  IOpenXRReferenceSpaces::ReferenceSpaceNameView },
                {IOpenXRReferenceSpaces::ReferenceSpaceIdLocal, IOpenXRReferenceSpaces::ReferenceSpaceNameLocal },
                {IOpenXRReferenceSpaces::ReferenceSpaceIdStage, IOpenXRReferenceSpaces::ReferenceSpaceNameStage }
            }
        };
        for (const auto& refPair : ReferenceSpaces)
        {
            if (auto outcome = AddReferenceSpace(refPair.first, { refPair.second }, identityTm);
                !outcome.IsSuccess())
            {
                AZ_Error(LogName, false, "Failed to create default reference space [%s].Reason:\n%s\n",
                    refPair.second, outcome.GetError().c_str());
                return false;
            }
        }
        
        // Set the default base space for View Space pose calculation.
        auto outcome = SetBaseSpaceForViewSpacePose({ IOpenXRReferenceSpaces::ReferenceSpaceNameLocal });
        AZ_Assert(outcome.IsSuccess(), "Failed to set the base space for View Space pose location");

        const auto& viewSpace = m_spaces.at(IOpenXRReferenceSpaces::ReferenceSpaceNameView);
        m_viewSpace = &viewSpace;
        return true;
    }


    bool ReferenceSpacesManager::SyncViews(XrTime predictedDisplayTime)
    {
        m_predictedDisplaytime = predictedDisplayTime;

        XrSpaceLocation xrSpaceLocation{ XR_TYPE_SPACE_LOCATION };
        auto result = xrLocateSpace(m_viewSpace->m_xrSpace, m_baseSpaceForViewSpace->m_xrSpace, m_predictedDisplaytime, &xrSpaceLocation);
        if (IsError(result))
        {
            PrintXrError(LogName, result, "Failed to locate View Space [%s] with base space [%s]",
                m_viewSpace->m_name.c_str(), m_baseSpaceForViewSpace->m_name.c_str());
            return false;
        }
        m_viewSpacePose = AzTransformFromXrPose(xrSpaceLocation.pose);
        ForceViewPosesCacheUpdate();
        return true;
    }


    void ReferenceSpacesManager::OnSessionReady()
    {
        // REMARK: On previous versions of the OpenXRVk::Spaces API it was necessary
        // to destroy and recreate the XrSpaces. But since the introduction of the
        // OpenXRVk::OpenXRActionsInterface and other architectural changes
        // that came with it, it appears that it is not ncessary anymore to recreate
        // the XrSpaces.
        // We are leaving this breadcrumb in case it becomes important in the future.
        AZ_Printf(LogName, "%s. For now, this function does nothing.\n", __FUNCTION__);
    }


    const AZStd::vector<XrView>& ReferenceSpacesManager::GetXrViews() const
    {
        return m_xrViews;
    }


    XrSpace ReferenceSpacesManager::GetViewSpaceXrSpace() const
    {
        return m_viewSpace->m_xrSpace;
    }

    /////////////////////////////////////////////////
    /// OpenXRReferenceSpacesInterface overrides
    AZStd::vector<AZStd::string> ReferenceSpacesManager::GetReferenceSpaceNames() const
    {
        AZStd::vector<AZStd::string> retList;
        for (auto const& pair : m_spaces) {
            retList.push_back(pair.first);
        }
        return retList;
    }


    AZ::Outcome<bool, AZStd::string> ReferenceSpacesManager::AddReferenceSpace(ReferenceSpaceId referenceSpaceType,
        const AZStd::string& spaceName, const AZ::Transform& poseInReferenceSpace)
    {
        if (m_spaces.contains(spaceName))
        {
            return AZ::Failure(
                AZStd::string::format("A space named [%s] already exists", spaceName.c_str())
            );
        }
        XrSpace newXrSpace = CreateXrSpace(XrReferenceSpaceType(referenceSpaceType), poseInReferenceSpace);
        if (XR_NULL_HANDLE == newXrSpace)
        {
            return AZ::Failure(
                AZStd::string::format("Failed to create new XrSpace for space named [%s] "
                "with reference space type [%u]", spaceName.c_str(), referenceSpaceType)
            );
        }

        ReferenceSpace visualizedSpace{ spaceName, poseInReferenceSpace, newXrSpace };
        m_spaces.emplace(spaceName, AZStd::move(visualizedSpace));

        return AZ::Success(true);
    }


    AZ::Outcome<bool, AZStd::string> ReferenceSpacesManager::RemoveReferenceSpace(const AZStd::string& spaceName)
    {
        static const AZStd::unordered_set<AZStd::string> defaultSystemSpaces {
            {IOpenXRReferenceSpaces::ReferenceSpaceNameView },
            {IOpenXRReferenceSpaces::ReferenceSpaceNameLocal},
            {IOpenXRReferenceSpaces::ReferenceSpaceNameStage}
        };
        if (defaultSystemSpaces.contains(spaceName))
        {
            return AZ::Failure(
                AZStd::string::format("Can not delete space [%s] because it is a system space.", spaceName.c_str())
            );
        }

        // If the space we are about to remove is currently the base space for View Space pose location, then we will fail too.
        if (m_baseSpaceForViewSpace &&
            (m_baseSpaceForViewSpace->m_name == spaceName))
        {
            return AZ::Failure(AZStd::string::format("Can not remove space [%s] because it is the base space to locate the [%s] space pose.",
                spaceName.c_str(), IOpenXRReferenceSpaces::ReferenceSpaceNameView));
        }

        auto itor = m_spaces.find(spaceName);
        if (itor == m_spaces.end())
        {
            AZ_Warning(LogName, false, "A space named [%s] doesn't exist.", spaceName.c_str());
            return AZ::Success(true);
        }

        xrDestroySpace(itor->second.m_xrSpace);
        m_spaces.erase(itor);
        return AZ::Success(true);
    }


    const void * ReferenceSpacesManager::GetReferenceSpaceNativeHandle(const AZStd::string& spaceName) const
    {
        const auto spaceItor = m_spaces.find(spaceName);
        if (spaceItor == m_spaces.end())
        {
            return nullptr;
        }
        return static_cast<void*>(spaceItor->second.m_xrSpace);
    }


    AZ::Outcome<AZ::Transform, AZStd::string> ReferenceSpacesManager::GetReferenceSpacePose(const AZStd::string& spaceName,
        const AZStd::string& baseSpaceName) const
    {
        const auto spaceItor = m_spaces.find(spaceName);
        if (spaceItor == m_spaces.end())
        {
            return AZ::Failure(AZStd::string::format("Space named [%s] doesn't exist.",
                spaceName.c_str()));
        }

        const auto baseSpaceItor = m_spaces.find(baseSpaceName);
        if (baseSpaceItor == m_spaces.end())
        {
            return AZ::Failure(AZStd::string::format("Base space named [%s] doesn't exist.",
                baseSpaceName.c_str()));
        }


        XrSpaceLocation xrSpaceLocation{ XR_TYPE_SPACE_LOCATION };
        auto result = xrLocateSpace(spaceItor->second.m_xrSpace, baseSpaceItor->second.m_xrSpace, m_predictedDisplaytime, &xrSpaceLocation);
        if (IsError(result))
        {
            return AZ::Failure(
                AZStd::string::format("Failed to locate visualized space [%s] with base space [%s]. Got Error:[%s].",
                spaceName.c_str(), baseSpaceName.c_str(), GetResultString(result))
            );
        }
        return AZ::Success(AzTransformFromXrPose(xrSpaceLocation.pose));
    }


    AZ::Outcome<bool, AZStd::string> ReferenceSpacesManager::SetBaseSpaceForViewSpacePose(const AZStd::string& spaceName)
    {
        const auto baseSpaceItor = m_spaces.find(spaceName);
        if (baseSpaceItor == m_spaces.end())
        {
            return AZ::Failure(
                AZStd::string::format("Can not set new base space named [%s] because it doesn't exist.",
                spaceName.c_str())
            );
        }

        m_baseSpaceForViewSpace = &(baseSpaceItor->second);
        return AZ::Success(true);
    }


    const AZStd::string& ReferenceSpacesManager::GetBaseSpaceForViewSpacePose() const
    {
        AZ_Assert(m_baseSpaceForViewSpace != nullptr, "A base space is always expected to exist!");
        return m_baseSpaceForViewSpace->m_name;
    }


    const AZ::Transform& ReferenceSpacesManager::GetViewSpacePose() const
    {
        return m_viewSpacePose;
    }


    uint32_t ReferenceSpacesManager::GetViewCount() const
    {
        return aznumeric_cast<uint32_t>(m_eyeViewPoses.size());
    }


    const AZ::Transform& ReferenceSpacesManager::GetViewPose(uint32_t eyeIndex) const
    {
        if (eyeIndex >= m_eyeViewPoses.size())
        {
            // Debug and Profile builds shoud crash here.
            AZ_Assert(false, "Can't get View Pose because [%u] is out of bounds", eyeIndex);
            // In Release build we'd see this message.
            AZ_Error(LogName, false, "Can't get View Pose because [%u] is out of bounds", eyeIndex);
            return AZ::Transform::Identity();
        }

        return m_eyeViewPoses[eyeIndex];
    }


    const AZ::RPI::FovData& ReferenceSpacesManager::GetViewFovData(uint32_t eyeIndex) const
    {
        if (eyeIndex >= m_eyeViewPoses.size())
        {
            // Debug and Profile builds shoud crash here.
            AZ_Assert(false, "Can't get View FovData because [%u] is out of bounds", eyeIndex);
            // In Release build we'd see this message.
            AZ_Error(LogName, false, "Can't get View FovData because [%u] is out of bounds", eyeIndex);
            static const AZ::RPI::FovData ZeroFovData{};
            return ZeroFovData;
        }

        return m_eyeViewFovDatas[eyeIndex];
    }


    const AZStd::vector<AZ::Transform>& ReferenceSpacesManager::GetViewPoses() const
    {
        return m_eyeViewPoses;
    }


    void ReferenceSpacesManager::ForceViewPosesCacheUpdate()
    {
        XrViewState viewState{ XR_TYPE_VIEW_STATE };
        uint32_t viewCapacityInput = aznumeric_cast<uint32_t>(m_xrViews.size());
        uint32_t viewCountOutput = 0;

        XrViewLocateInfo viewLocateInfo{ XR_TYPE_VIEW_LOCATE_INFO };
        viewLocateInfo.viewConfigurationType = m_xrViewConfigurationType;
        viewLocateInfo.displayTime = m_predictedDisplaytime;
        viewLocateInfo.space = m_viewSpace->m_xrSpace;

        XrResult result = xrLocateViews(m_xrSession, &viewLocateInfo, &viewState, viewCapacityInput, &viewCountOutput, m_xrViews.data());
        if (IsError(result) || (viewCapacityInput != viewCountOutput))
        {
            PrintXrError(LogName, result, "xrLocateViews failed with viewCapacityInput=%u, viewCountOutput=%u",
                viewCapacityInput, viewCountOutput);
            return;
        }

        AZ_Warning(LogName, (viewState.viewStateFlags & XR_VIEW_STATE_POSITION_VALID_BIT) != 0, "View poses position data won't be valid");
        AZ_Warning(LogName, (viewState.viewStateFlags & XR_VIEW_STATE_ORIENTATION_VALID_BIT) != 0, "View poses orientation data won't be valid");

        for (size_t viewIdx = 0; viewIdx < m_xrViews.size(); viewIdx++)
        {
            m_eyeViewPoses[viewIdx] = AzTransformFromXrPose(m_xrViews[viewIdx].pose);
            auto& fovData = m_eyeViewFovDatas[viewIdx];
            const auto& xrFov = m_xrViews[viewIdx].fov;
            fovData.m_angleLeft = xrFov.angleLeft;
            fovData.m_angleRight = xrFov.angleRight;
            fovData.m_angleUp = xrFov.angleUp;
            fovData.m_angleDown = xrFov.angleDown;
        }
    }
    /// OpenXRReferenceSpacesInterface overrides
    /////////////////////////////////////////////////

    XrSpace ReferenceSpacesManager::CreateXrSpace(XrReferenceSpaceType referenceSpaceType, const AZ::Transform& relativePose)
    {
        XrReferenceSpaceCreateInfo referenceSpaceCreateInfo{ XR_TYPE_REFERENCE_SPACE_CREATE_INFO };
        referenceSpaceCreateInfo.poseInReferenceSpace = XrPoseFromAzTransform(relativePose);
        referenceSpaceCreateInfo.referenceSpaceType = referenceSpaceType;
        XrSpace newXrSpace = XR_NULL_HANDLE;
        XrResult result = xrCreateReferenceSpace(m_xrSession, &referenceSpaceCreateInfo, &newXrSpace);
        if (IsError(result))
        {
            PrintXrError(LogName, result, "Failed to create XrSpace using referenceSpaceType=%u.\n", referenceSpaceType);
            return XR_NULL_HANDLE;
        }
        return newXrSpace;
    }

} // namespace OpenXRVk
