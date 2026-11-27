/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <openxr/openxr.h>

#include <OpenXRVk/OpenXRVkReferenceSpacesInterface.h>

namespace OpenXRVk
{
    //! Concrete implementation of OpenXRReferenceSpacesInterface.
    //! The OpenXRVk::Session class should own an instance of this class.
    //! Automatically instantiates the default system provided reference spaces
    //! as mandated by the OpenXR Spec: View, Local and Stage. 
    class ReferenceSpacesManager final :
        public OpenXRReferenceSpacesInterface::Registrar
    {
    public:
        AZ_CLASS_ALLOCATOR(ReferenceSpacesManager, AZ::SystemAllocator);
        AZ_RTTI(ReferenceSpacesManager, "{4BC4D0C0-02D4-4676-8352-8BC51306AF02}", IOpenXRReferenceSpaces)

        static constexpr char LogName[] = "OpenXRVkReferenceSpacesManager";

        //! Instantiates the OpenXR Guaranteed Reference Spaces: View, Local and Stage.
        //! And prepares all the internal data required to service the OpenXRReferenceSpacesInterface interface.
        bool Init(XrInstance xrInstance, XrSession xrSession, XrViewConfigurationType xrViewConfigurationType, uint32_t numEyeViews);

        //! Called by the Session each tick.
        bool SyncViews(XrTime predictedDisplayTime);

        //! The Session calls this function when it receives the XR_SESSION_STATE_READY
        //! event.
        void OnSessionReady();

        //! This is useful for the Swapchain system.
        const AZStd::vector<XrView>& GetXrViews() const;

        //! This is useful for the Swapchain system
        XrSpace GetViewSpaceXrSpace() const;

        /////////////////////////////////////////////////
        /// OpenXRReferenceSpacesInterface overrides
        AZStd::vector<AZStd::string> GetReferenceSpaceNames() const override;
        AZ::Outcome<bool, AZStd::string> AddReferenceSpace(ReferenceSpaceId referenceSpaceType,
            const AZStd::string& spaceName, const AZ::Transform& poseInReferenceSpace) override;
        AZ::Outcome<bool, AZStd::string> RemoveReferenceSpace(const AZStd::string& spaceName) override;

        const void * GetReferenceSpaceNativeHandle(const AZStd::string& spaceName) const override;

        AZ::Outcome<AZ::Transform, AZStd::string> GetReferenceSpacePose(const AZStd::string& spaceName,
            const AZStd::string& baseSpaceName) const override;

        AZ::Outcome<bool, AZStd::string> SetBaseSpaceForViewSpacePose(const AZStd::string& spaceName) override;
        const AZStd::string& GetBaseSpaceForViewSpacePose() const override;
        const AZ::Transform& GetViewSpacePose() const override;

        uint32_t GetViewCount() const override;
        const AZ::Transform& GetViewPose(uint32_t eyeIndex) const override;
        const AZ::RPI::FovData& GetViewFovData(uint32_t eyeIndex) const override;
        const AZStd::vector<AZ::Transform>& GetViewPoses() const override;

        void ForceViewPosesCacheUpdate() override;
        /// OpenXRReferenceSpacesInterface overrides
        /////////////////////////////////////////////////

    private:

        XrInstance m_xrInstance = XR_NULL_HANDLE;
        XrSession m_xrSession = XR_NULL_HANDLE;
        XrViewConfigurationType m_xrViewConfigurationType;

        // Updated each time the Session calls UpdateViewSpacePoseAndEyeViewPoses().
        XrTime m_predictedDisplaytime;

        struct ReferenceSpace
        {
            AZStd::string m_name;
            //! We shave this transform in case we have to reset the spaces.
            AZ::Transform m_offsetPose;
            // Runtime data
            XrSpace m_xrSpace;
        };

        // At the bare minimum this map will always contain three spaces that can not be removed:
        // "View", "Local" and "Stage".
        AZStd::unordered_map<AZStd::string, ReferenceSpace> m_spaces;

        // We cache here the base space what will be used to "locate" the View Space pose.
        const ReferenceSpace* m_baseSpaceForViewSpace;
        const ReferenceSpace* m_viewSpace; // Cached for convenience. This pointer is set once during initialization and never changes.

        AZ::Transform m_viewSpacePose;
        //! The following poses are always relative to @m_viewSpacePose.
        AZStd::vector<AZ::Transform> m_eyeViewPoses;
        AZStd::vector<AZ::RPI::FovData> m_eyeViewFovDatas;
        AZStd::vector<XrView> m_xrViews;

        //! Helper function to create an XrSpace.
        XrSpace CreateXrSpace(XrReferenceSpaceType referenceSpaceType, const AZ::Transform& relativePose);
    };
}
