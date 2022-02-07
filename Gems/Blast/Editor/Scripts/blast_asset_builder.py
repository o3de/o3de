"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""

"""
This a Python Asset Builder script examines each .blast file to see if an
associated .fbx file needs to be processed by exporting all of its chunks
into a scene manifest

This is also a SceneAPI script that executes from a foo.fbx.assetinfo scene
manifest that writes out asset chunk data for .blast files
"""
import os, traceback, binascii, sys, json, pathlib
import azlmbr.math
import azlmbr.asset
import azlmbr.asset.entity
import azlmbr.asset.builder
import azlmbr.bus

#
# Python Asset Builder
#
busId = azlmbr.math.Uuid_CreateString('{D4FA20E3-8EF4-44A3-A045-AAE6C1CCAAAB}', 0)
jobKeyName = 'Blast Chunk Assets'

def log_exception_traceback():
    exc_type, exc_value, exc_tb = sys.exc_info()
    data = traceback.format_exception(exc_type, exc_value, exc_tb)
    print(str(data))

def raise_error(message):
    print (f'ERROR - {message}');
    raise RuntimeError(f'[ERROR]: {message}');

# creates a single job to compile for each platform
def get_source_fbx_filename(request):
    fullPath = os.path.join(request.watchFolder, request.sourceFile)
    basePath, filePart = os.path.split(fullPath)
    filename = os.path.splitext(filePart)[0] + '.fbx'
    filename = os.path.join(basePath, filename)
    return filename

def create_jobs(request):
    fbxSidecarFilename = get_source_fbx_filename(request)
    if (os.path.exists(fbxSidecarFilename) is False):
        print('[WARN] Sidecar FBX file {} is missing for blast file {}'.format(fbxSidecarFilename, request.sourceFile))
        return azlmbr.asset.builder.CreateJobsResponse()

    # see if the FBX file already has a .assetinfo source asset, if so then do not create a job
    establishedAssetInfo = f'{fbxSidecarFilename}.assetinfo';
    if (os.path.exists(establishedAssetInfo)):
        response = azlmbr.asset.builder.CreateJobsResponse()
        response.result = azlmbr.asset.builder.CreateJobsResponse_ResultSuccess
        return response

    # create job descriptor for each platform
    jobDescriptorList = []
    for platformInfo in request.enabledPlatforms:
        sourceFileDependency = azlmbr.asset.builder.SourceFileDependency()
        sourceFileDependency.sourceFileDependencyPath = fbxSidecarFilename

        jobDependency = azlmbr.asset.builder.JobDependency()
        jobDependency.sourceFile = sourceFileDependency
        jobDependency.jobKey = jobKeyName
        jobDependency.platformIdentifier = platformInfo.identifier

        jobDesc = azlmbr.asset.builder.JobDescriptor()
        jobDesc.jobKey = jobKeyName
        jobDesc.set_platform_identifier(platformInfo.identifier)
        jobDesc.jobDependencyList = [jobDependency]
        jobDescriptorList.append(jobDesc)

    response = azlmbr.asset.builder.CreateJobsResponse()
    response.result = azlmbr.asset.builder.CreateJobsResponse_ResultSuccess
    response.createJobOutputs = jobDescriptorList
    return response

# to create jobs for a source asset
def on_create_jobs(args):
    try:
        request = args[0]
        return create_jobs(request)
    except:
        log_exception_traceback()
        return azlmbr.asset.builder.CreateJobsResponse()

def generate_assetinfo_product(request):
    # write out a product asset file with the extension of .fbx.assetinfo.generated
    basePath, sceneFile = os.path.split(request.sourceFile)
    assetinfoFilename = os.path.splitext(sceneFile)[0] + '.fbx.assetinfo.generated'
    assetinfoFilename = os.path.join(basePath, assetinfoFilename)
    assetinfoFilename = assetinfoFilename.replace('\\', '/').lower()
    outputFilename = os.path.join(request.tempDirPath, assetinfoFilename)

    # the only rule in it is to run this file again as a scene processor
    currentScript = str(pathlib.Path(__file__).resolve())
    currentScript = currentScript.replace('\\', '/').lower()
    currentScript = currentScript.replace('blast_asset_builder.py', 'blast_chunk_processor.py')
    aDict = {"values": [{"$type": "ScriptProcessorRule", "scriptFilename": f"{currentScript}"}]}
    jsonString = json.dumps(aDict)
    jsonFile = open(outputFilename, "w")
    jsonFile.write(jsonString)
    jsonFile.close()

    # return a job product for the generated assetinfo file
    sceneManifestType = azlmbr.math.Uuid_CreateString('{9274AD17-3212-4651-9F3B-7DCCB080E467}', 0)
    subId = 1
    product = azlmbr.asset.builder.JobProduct(outputFilename, sceneManifestType, subId)
    product.dependenciesHandled = True
    return product

def process_fbx_file(request):
    # fill out response object
    response = azlmbr.asset.builder.ProcessJobResponse()
    productOutputs = []

    # prepare output folder
    basePath, _ = os.path.split(request.sourceFile)
    outputPath = os.path.join(request.tempDirPath, basePath)
    os.makedirs(outputPath)

    # create assetinfo generated file
    productOutputs.append(generate_assetinfo_product(request))

    response.outputProducts = productOutputs
    response.resultCode = azlmbr.asset.builder.ProcessJobResponse_Success
    response.dependenciesHandled = True
    return response

# using the incoming 'request' find the type of job via 'jobKey' to determine what to do
def on_process_job(args):
    try:
        request = args[0]
        if (request.jobDescription.jobKey.startswith(jobKeyName)):
            return process_fbx_file(request)

        return azlmbr.asset.builder.ProcessJobResponse()
    except:
        log_exception_traceback()
        return azlmbr.asset.builder.ProcessJobResponse()

# register asset builder
def register_asset_builder():
    assetPattern = azlmbr.asset.builder.AssetBuilderPattern()
    assetPattern.pattern = '*.blast'
    assetPattern.type = azlmbr.asset.builder.AssetBuilderPattern_Wildcard

    builderDescriptor = azlmbr.asset.builder.AssetBuilderDesc()
    builderDescriptor.name = "Blast Scene Builder"
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
pythonAssetBuilderHandler = None
try:
    if (pythonAssetBuilderHandler == None):
        pythonAssetBuilderHandler = register_asset_builder()
except:
    pythonAssetBuilderHandler = None
