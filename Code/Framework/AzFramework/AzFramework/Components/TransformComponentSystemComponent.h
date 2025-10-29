/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/string/string.h>

namespace AzFramework
{
    static constexpr AZ::Crc32 PARENT_ACTIVE_TYPE_NAME = AZ_CRC_CE("Parent");

    //! The System Component that handles and defines Transform Parent Activation Type.
    class AZF_API TransformComponentSystemComponent : public AZ::Component
    {
    public:
        AZ_COMPONENT(TransformComponentSystemComponent, "{13EFE42F-60C8-4BF2-B895-6BDD8345D2A4}", AZ::Component);

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}
    };
} // namespace AzFramework
