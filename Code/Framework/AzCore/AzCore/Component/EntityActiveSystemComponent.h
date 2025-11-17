/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityActiveSystemBus.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    static constexpr size_t ENTITY_ACTIVE_TYPE_INDEX = 0;
    static constexpr AZ::Crc32 ENTITY_ACTIVE_TYPE_NAME = AZ_CRC_CE("Entity");

    //! The System Component that handles and defines Entity Activation Layers.
    class AZCORE_API EntityActiveSystemComponent
        : public Component
        , public EntityActiveSystemRequestBus::Handler
    {
    public:
        AZ_COMPONENT(EntityActiveSystemComponent, "{AAC78518-BC7B-42DD-8257-7F15C22AD0E3}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        static constexpr size_t kInvalidIndex = std::numeric_limits<size_t>::max();
        
        size_t GetActiveTypeIndexByName(AZStd::string typeName) override;
        size_t GetActiveTypeIndexById(AZ::Crc32 typeNameId) override;

        size_t ScanListForIndex(AZ::Crc32 typeNameId);
    private:
        AZStd::vector<AZ::Crc32> m_activeTypeNameToIndex = { ENTITY_ACTIVE_TYPE_NAME };
        static constexpr size_t s_maxStateFlags = 32;
    };
} // namespace AZ
