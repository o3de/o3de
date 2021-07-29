/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <aws/core/auth/AWSCredentials.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <fstream>

#include <Framework/AWSApiRequestJob.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <ScriptCanvas/AWSScriptBehaviorS3.h>


namespace AWSCore
{
    void AWSScriptBehaviorS3::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<AWSScriptBehaviorS3>()
                ->Version(0);
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<AWSScriptBehaviorS3>(AWSScriptBehaviorS3Name)
                ->Attribute(AZ::Script::Attributes::Category, "AWSCore")
                ->Method("GetObject", &AWSScriptBehaviorS3::GetObject,
                    {{{"Bucket Resource KeyName", "The resource key name of the bucket in resource mapping config file."},
                      {"Object KeyName", "The object key."},
                      {"Outfile Name", "Filename where the content will be saved."}}})
                ->Method("GetObjectRaw", &AWSScriptBehaviorS3::GetObjectRaw,
                    {{{"Bucket Name", "The name of the bucket containing the object."},
                      {"Object KeyName", "The object key."},
                      {"Region Name", "The region of the bucket located in."},
                      {"Outfile Name", "Filename where the content will be saved."}}})
                ->Method("HeadObject", &AWSScriptBehaviorS3::HeadObject,
                    {{{"Bucket Resource KeyName", "The resource key name of the bucket in resource mapping config file."},
                      {"Object KeyName", "The object key."}}})
                ->Method("HeadObjectRaw", &AWSScriptBehaviorS3::HeadObjectRaw,
                    {{{"Bucket Name", "The name of the bucket containing the object."},
                      {"Object KeyName", "The object key."},
                      {"Region Name", "The region of the bucket located in."}}});

            behaviorContext->EBus<AWSScriptBehaviorS3NotificationBus>("AWSS3BehaviorNotificationBus")
                ->Attribute(AZ::Script::Attributes::Category, "AWSCore")
                ->Handler<AWSScriptBehaviorS3NotificationBusHandler>();
        }
    }

    void AWSScriptBehaviorS3::GetObject(
        const AZStd::string& bucketResourceKey, const AZStd::string& objectKey, const AZStd::string& outFile)
    {
        AZStd::string requestBucket = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestBucket, &AWSResourceMappingRequests::GetResourceNameId, bucketResourceKey);
        AZStd::string requestRegion = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestRegion, &AWSResourceMappingRequests::GetResourceRegion, bucketResourceKey);
        GetObjectRaw(requestBucket, objectKey, requestRegion, outFile);
    }

    void AWSScriptBehaviorS3::GetObjectRaw(
        const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region, const AZStd::string& outFile)
    {
        AZStd::string normalizedOutFile = outFile;
        if (!ValidateGetObjectRequest(
                &AWSScriptBehaviorS3NotificationBus::Events::OnGetObjectError, bucket, objectKey, region, normalizedOutFile))
        {
            return;
        }

        using S3GetObjectRequestJob = AWS_API_REQUEST_JOB(S3, GetObject);
        S3GetObjectRequestJob::Config config(S3GetObjectRequestJob::GetDefaultConfig());
        config.region = region.c_str();

        auto job = S3GetObjectRequestJob::Create(
            [objectKey](S3GetObjectRequestJob*) // OnSuccess handler
            {
                AWSScriptBehaviorS3NotificationBus::Broadcast(
                    &AWSScriptBehaviorS3NotificationBus::Events::OnGetObjectSuccess,
                    AZStd::string::format("Object %s is downloaded.", objectKey.c_str()));
            },
            [](S3GetObjectRequestJob* job) // OnError handler
            {
                Aws::String errorMessage = job->error.GetMessage();
                AWSScriptBehaviorS3NotificationBus::Broadcast(
                    &AWSScriptBehaviorS3NotificationBus::Events::OnGetObjectError, errorMessage.c_str());
            },
            &config);

        job->request.SetBucket(Aws::String(bucket.c_str()));
        job->request.SetKey(Aws::String(objectKey.c_str()));
        Aws::String outFileName(normalizedOutFile.c_str());
        job->request.SetResponseStreamFactory([outFileName]() {
            return Aws::New<Aws::FStream>(
                AWSScriptBehaviorS3Name, outFileName.c_str(),
                std::ios_base::out | std::ios_base::in | std::ios_base::binary | std::ios_base::trunc);
        });
        job->Start();
    }

    void AWSScriptBehaviorS3::HeadObject(const AZStd::string& bucketResourceKey, const AZStd::string& objectKey)
    {
        AZStd::string requestBucket = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestBucket, &AWSResourceMappingRequests::GetResourceNameId, bucketResourceKey);
        AZStd::string requestRegion = "";
        AWSResourceMappingRequestBus::BroadcastResult(requestRegion, &AWSResourceMappingRequests::GetResourceRegion, bucketResourceKey);
        HeadObjectRaw(requestBucket, objectKey, requestRegion);
    }

    void AWSScriptBehaviorS3::HeadObjectRaw(
        const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region)
    {
        if (!ValidateHeadObjectRequest(&AWSScriptBehaviorS3NotificationBus::Events::OnHeadObjectError, bucket, objectKey, region))
        {
            return;
        }

        using S3HeadObjectRequestJob = AWS_API_REQUEST_JOB(S3, HeadObject);
        S3HeadObjectRequestJob::Config config(S3HeadObjectRequestJob::GetDefaultConfig());
        config.region = region.c_str();

        auto job = S3HeadObjectRequestJob::Create(
            [objectKey](S3HeadObjectRequestJob*) // OnSuccess handler
            {
                AWSScriptBehaviorS3NotificationBus::Broadcast(
                    &AWSScriptBehaviorS3NotificationBus::Events::OnHeadObjectSuccess,
                    AZStd::string::format("Object %s is found.", objectKey.c_str()));
            },
            [](S3HeadObjectRequestJob* job) // OnError handler
            {
                Aws::String errorMessage = job->error.GetMessage();
                AWSScriptBehaviorS3NotificationBus::Broadcast(
                    &AWSScriptBehaviorS3NotificationBus::Events::OnHeadObjectError, errorMessage.c_str());
            },
            &config);

        job->request.SetBucket(Aws::String(bucket.c_str()));
        job->request.SetKey(Aws::String(objectKey.c_str()));
        job->Start();
    }

    bool AWSScriptBehaviorS3::ValidateGetObjectRequest(S3NotificationFunctionType notificationFunc,
        const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region, AZStd::string& outFile)
    {
        if (ValidateHeadObjectRequest(notificationFunc, bucket, objectKey, region))
        {
            AzFramework::StringFunc::Path::Normalize(outFile);
            if (outFile.empty())
            {
                AZ_Warning(AWSScriptBehaviorS3Name, false, OutputFileIsEmptyErrorMessage);
                AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, OutputFileIsEmptyErrorMessage);
                return false;
            }

            char resolvedPath[AZ_MAX_PATH_LEN] = { 0 };
            if (!AZ::IO::FileIOBase::GetInstance()->ResolvePath(outFile.c_str(), resolvedPath, AZ_MAX_PATH_LEN))
            {
                AZ_Warning(AWSScriptBehaviorS3Name, false, OutputFileFailedToResolveErrorMessage);
                AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, OutputFileFailedToResolveErrorMessage);
                return false;
            }
            outFile = resolvedPath;

            if (AZ::IO::FileIOBase::GetInstance()->IsDirectory(outFile.c_str()))
            {
                AZ_Warning(AWSScriptBehaviorS3Name, false, OutputFileIsDirectoryErrorMessage);
                AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, OutputFileIsDirectoryErrorMessage);
                return false;
            }
            auto lastSeparator = outFile.find_last_of(AZ_CORRECT_FILESYSTEM_SEPARATOR);
            if (lastSeparator != AZStd::string::npos)
            {
                auto parentPath = outFile.substr(0, lastSeparator);
                if (!AZ::IO::FileIOBase::GetInstance()->Exists(parentPath.c_str()))
                {
                    AZ_Warning(AWSScriptBehaviorS3Name, false, OutputFileDirectoryNotExistErrorMessage);
                    AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, OutputFileDirectoryNotExistErrorMessage);
                    return false;
                }
            }
            if (AZ::IO::FileIOBase::GetInstance()->IsReadOnly(outFile.c_str()))
            {
                AZ_Warning(AWSScriptBehaviorS3Name, false, OutputFileIsReadOnlyErrorMessage);
                AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, OutputFileIsReadOnlyErrorMessage);
                return false;
            }
            return true;
        }
        return false;
    }

    bool AWSScriptBehaviorS3::ValidateHeadObjectRequest(S3NotificationFunctionType notificationFunc,
        const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region)
    {
        if (bucket.empty())
        {
            AZ_Warning(AWSScriptBehaviorS3Name, false, BucketNameIsEmptyErrorMessage);
            AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, BucketNameIsEmptyErrorMessage);
            return false;
        }
        if (objectKey.empty())
        {
            AZ_Warning(AWSScriptBehaviorS3Name, false, ObjectKeyNameIsEmptyErrorMessage);
            AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, ObjectKeyNameIsEmptyErrorMessage);
            return false;
        }
        if (region.empty())
        {
            AZ_Warning(AWSScriptBehaviorS3Name, false, RegionNameIsEmptyErrorMessage);
            AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, RegionNameIsEmptyErrorMessage);
            return false;
        }
        return true;
    }
}
