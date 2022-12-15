/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Serialization/SerializeContext.h>

namespace AzToolsFramework
{
    struct IUuid
    {
        AZ_RTTI(IUuid, "{C3281F96-430A-49C2-95EF-66505C34CD2E}");
        virtual ~IUuid() = default;

        //! Gets the UUID for a source asset
        //! @param absoluteFilePath Absolute path to the source asset
        //! @return
        virtual AZ::Uuid GetSourceUuid(AZ::IO::PathView absoluteFilePath) = 0;
        //! Assigns the provided UUID to a source asset
        //! @param absoluteFilePath Absolute path to the source asset
        //! @param uuid Uuid to assign
        //! @return
        virtual bool CreateSourceUuid(AZ::IO::PathView absoluteFilePath, AZ::Uuid uuid) = 0;
        //! Assigns a randomly generated UUID to a source asset
        //! @param absoluteFilePath Absolute path to the source asset
        //! @return
        virtual AZ::Uuid CreateSourceUuid(AZ::IO::PathView absoluteFilePath) = 0;
    };

    class UuidUtilComponent
        : public AZ::Component
        , AZ::Interface<IUuid>::Registrar
    {
    public:
        AZ_COMPONENT(UuidUtilComponent, "{357CAA8E-CDDB-4094-8C2A-669D3D33E308}");

        static constexpr const char* UuidKey = "/UUID";

        static void Reflect(AZ::ReflectContext* context);

        AZ::Uuid GetSourceUuid(AZ::IO::PathView absoluteFilePath) override;
        bool CreateSourceUuid(AZ::IO::PathView absoluteFilePath, AZ::Uuid uuid) override;
        AZ::Uuid CreateSourceUuid(AZ::IO::PathView absoluteFilePath) override;

        // Inherited via Component
        void Activate() override{}
        void Deactivate() override{}
    };
} // namespace AzToolsFramework
