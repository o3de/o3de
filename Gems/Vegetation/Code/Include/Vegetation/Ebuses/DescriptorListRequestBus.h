/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace Vegetation
{
    struct Descriptor;

    enum class DescriptorListSourceType : AZ::u8
    {
        EMBEDDED = 0,
        EXTERNAL = 1,
    };

    class DescriptorListRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual DescriptorListSourceType GetDescriptorListSourceType() const = 0;
        virtual void SetDescriptorListSourceType(DescriptorListSourceType sourceType) = 0;

        virtual AZStd::string GetDescriptorAssetPath() const = 0;
        virtual void SetDescriptorAssetPath(const AZStd::string& assetPath) = 0;

        virtual size_t GetNumDescriptors() const = 0;
        virtual Descriptor* GetDescriptor(int index) = 0;
        virtual void RemoveDescriptor(int index) = 0;
        virtual void SetDescriptor(int index, Descriptor* descriptor) = 0;
        virtual void AddDescriptor(Descriptor* descriptor) = 0;
    };

    using DescriptorListRequestBus = AZ::EBus<DescriptorListRequests>;
}
