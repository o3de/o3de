/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace AWSCore
{
    //! AWS Script Behavior notifications for ScriptCanvas behaviors that interact with AWS S3
    class AWSScriptBehaviorS3Notifications
        : public AZ::EBusTraits
    {
    public:
        ///////////////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
        static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;

        //! Called when a successful script behavior s3 head object call has occurred
        virtual void OnHeadObjectSuccess(const AZStd::string& resultBody) = 0;

        //! Called when script behavior s3 head object call has failed
        virtual void OnHeadObjectError(const AZStd::string& errorBody) = 0;

        //! Called when a successful script behavior s3 get object call has occurred
        virtual void OnGetObjectSuccess(const AZStd::string& resultBody) = 0;

        //! Called when script behavior s3 get object call has failed
        virtual void OnGetObjectError(const AZStd::string& errorBody) = 0;
    };
    using AWSScriptBehaviorS3NotificationBus = AZ::EBus<AWSScriptBehaviorS3Notifications>;

    class AWSScriptBehaviorS3NotificationBusHandler
        : public AWSScriptBehaviorS3NotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSScriptBehaviorS3NotificationBusHandler, "{CB7E8710-F256-48A6-BC03-D3E3001AEB1E}",
            AZ::SystemAllocator, OnHeadObjectSuccess, OnHeadObjectError, OnGetObjectSuccess, OnGetObjectError);

        void OnHeadObjectSuccess(const AZStd::string& resultBody) override
        {
            Call(FN_OnHeadObjectSuccess, resultBody);
        }

        void OnHeadObjectError(const AZStd::string& errorBody) override
        {
            Call(FN_OnHeadObjectError, errorBody);
        }

        void OnGetObjectSuccess(const AZStd::string& resultBody) override
        {
            Call(FN_OnGetObjectSuccess, resultBody);
        }

        void OnGetObjectError(const AZStd::string& errorBody) override
        {
            Call(FN_OnGetObjectError, errorBody);
        }
    };

    class AWSScriptBehaviorS3
    {
        static constexpr const char AWSScriptBehaviorS3Name[] = "AWSScriptBehaviorS3";
        static constexpr const char OutputFileIsEmptyErrorMessage[] = "Request validation failed, output file is empty.";
        static constexpr const char OutputFileFailedToResolveErrorMessage[] = "Request validation failed, cannot resolve the output file path.";
        static constexpr const char OutputFileIsDirectoryErrorMessage[] = "Request validation failed, output file is a directory.";
        static constexpr const char OutputFileDirectoryNotExistErrorMessage[] = "Request validation failed, output file directory doesn't exist.";
        static constexpr const char OutputFileIsReadOnlyErrorMessage[] = "Request validation failed, output file is read-only.";
        static constexpr const char BucketNameIsEmptyErrorMessage[] = "Request validation failed, bucket name is empty";
        static constexpr const char ObjectKeyNameIsEmptyErrorMessage[] = "Request validation failed, object key name is empty.";
        static constexpr const char RegionNameIsEmptyErrorMessage[] = "Request validation failed, region name is empty.";

    public:
        AZ_RTTI(AWSScriptBehaviorS3, "{7F4E956C-7463-4236-B320-C992D36A9C6E}");

        AWSScriptBehaviorS3() = default;
        virtual ~AWSScriptBehaviorS3() = default;

        static void Reflect(AZ::ReflectContext* context);

        static void GetObject(const AZStd::string& bucketResourceKey, const AZStd::string& objectKey, const AZStd::string& outFile);
        static void GetObjectRaw(const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region, const AZStd::string& outFile);

        static void HeadObject(const AZStd::string& bucketResourceKey, const AZStd::string& objectKey);
        static void HeadObjectRaw(const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region);

    private:
        using S3NotificationFunctionType =  void(AWSScriptBehaviorS3Notifications::*)(const AZStd::string&);
        static bool ValidateGetObjectRequest(S3NotificationFunctionType notificationFunc,
            const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region, AZStd::string& outFile);

        static bool ValidateHeadObjectRequest(S3NotificationFunctionType notificationFunc,
            const AZStd::string& bucket, const AZStd::string& key, const AZStd::string& region);
    };
}
