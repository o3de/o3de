/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef ASSETBUILDERUTILEBUSHELPER_H
#define ASSETBUILDERUTILEBUSHELPER_H

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Math/Uuid.h>

namespace AssetBuilderSDK
{
    //!This EBUS is used to send commands from the assetprocessor to the builder
    class AssetBuilderCommandBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef AZ::Uuid BusIdType;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~AssetBuilderCommandBusTraits() {}

        //Shutdown the builder.
        virtual void ShutDown() {}
    };

    typedef AZ::EBus<AssetBuilderCommandBusTraits> AssetBuilderCommandBus;

    //!Information that builders will send to the assetprocessor
    struct AssetBuilderDesc
    {
        AZStd::string m_name;//builder name
        AZStd::string m_regex;//builder regex
        AZ::Uuid m_busId;// builder id
    };

    //!This EBUS is used to send information from the builder to the AssetProcessor
    class AssetBuilderBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~AssetBuilderBusTraits() {}

        //Use this function to send AssetBuilderDesc info to the assetprocessor
        virtual void RegisterBuilderInformation(AssetBuilderDesc builderDesc) {}

        //Use this function to register all the component descriptors
        virtual void RegisterComponentDescriptor(AZ::ComponentDescriptor* descriptor) {}
    };

    typedef AZ::EBus<AssetBuilderBusTraits> AssetBuilderBus;
}


#endif //ASSETBUILDERUTILEBUSHELPER_H
