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
#include <AzCore/std/string/string.h>

namespace Vegetation
{
    class SurfaceMaskFilterRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetAllowOverrides() const = 0;
        virtual void SetAllowOverrides(bool value) = 0;

        virtual float GetInclusiveWeightMin() const = 0;
        virtual void SetInclusiveWeightMin(float value) = 0;

        virtual float GetInclusiveWeightMax() const = 0;
        virtual void SetInclusiveWeightMax(float value) = 0;

        virtual float GetExclusiveWeightMin() const = 0;
        virtual void SetExclusiveWeightMin(float value) = 0;

        virtual float GetExclusiveWeightMax() const = 0;
        virtual void SetExclusiveWeightMax(float value) = 0;

        virtual size_t GetNumInclusiveTags() const = 0;
        virtual AZ::Crc32 GetInclusiveTag(int tagIndex) const = 0;
        virtual void RemoveInclusiveTag(int tagIndex) = 0;
        virtual void AddInclusiveTag(AZStd::string tag) = 0;

        virtual size_t GetNumExclusiveTags() const = 0;
        virtual AZ::Crc32 GetExclusiveTag(int tagIndex) const = 0;
        virtual void RemoveExclusiveTag(int tagIndex) = 0;
        virtual void AddExclusiveTag(AZStd::string tag) = 0;
    };

    using SurfaceMaskFilterRequestBus = AZ::EBus<SurfaceMaskFilterRequests>;
}