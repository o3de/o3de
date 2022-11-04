/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Name/Name.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/std/containers/vector.h>
#include <Atom/RPI.Public/Image/ImageQuality.h>

namespace AZ
{
    namespace RPI
    {
        class ImageTagInterface
            : public AZ::EBusTraits
        {
        public:
            virtual ImageQuality GetQuality(const AZ::Name& imageTag) const = 0;

            virtual AZStd::vector<AZ::Name> GetTags() const = 0;

            virtual void RegisterImageAsset(AZ::Name imageTag, const Data::AssetId& assetId) = 0;

            virtual void RegisterTag(AZ::Name tag) = 0;

            virtual void SetQuality(const AZ::Name& imageTag, ImageQuality quality) = 0;
        };

        using ImageTagBus = AZ::EBus<ImageTagInterface>;
    }
}
