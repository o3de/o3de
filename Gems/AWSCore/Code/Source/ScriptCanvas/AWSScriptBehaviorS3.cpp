/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include <AzCore/IO/FileIO.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <fstream>

#include <Framework/AWSApiRequestJob.h>
#include <ResourceMapping/AWSResourceMappingBus.h>
#include <ScriptCanvas/AWSScriptBehaviorS3.h>


namespace AWSCore
{
    AWSScriptBehaviorS3::AWSScriptBehaviorS3()
    {
    }

    void AWSScriptBehaviorS3::ReflectSerialization(AZ::SerializeContext* serializeContext)
    {
        if (serializeContext)
        {
            serializeContext->Class<AWSScriptBehaviorS3>()
                ->Version(0);
        }
    }

    void AWSScriptBehaviorS3::ReflectBehaviors(AZ::BehaviorContext* behaviorContext)
    {
        behaviorContext->Class<AWSScriptBehaviorS3>("AWSScriptBehaviorS3")
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
                  {"Region Name", "The region of the bucket located in."}}})
            ;

        behaviorContext->EBus<AWSScriptBehaviorS3NotificationBus>("AWSS3BehaviorNotificationBus")
            ->Attribute(AZ::Script::Attributes::Category, "AWSCore")
            ->Handler<AWSScriptBehaviorS3NotificationBusHandler>();
    }

    void AWSScriptBehaviorS3::ReflectEditParameters(AZ::EditContext* editContext)
    {
        AZ_UNUSED(editContext);
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
        if (!ValidateGetObjectRequest(&AWSScriptBehaviorS3NotificationBus::Events::OnGetObjectError, bucket, objectKey, region, outFile))
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
        Aws::String outFileName(outFile.c_str());
        job->request.SetResponseStreamFactory([outFileName]() {
            return Aws::New<Aws::FStream>(
                "AWSScriptBehaviorS3", outFileName.c_str(),
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
        const AZStd::string& bucket, const AZStd::string& objectKey, const AZStd::string& region, const AZStd::string& outFile)
    {
        if (ValidateHeadObjectRequest(notificationFunc, bucket, objectKey, region))
        {
            if (!AzFramework::StringFunc::Path::IsValid(outFile.c_str()))
            {
                AZ_Warning("AWSScriptBehaviorS3", false, "Request validation failed, outfile is not valid.");
                AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, "Request validation failed, outfile is not valid.");
                return false;
            }
            if (AZ::IO::FileIOBase::GetInstance()->IsReadOnly(outFile.c_str()))
            {
                AZ_Warning("AWSScriptBehaviorS3", false, "Request validation failed, outfile is read-only.");
                AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, "Request validation failed, outfile is read-only.");
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
            AZ_Warning("AWSScriptBehaviorS3", false, "Request validation failed, bucket name is required.");
            AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, "Request validation failed, bucket name is required.");
            return false;
        }
        if (objectKey.empty())
        {
            AZ_Warning("AWSScriptBehaviorS3", false, "Request validation failed, object key name is required.");
            AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, "Request validation failed, object key name is required.");
            return false;
        }
        if (region.empty())
        {
            AZ_Warning("AWSScriptBehaviorS3", false, "Request validation failed, region name is required.");
            AWSScriptBehaviorS3NotificationBus::Broadcast(notificationFunc, "Request validation failed, region name is required.");
            return false;
        }
        return true;
    }
}
