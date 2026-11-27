/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <OpenXRVk/OpenXRVkAssetsValidator.h>

TEST(OpenXRVkLoader, ResolvesGlobalEntryPointWithoutRuntime)
{
    PFN_xrVoidFunction createInstance = nullptr;
    ASSERT_EQ(xrGetInstanceProcAddr(XR_NULL_HANDLE, "xrCreateInstance", &createInstance), XR_SUCCESS);
    EXPECT_NE(createInstance, nullptr);
}

TEST(OpenXRVkAssetValidation, AcceptsValidProfileAndRejectsDuplicatePath)
{
    OpenXRVk::OpenXRInteractionProfilesAsset profiles;
    auto& profile = profiles.m_interactionProfileDescriptors.emplace_back();
    profile.m_name = "Khronos Simple";
    profile.m_path = "/interaction_profiles/khr/simple_controller";
    auto& userPath = profile.m_userPathDescriptors.emplace_back();
    userPath.m_name = "Left hand";
    userPath.m_path = "/user/hand/left";
    auto& componentPath = profile.m_commonComponentPathDescriptors.emplace_back();
    componentPath.m_name = "Select";
    componentPath.m_path = "/input/select/click";
    componentPath.m_actionTypeStr = OpenXRVk::OpenXRInteractionComponentPathDescriptor::s_TypeBoolStr;

    ASSERT_TRUE(OpenXRVkAssetsValidator::ValidateInteractionProfilesAsset(profiles).IsSuccess());
    auto duplicate = profile;
    duplicate.m_name = "Duplicate profile";
    profiles.m_interactionProfileDescriptors.push_back(AZStd::move(duplicate));
    EXPECT_FALSE(OpenXRVkAssetsValidator::ValidateInteractionProfilesAsset(profiles).IsSuccess());
}

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
