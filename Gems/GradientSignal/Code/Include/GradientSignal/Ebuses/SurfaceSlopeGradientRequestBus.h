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

namespace GradientSignal
{
    class SurfaceSlopeGradientRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual float GetSlopeMin() const = 0;
        virtual void SetSlopeMin(float slopeMin) = 0;

        virtual float GetSlopeMax() const = 0;
        virtual void SetSlopeMax(float slopeMax) = 0;

        virtual size_t GetNumTags() const = 0;
        virtual AZ::Crc32 GetTag(int tagIndex) const = 0;
        virtual void RemoveTag(int tagIndex) = 0;
        virtual void AddTag(AZStd::string tag) = 0;

        virtual AZ::u8 GetRampType() const = 0;
        virtual void SetRampType(AZ::u8 rampType) = 0;
    };

    using SurfaceSlopeGradientRequestBus = AZ::EBus<SurfaceSlopeGradientRequests>;
}