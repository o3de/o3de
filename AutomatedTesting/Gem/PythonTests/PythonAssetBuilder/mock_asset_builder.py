"""
All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
its licensors.

For complete copyright and license terms please see the LICENSE at the root of this
distribution (the "License"). All use of this software is governed by the License,
or, if provided, by the license below or the license accompanying this file. Do not
remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
"""
assetBuilderReady = True
try:
    import azlmbr.asset
    import azlmbr.asset.builder
    from azlmbr.asset.builder import AssetBuilderPattern
    import azlmbr.bus
    import azlmbr.math
    import os, traceback, binascii, sys
except:
    # skip if the azlmbr.asset.builder is not enabled
    assetBuilderReady = False

jobKeyName = 'Mock Asset'

def log_exception_traceback():
    exc_type, exc_value, exc_tb = sys.exc_info()
    data = traceback.format_exception(exc_type, exc_value, exc_tb)
    print(str(data))

# creates a single job to compile for each platform
def create_jobs(request):
    # create job descriptor for each platform
    jobDescriptorList = []
    for platformInfo in request.enabledPlatforms:
        jobDesc = azlmbr.asset.builder.JobDescriptor()
        jobDesc.jobKey = jobKeyName
        jobDesc.set_platform_identifier(platformInfo.identifier)
        jobDescriptorList.append(jobDesc)

    response = azlmbr.asset.builder.CreateJobsResponse()
    response.result = azlmbr.asset.builder.CreateJobsResponse_ResultSuccess
    response.createJobOutputs = jobDescriptorList
    return response

def on_create_jobs(args):
    try:
        request = args[0]
        return create_jobs(request)
    except:
        log_exception_traceback()
    # returing back a default CreateJobsResponse() records an asset error
    return azlmbr.asset.builder.CreateJobsResponse()

def process_file(request):
    # prepare output folder
    basePath, _ = os.path.split(request.sourceFile)
    outputPath = os.path.join(request.tempDirPath, basePath)
    os.makedirs(outputPath, exist_ok=True)

    # write out a mock file
    basePath, sourceFile = os.path.split(request.sourceFile)
    mockFilename = os.path.splitext(sourceFile)[0] + '.mock_asset'
    mockFilename = os.path.join(basePath, mockFilename)
    mockFilename = mockFilename.replace('\\', '/')
    tempFilename = os.path.join(request.tempDirPath, mockFilename)

    # write out a tempFilename like a JSON
    fileOutput = open(tempFilename, "w")
    fileOutput.write('{}')
    fileOutput.close()

    # generate a product asset file entry
    subId = binascii.crc32(mockFilename.encode())
    mockAssetType = azlmbr.math.Uuid_CreateString('{9274AD17-3212-4651-9F3B-7DCCB080E467}', 0)
    product = azlmbr.asset.builder.JobProduct(mockFilename, mockAssetType, subId)
    product.dependenciesHandled = True
    productOutputs = []
    productOutputs.append(product)

    # fill out response object
    response = azlmbr.asset.builder.ProcessJobResponse()
    response.outputProducts = productOutputs
    response.resultCode = azlmbr.asset.builder.ProcessJobResponse_Success
    response.dependenciesHandled = True
    return response

# using the incoming 'request' find the type of job via 'jobKey' to determine what to do
def on_process_job(args):
    try:
        request = args[0]
        if (request.jobDescription.jobKey.startswith(jobKeyName)):
            return process_file(request)
    except:
        log_exception_traceback()
    # returning back an empty ProcessJobResponse() will record an error
    return azlmbr.asset.builder.ProcessJobResponse()

# register asset builder
def register_asset_builder(busId):
    assetPattern = azlmbr.asset.builder.AssetBuilderPattern()
    assetPattern.pattern = '*.mock'
    assetPattern.type = azlmbr.asset.builder.AssetBuilderPattern_Wildcard

    builderDescriptor = azlmbr.asset.builder.AssetBuilderDesc()
    builderDescriptor.name = "Mock Builder"
    builderDescriptor.patterns = [assetPattern]
    builderDescriptor.busId = busId
    builderDescriptor.version = 1

    outcome = azlmbr.asset.builder.PythonAssetBuilderRequestBus(azlmbr.bus.Broadcast, 'RegisterAssetBuilder', builderDescriptor)
    if outcome.IsSuccess():
        # created the asset builder to hook into the notification bus
        handler = azlmbr.asset.builder.PythonBuilderNotificationBusHandler()
        handler.connect(busId)
        handler.add_callback('OnCreateJobsRequest', on_create_jobs)
        handler.add_callback('OnProcessJobRequest', on_process_job)
        return handler

# create the asset builder handler
handler = None
try:
    if (assetBuilderReady):
        busIdString = '{CF5C74C1-9ED4-5851-95B1-0B15090DBEC7}'
        busId = azlmbr.math.Uuid_CreateString(busIdString, 0)
        handler = register_asset_builder(busId)
except:
    handler = None
    log_exception_traceback()
