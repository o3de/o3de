/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <CustomAssetExample/Builder/CustomAssetExampleBuilderWorker.h>

#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace CustomAssetExample
{
    // Note - Shutdown will be called on a different thread than your process job thread
    void ExampleBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    // CreateJobs will be called early on in the file scanning pass from the Asset Processor.
    // You should create the same jobs, and avoid checking whether the job is up to date or not.  The Asset Processor will manage this for you
    void ExampleBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);

        // Our *.example extension details a source file with NO dependencies.
        // Here we simply create the JobDescriptors for each enabled platform in order to process the source file.
        if (AzFramework::StringFunc::Equal(ext.c_str(), "example"))
        {
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                // We create a simple job here which only contains the identifying job key and the platform to process the file on
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                // Note that there are additional parameters for the JobDescriptor which may be beneficial in your use case.
                // Notable ones include:
                //   * m_critical - a boolean that flags this job as one which must complete before the Editor will start up.
                //   * m_priority - an integer where larger values signify that the job should be processed with higher priority than those with lower values.
                // Please see the JobDescriptor for the full complement of configuration parameters.
                response.m_createJobOutputs.push_back(descriptor);

                // One builder can make multiple jobs for the same source file, for the same platform, as long as it emits a different job key per job. 
                // This allows you to break large compilations up into smaller jobs.  Jobs emitted in this manner may be run in parallel
                descriptor.m_jobKey = "Second Compile Example";

                // Custom parameters that you may need to know about when the job processes can be added to m_jobParameters
                descriptor.m_jobParameters[AZ_CRC_CE("hello")] = "World";
                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        // Our *.examplesource extension details a source file with dependencies.
        // Here we declare source file dependencies and forward the info to the Asset Processor
        // This example creates the following dependencies:
        //     * the source file .../test.examplesource depends on the source file  .../test.exampleinclude 
        //     * the source file .../test.exampleinclude depends on the source file .../common.exampleinclude
        //     * the source file .../common.exampleinclude depends on the non-source file .../common.examplefile

        // Note - both file extensions "exampleinclude" and "examplesource" are handled by this builder class.
        // However, files with extension "exampleinclude" do not create JobDescriptors, so they are not actually being processed by this builder.
        // We are only collecting their dependencies here.
        AssetBuilderSDK::SourceFileDependency sourceFileDependencyInfo;
        AZStd::string fullPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath, false);
        AzFramework::StringFunc::Path::Normalize(fullPath);
        AZStd::string relPath = request.m_sourceFile;

        // Source files in this example generate dependencies on a file with the same name, but having "exampleinclude" extensions
        if (AzFramework::StringFunc::Equal(ext.c_str(), "examplesource"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(relPath, "exampleinclude");
            // Declare and add the dependency on the *.exampleinclude file:
            sourceFileDependencyInfo.m_sourceFileDependencyPath = relPath;
            response.m_sourceFileDependencyList.push_back(sourceFileDependencyInfo);

            // Since we're a source file, we also add a job to do the actual compilation (for each enabled platform)
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());

                // you can also place whatever parameters you want to save for later into this map:
                descriptor.m_jobParameters[AZ_CRC_CE("hello")] = "World";
                response.m_createJobOutputs.push_back(descriptor);
            }

            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        if (AzFramework::StringFunc::Equal(ext.c_str(), "exampleinclude"))
        {
            if (AzFramework::StringFunc::Find(request.m_sourceFile.c_str(), "common.exampleinclude") != AZStd::string::npos)
            {
                // Add any dependencies that common.exampleinclude would like to depend on here, we can also add a non source file as a dependency like we are doing here
                AzFramework::StringFunc::Path::ReplaceFullName(fullPath, "common.examplefile");
                sourceFileDependencyInfo.m_sourceFileDependencyPath = fullPath;
            }
            else
            {
                AzFramework::StringFunc::Path::ReplaceFullName(fullPath, "common.exampleinclude");
                // Assigning full path to sourceFileDependency path
                sourceFileDependencyInfo.m_sourceFileDependencyPath = fullPath;
            }

            response.m_sourceFileDependencyList.push_back(sourceFileDependencyInfo);
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }
        // This example shows how you would be able to declare job dependencies on source files inside a builder and
        // forward that info to the asset processor.
        // Basically here we are creating a job dependency such that the job with source file ./test.examplejob and 
        // jobKey "Compile Example" depends on the fingerprint of the job with source file ./test.examplesource and jobkey "Compile Example". 

        else if (AzFramework::StringFunc::Equal(ext.c_str(), "examplejob"))
        {
            for (const AssetBuilderSDK::PlatformInfo& platformInfo : request.m_enabledPlatforms)
            {
                AssetBuilderSDK::JobDescriptor descriptor;
                descriptor.m_jobKey = "Compile Example";
                descriptor.SetPlatformIdentifier(platformInfo.m_identifier.c_str());
                AssetBuilderSDK::SourceFileDependency sourceFile;
                sourceFile.m_sourceFileDependencyPath = "test.examplesource";
                AssetBuilderSDK::JobDependency jobDependency("Compile Example", platformInfo.m_identifier.c_str(), AssetBuilderSDK::JobDependencyType::Fingerprint, sourceFile);
                descriptor.m_jobDependencyList.push_back(jobDependency);
                response.m_createJobOutputs.push_back(descriptor);
            }
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
            return;
        }

        AZ_Assert(false, "Unhandled extension type in CustomExampleAssetBuilderWorker.");
        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Failed;
    }

    // In this example builder class, we just copy the source file to a modified destination path in the temp directory.
    // Later on, this function will be called for jobs the Asset Processor has determined need to be run.
    // The request will contain the CreateJobResponse you constructed earlier, including any key value pairs you placed into m_jobParameters
    void ExampleBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        // This is the most basic example of handling for cancellation requests.
        // If possible, you should listen for cancellation requests and then cancel processing work to facilitate faster shutdown of the Asset Processor
        // If you need to do more things such as signal a semaphore or other threading work, derive from the Job Cancel Listener and reimplement Cancel()
        AssetBuilderSDK::JobCancelListener jobCancelListener(request.m_jobId);

        // Use AZ_TracePrintf to communicate job details. The logging system will automatically file the text under the appropriate log file and category.
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");

        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);
        AZStd::string ext;
        AzFramework::StringFunc::Path::GetExtension(request.m_sourceFile.c_str(), ext, false);

        if (AzFramework::StringFunc::Equal(ext.c_str(), "example"))
        {
            if (AzFramework::StringFunc::Equal(request.m_jobDescription.m_jobKey.c_str(), "Compile Example"))
            {
                AzFramework::StringFunc::Path::ReplaceExtension(fileName, "example1");
            }
            else if (AzFramework::StringFunc::Equal(request.m_jobDescription.m_jobKey.c_str(), "Second Compile Example"))
            {
                AzFramework::StringFunc::Path::ReplaceExtension(fileName, "example2");
            }
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "examplesource"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "examplesourceprocessed");
        }
        else if (AzFramework::StringFunc::Equal(ext.c_str(), "examplejob"))
        {
            AzFramework::StringFunc::Path::ReplaceExtension(fileName, "examplejobprocessed");
        }

        // All your work should happen inside the tempDirPath.
        // The Asset Processor will handle taking the completed files you specify in JobProduct.m_outputProducts from the temp directory into the cache.
        AZStd::string destPath;
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), destPath, true);

        // Check if we are cancelled or shutting down before doing intensive processing on this source file
        if (jobCancelListener.IsCancelled())
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancel was requested for job %s.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }
        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZ::IO::LocalFileIO fileIO;
        if (fileIO.Copy(request.m_fullPath.c_str(), destPath.c_str()) != AZ::IO::ResultCode::Success)
        {
            AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Error during processing job %s.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        // Push all products successfully built into the JobProduct.m_outputProducts.
        // Filepaths can be absolute, or relative to your temporary directory.
        // The job request struct has the temp directory, so it will be properly reconstructed to an absolute path later.
        AssetBuilderSDK::JobProduct jobProduct(fileName);

        // Note - you must also add the asset type to the JobProduct.
        // If you have direct access to the type within your gem, you can grab the asset type directly:
        //     jobProduct.m_productAssetType = return AZ::AzTypeInfo<CustomAssetExample>::Uuid();
        // If you need to cross a gem boundary, you can use the AssetTypeInfo EBus and EBusFindAssetTypeByName:
        //     EBusFindAssetTypeByName assetType("customassetexample");
        //     AssetTypeInfoBus::BroadcastResult(assetType, &AssetTypeInfo::GetAssetType);
        //     jobProduct.m_productAssetType = assetType.GetAssetType();

        // You should also pick a unique "SubID" for each product.  The final address of an asset (the AssetId) is the combination
        // of the subId you choose here and the source file's UUID, so if it is not unique, errors will be generated since your
        // assets will shadow each other's address, and not be accessible via AssetId.
        // you can pick whatever scheme you want but you should ensure stablility in your choice.
        // For example, do not use random numbers - ideally no matter what happens, each time you run this process, the same
        // subIds are chosen for the same logical asset (even if your builder starts emitting more or different assets out of the same source)
        // You can use AssetBuilderSDK::ConstructSubID(...) helper function if you want to use various bits of the subID for things like LOD level
        // or you can come up with your own scheme to ensure stability, using the 32-bit address space as you see fit.  It only has to be unique
        // and stable within the confines of a single source file, it is not globally unique.
        jobProduct.m_productSubID = 0;
        
        // once you've filled up the details of the product in jobProduct, add it to the result list:
        response.m_outputProducts.push_back(jobProduct);

        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }
}
