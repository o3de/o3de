/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>

#include <OpenXRVk/OpenXRVkInteractionProfilesAsset.h>
#include <OpenXRVk/OpenXRVkActionSetsAsset.h>

//! API that validates the content of InteractionProfiles assets and ActionSets assets.
//! In principle, this API doesn't belong in the main OpenXRVk Runtime, instead, it should be
//! private to the Asset Builders. BUT, at the moment we rely on the Asset Editor as a means to
//! provide the application developer with an UI to create these kind of assets. We need to make sure
//! the assets are valid before the user can save them from Asset Editor -> Save menu option to prevent
//! asset build failure which would cause these assets to become uneditable in the Asset Editor.
//! TODO: Develop a custom tool, either in C++ or Python to edit InteractionProfiles assets and ActionSets assets
//! and move this API to the OpenXRVk.Builders.Static target.
namespace OpenXRVkAssetsValidator
{
    AZ::Outcome<void, AZStd::string> ValidateInteractionProfilesAsset(
        const OpenXRVk::OpenXRInteractionProfilesAsset& interactionProfilesAsset);

    AZ::Outcome<void, AZStd::string> ValidateActionSetsAsset(const OpenXRVk::OpenXRActionSetsAsset& actionSetsAsset,
        const OpenXRVk::OpenXRInteractionProfilesAsset& interactionProfilesAsset);

}// namespace OpenXRVkAssetsValidator
