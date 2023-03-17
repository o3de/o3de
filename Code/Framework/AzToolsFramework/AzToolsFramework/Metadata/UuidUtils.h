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

namespace AZ
{
    class ReflectContext;
}
namespace AzToolsFramework
{
    struct IUuidUtil
    {
        AZ_RTTI(IUuidUtil, "{C3281F96-430A-49C2-95EF-66505C34CD2E}");
        virtual ~IUuidUtil() = default;

        //! Creates a metadata file with a randomly generated UUID.  This will fail if a metadata file with a UUID already exists for this asset.
        //! @param absoluteFilePath Absolute path of the source asset (or metadata file)
        //! @param uuid Uuid to assign
        virtual AZ::Outcome<void, AZStd::string> CreateSourceUuid(AZ::IO::PathView absoluteFilePath, AZ::Uuid uuid) = 0;
        //! Creates a metadata file with the assigned UUID.  This will fail if a metadata file with a UUID already exists for this asset.
        //! @param absoluteFilePath Absolute path of the source asset (or metadata file)
        //! @return The generated UUID if successful, a null UUID otherwise.
        virtual AZ::Outcome<AZ::Uuid, AZStd::string> CreateSourceUuid(AZ::IO::PathView absoluteFilePath) = 0;
    };

    class UuidUtilComponent
        : public AZ::Component
        , AZ::Interface<IUuidUtil>::Registrar
    {
    public:
        AZ_COMPONENT(UuidUtilComponent, "{357CAA8E-CDDB-4094-8C2A-669D3D33E308}");

        static constexpr const char* UuidKey = "/UUID";

        static void Reflect(AZ::ReflectContext* context);

        AZ::Outcome<void, AZStd::string> CreateSourceUuid(AZ::IO::PathView absoluteFilePath, AZ::Uuid uuid) override;
        AZ::Outcome<AZ::Uuid, AZStd::string> CreateSourceUuid(AZ::IO::PathView absoluteFilePath) override;

        // Inherited via Component
        void Activate() override{}
        void Deactivate() override{}
    };
} // namespace AzToolsFramework
