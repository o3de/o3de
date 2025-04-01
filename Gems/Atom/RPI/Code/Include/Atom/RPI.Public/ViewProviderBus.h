/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/base.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <Atom/RPI.Public/Base.h>

namespace AZ
{
    namespace RPI
    {
        //! Enum to describe view type
        enum class ViewType : uint32_t
        {
            Default = 0,
            XrLeft,
            XrRight,
            Count
        };

        static constexpr uint32_t DefaultViewType = static_cast<uint32_t>(ViewType::Default);
        static constexpr uint32_t MaxViewTypes = static_cast<uint32_t>(ViewType::Count);

        //! Interface for component which may provide a RPI view. 
        class ViewProvider
            : public AZ::EBusTraits
        {
        public:
            // EBus Configuration
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = AZ::EntityId;

            virtual ViewPtr GetView() const = 0;
            virtual ViewPtr GetStereoscopicView(RPI::ViewType viewType) const = 0;
        };

        using ViewProviderBus = AZ::EBus<ViewProvider>;
    } // namespace RPI
} // namespace AZ
