/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

#include <GradientSignal/Util.h>

namespace GradientSignal
{
    enum class OutputFormat : AZ::u8
    {
        R8,
        R16,
        R32
    };

    class GradientBakerRequests
        : public AZ::ComponentBus
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual void BakeImage() = 0;

        virtual AZ::EntityId GetInputBounds() const = 0;
        virtual void SetInputBounds(const AZ::EntityId& inputBounds) = 0;

        virtual AZ::Vector2 GetOutputResolution() const = 0;
        virtual void SetOutputResolution(const AZ::Vector2& resolution) = 0;

        virtual OutputFormat GetOutputFormat() const = 0;
        virtual void SetOutputFormat(OutputFormat outputFormat) = 0;

        virtual AZ::IO::Path GetOutputImagePath() const = 0;
        virtual void SetOutputImagePath(const AZ::IO::Path& outputImagePath) = 0;
    };

    using GradientBakerRequestBus = AZ::EBus<GradientBakerRequests>;
}
