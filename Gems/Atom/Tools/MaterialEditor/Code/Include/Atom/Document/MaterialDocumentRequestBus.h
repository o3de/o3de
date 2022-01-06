/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RPI.Public/Material/Material.h>
#include <Atom/RPI.Reflect/Material/MaterialAsset.h>
#include <Atom/RPI.Reflect/Material/MaterialPropertyDescriptor.h>
#include <AzCore/Asset/AssetCommon.h>

namespace AZ
{
    namespace RPI
    {
        class MaterialSourceData;
        class MaterialTypeSourceData;
    } // namespace RPI
} // namespace AZ

namespace MaterialEditor
{
    //! UVs are processed in a property group but will be handled differently.
    static constexpr const char UvGroupName[] = "uvSets";

    class MaterialDocumentRequests
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        typedef AZ::Uuid BusIdType;

        //! Get material asset created by MaterialDocument
        virtual AZ::Data::Asset<AZ::RPI::MaterialAsset> GetAsset() const = 0;

        //! Get material instance created from asset loaded by MaterialDocument
        virtual AZ::Data::Instance<AZ::RPI::Material> GetInstance() const = 0;

        //! Get the internal material source data
        virtual const AZ::RPI::MaterialSourceData* GetMaterialSourceData() const = 0;

        //! Get the internal material type source data
        virtual const AZ::RPI::MaterialTypeSourceData* GetMaterialTypeSourceData() const = 0;
    };

    using MaterialDocumentRequestBus = AZ::EBus<MaterialDocumentRequests>;
} // namespace MaterialEditor
