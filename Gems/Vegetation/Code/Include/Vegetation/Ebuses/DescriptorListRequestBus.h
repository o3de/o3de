/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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