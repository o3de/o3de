/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <GradientSignal/ImageAsset.h>
#include <GradientSignal/ImageSettings.h>

namespace GradientSignal
{
    AZStd::unique_ptr<ImageAsset> ConvertImage(const ImageAsset& image, const ImageSettings& settings);
}
