/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/parallel/mutex.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Math/Uuid.h>

namespace AssetBuilderSDK
{
    struct CreateJobsRequest;
    struct CreateJobsResponse;
    struct ProcessJobRequest;
    struct ProcessJobResponse;
    struct AssetBuilderDesc;

    //! This EBUS is used to send commands from the assetprocessor to the builder
    //! Every new builder should implement a listener for this bus and implement the CreateJobs, Shutdown and ProcessJobs functions.
    class AssetBuilderCommandBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef AZ::Uuid BusIdType;

        virtual ~AssetBuilderCommandBusTraits() {};

        //! Shutdown() REQUIRED - Handle the message indicating shutdown.  Cancel all your tasks and get them stopped ASAP
        //! this message will come in from a different thread than your ProcessJob() thread.
        //! failure to terminate promptly can cause a hangup on AP shutdown and restart.
        virtual void ShutDown() = 0;
    };

    typedef AZ::EBus<AssetBuilderCommandBusTraits> AssetBuilderCommandBus;

    //!This EBUS is used to send information from the builder to the AssetProcessor
    class AssetBuilderBusTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        typedef AZStd::recursive_mutex MutexType;

        virtual ~AssetBuilderBusTraits() {}

        virtual bool FindBuilderInformation(const AZ::Uuid& /*builderGuid*/, AssetBuilderDesc& /*descriptionOut*/) { return false; }

        // Use this function to send AssetBuilderDesc info to the assetprocessor
        virtual void RegisterBuilderInformation(const AssetBuilderDesc& /*builderDesc*/) {}

        // Use this function to register all the component descriptors
        virtual void RegisterComponentDescriptor(AZ::ComponentDescriptor* /*descriptor*/) {}

        // Log functions to report general builder related messages/error.
        virtual void BuilderLog(const AZ::Uuid& /*builderId*/, const char* /*message*/, ...) {}
        virtual void BuilderLogV(const AZ::Uuid& /*builderId*/, const char* /*message*/, va_list /*list*/) {}
    };

    typedef AZ::EBus<AssetBuilderBusTraits> AssetBuilderBus;

    //! This EBus provides builders access to the Asset Builders issue tracking facilities.
    class AssetBuilderTraceTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        
        virtual ~AssetBuilderTraceTraits() = default;

        //! The next <count> requests that the Asset Builder gets to forward errors to the console
        //! will be ignored.
        virtual void IgnoreNextErrors(AZ::u32 count) = 0;
        //! The next <count> requests that the Asset Builder gets to forward warnings to the console
        //! will be ignored.
        virtual void IgnoreNextWarning(AZ::u32 count) = 0;
        //! The next <count> requests that the Asset Builder gets to forward prints to the console
        //! will be ignored.
        virtual void IgnoreNextPrintf(AZ::u32 count) = 0;

        virtual void ResetWarningCount() = 0;
        virtual void ResetErrorCount() = 0;
        virtual AZ::u32 GetWarningCount() = 0;
        virtual AZ::u32 GetErrorCount() = 0;
    };

    typedef AZ::EBus<AssetBuilderTraceTraits> AssetBuilderTraceBus;

    //! This EBUS is used to send commands from the assetprocessor to a specific job
    class JobCommandTraits
        : public AZ::EBusTraits
    {
    public:
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        typedef AZStd::recursive_mutex MutexType;
        typedef AZ::s64 BusIdType;

        virtual ~JobCommandTraits() {}
        //! Handle the message indicating that the specific job needs to cancel.
        virtual void Cancel() {}
    };

    typedef AZ::EBus<JobCommandTraits> JobCommandBus;
}
