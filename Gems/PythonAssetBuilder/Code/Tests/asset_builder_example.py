"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
#
# Simple example asset builder that processes *.foo files
#
import azlmbr.math
import azlmbr.asset.builder
import os, shutil

# the UUID must be unique amongst all the asset builders in Python or otherwise
busIdString = '{E4DB381B-61A0-4729-ACD9-4C8BDD2D2282}'
busId = azlmbr.math.Uuid_CreateString(busIdString)
assetTypeScript = azlmbr.math.Uuid_CreateString('{82557326-4AE3-416C-95D6-C70635AB7588}')
handler = None
jobKeyPrefix = 'Foo Job Key'
targetAssetFolder = 'foo_scripts'


# creates a single job to compile for a 'pc' platform
def on_create_jobs(args):
    request = args[0]  # azlmbr.asset.builder.CreateJobsRequest
    response = azlmbr.asset.builder.CreateJobsResponse()

    # note: if the asset builder is going to handle more than one file pattern it might need to check out 
    #       the request.sourceFile to figure out what jobs need to be created

    jobDescriptorList = []
    for platformInfo in request.enabledPlatforms:
        # for each enabled platform like 'pc' or 'ios'
        platformId = platformInfo.identifier

        # set up unique job key
        jobKey = '{} {}'.format(jobKeyPrefix, platformId)

        # create job descriptor
        jobDesc = azlmbr.asset.builder.JobDescriptor()
        jobDesc.jobKey = jobKey
        jobDesc.set_platform_identifier(platformId)
        jobDescriptorList.append(jobDesc)
        print ('created a job for {} with key {}'.format(platformId, jobKey))

    response.createJobOutputs = jobDescriptorList
    response.result = azlmbr.asset.builder.CreateJobsResponse_ResultSuccess
    return response


def get_target_name(sourceFullpath):
    lua_file = os.path.basename(sourceFullpath)
    lua_file = os.path.splitext(lua_file)[0]
    lua_file = lua_file + '.lua'
    return lua_file


def copy_foo_file(srcFile, dstFile):
    try:
        dir_name = os.path.dirname(dstFile)
        if (os.path.exists(dir_name) is False):
            os.makedirs(dir_name)
        shutil.copyfile(srcFile, dstFile)
        return True
    except:
        return False


# using the incoming 'request' find the type of job via 'jobKey' to determine what to do
def on_process_job(args):
    request = args[0]  # azlmbr.asset.builder.ProcessJobRequest
    response = azlmbr.asset.builder.ProcessJobResponse()

    # note: if possible to loop through incoming data a 'yeild' can be used to cooperatively
    #       thread the processing of the assets so that shutdown and cancel can be handled

    if (request.jobDescription.jobKey.startswith(jobKeyPrefix)):
        targetFile = os.path.join(targetAssetFolder, get_target_name(request.fullPath))
        dstFile = os.path.join(request.tempDirPath, targetFile)
        if (copy_foo_file(request.fullPath, dstFile)):
            response.outputProducts = [azlmbr.asset.builder.JobProduct(dstFile, assetTypeScript, 0)]
            response.resultCode = azlmbr.asset.builder.ProcessJobResponse_Success
            response.dependenciesHandled = True

    return response


def on_shutdown(args):
    # note: user should attempt to close down any processing job if any running
    global handler
    if (handler is not None):
        handler.disconnect()
    handler = None


def on_cancel_job(args):
    # note: user should attempt to close down any processing job if any running
    print('>>> FOO asset builder - on_cancel_job <<<')


# register asset builder for source assets
def register_asset_builder():
    assetPattern = azlmbr.asset.builder.AssetBuilderPattern()
    assetPattern.pattern = '*.foo'
    assetPattern.type = azlmbr.asset.builder.AssetBuilderPattern_Wildcard

    builderDescriptor = azlmbr.asset.builder.AssetBuilderDesc()
    builderDescriptor.name = "Foo Asset Builder"
    builderDescriptor.patterns = [assetPattern]
    builderDescriptor.busId = busId
    builderDescriptor.version = 0

    outcome = azlmbr.asset.builder.PythonAssetBuilderRequestBus(azlmbr.bus.Broadcast, 'RegisterAssetBuilder', builderDescriptor)
    if outcome.IsSuccess():
        # created the asset builder handler to hook into the notification bus
        jobHandler = azlmbr.asset.builder.PythonBuilderNotificationBusHandler()
        jobHandler.connect(busId)
        jobHandler.add_callback('OnCreateJobsRequest', on_create_jobs)
        jobHandler.add_callback('OnProcessJobRequest', on_process_job)
        jobHandler.add_callback('OnShutdown', on_shutdown)
        jobHandler.add_callback('OnCancel', on_cancel_job)
        return jobHandler


# note: the handler has to be retained since Python retains the object ref count
#       on_shutdown will clear the 'handler' to disconnect from the notification bus
handler = register_asset_builder()
