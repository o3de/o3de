"""
Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT
"""
def install_user_site():
    import os
    import sys
    import azlmbr.paths
    executableBinFolder = azlmbr.paths.executableFolder

    # the PyAssImp module checks the Windows PATH for the assimp DLL file
    if os.name == "nt":
        os.environ['PATH'] = os.environ['PATH'] + os.pathsep + executableBinFolder

    # PyAssImp module needs to find the shared library for assimp to load; "posix" handles Mac and Linux
    if os.name == "posix":
        if 'LD_LIBRARY_PATH' in os.environ:
            os.environ['LD_LIBRARY_PATH'] = os.environ['LD_LIBRARY_PATH'] + os.pathsep + executableBinFolder
        else:
            os.environ['LD_LIBRARY_PATH'] = executableBinFolder
        
    # add the user site packages folder to find the pyassimp egg link
    import site
    for item in sys.path:        
        if (item.find('site-packages') != -1):
            site.addsitedir(item)

install_user_site()
import pyassimp

import azlmbr.asset
import azlmbr.asset.builder
import azlmbr.asset.entity
import azlmbr.blast
import azlmbr.bus as bus
import azlmbr.editor as editor
import azlmbr.entity
import azlmbr.math
import os
import traceback
import binascii
import sys

# the UUID must be unique amongst all the asset builders in Python or otherwise
# a collision of builders will happen preventing one from running
busIdString = '{CF5C74D1-9ED4-4851-85B1-9B15090DBEC7}'
busId = azlmbr.math.Uuid_CreateString(busIdString, 0)
handler = None
jobKeyName = 'Blast Chunk Assets'
sceneManifestType = azlmbr.math.Uuid_CreateString('{9274AD17-3212-4651-9F3B-7DCCB080E467}', 0)
dccMaterialType = azlmbr.math.Uuid_CreateString('{C88469CF-21E7-41EB-96FD-BF14FBB05EDC}', 0)


def log_exception_traceback():
    exc_type, exc_value, exc_tb = sys.exc_info()
    data = traceback.format_exception(exc_type, exc_value, exc_tb)
    print(str(data))


def get_source_fbx_filename(request):
    fullPath = os.path.join(request.watchFolder, request.sourceFile)
    basePath, filePart = os.path.split(fullPath)
    filename = os.path.splitext(filePart)[0] + '.fbx'
    filename = os.path.join(basePath, filename)
    return filename


def raise_error(message):
    raise RuntimeError(f'[ERROR]: {message}')


def generate_asset_info(chunkNames, request):
    import azlmbr.blast

    # write out an object stream with the extension of .fbx.assetinfo.generated
    basePath, sceneFile = os.path.split(request.sourceFile)
    assetinfoFilename = os.path.splitext(sceneFile)[0] + '.fbx.assetinfo.generated'
    assetinfoFilename = os.path.join(basePath, assetinfoFilename)
    assetinfoFilename = assetinfoFilename.replace('\\', '/').lower()
    outputFilename = os.path.join(request.tempDirPath, assetinfoFilename)

    storage = azlmbr.blast.BlastSliceAssetStorageComponent()
    if (storage.GenerateAssetInfo(chunkNames, request.sourceFile, outputFilename)):
        product = azlmbr.asset.builder.JobProduct(assetinfoFilename, sceneManifestType, 1)
        product.dependenciesHandled = True
        return product
    raise_error('Failed to generate assetinfo.generated')


def export_fbx_manifest(request):
    output = []
    fbxFilename = get_source_fbx_filename(request)
    sceneAsset = pyassimp.load(fbxFilename)
    with sceneAsset as scene:
        rootNode = scene.mRootNode.contents
        for index in range(0, rootNode.mNumChildren):
            child = rootNode.mChildren[index]
            childNode = child.contents
            childNodeName = bytes.decode(childNode.mName.data)
            output.append(str(childNodeName))
    return output


def convert_to_asset_paths(fbxFilename, gameRoot, chunkNameList):
    realtivePath = fbxFilename[len(gameRoot) + 1:]
    realtivePath = os.path.splitext(realtivePath)[0]
    output = []
    for chunk in chunkNameList:
        assetPath = realtivePath + '-' + chunk + '.cgf'
        assetPath = assetPath.replace('\\', '/')
        assetPath = assetPath.lower()
        output.append(assetPath)
    return output


# creates a single job to compile for each platform
def create_jobs(request):
    fbxSidecarFilename = get_source_fbx_filename(request)
    if (os.path.exists(fbxSidecarFilename) is False):
        print('[WARN] Sidecar FBX file {} is missing for blast file {}'.format(fbxSidecarFilename, request.sourceFile))
        return azlmbr.asset.builder.CreateJobsResponse()

    # see if the FBX file already has a .assetinfo source asset, if so then do not create a job
    if (os.path.exists(f'{fbxSidecarFilename}.assetinfo')):
        response = azlmbr.asset.builder.CreateJobsResponse()
        response.result = azlmbr.asset.builder.CreateJobsResponse_ResultSuccess
        return response

    # create job descriptor for each platform
    jobDescriptorList = []
    for platformInfo in request.enabledPlatforms:
        jobDesc = azlmbr.asset.builder.JobDescriptor()
        jobDesc.jobKey = jobKeyName
        jobDesc.priority = 12  # higher than the 'Scene compilation' or 'fbx'
        jobDesc.set_platform_identifier(platformInfo.identifier)
        jobDescriptorList.append(jobDesc)

    response = azlmbr.asset.builder.CreateJobsResponse()
    response.result = azlmbr.asset.builder.CreateJobsResponse_ResultSuccess
    response.createJobOutputs = jobDescriptorList
    return response

# handler to create jobs for a source asset


def on_create_jobs(args):
    try:
        request = args[0]
        return create_jobs(request)
    except:
        log_exception_traceback()
        return azlmbr.asset.builder.CreateJobsResponse()


def generate_blast_slice_asset(chunkNameList, request):
    # get list of relative chunk paths
    fbxFilename = get_source_fbx_filename(request)
    assetPaths = convert_to_asset_paths(fbxFilename, request.watchFolder, chunkNameList)

    outcome = azlmbr.asset.entity.PythonBuilderRequestBus(bus.Broadcast, 'CreateEditorEntity', 'BlastData')
    if (outcome.IsSuccess() is False):
        raise_error('could not create an editor entity')
    blastDataEntityId = outcome.GetValue()

    # create a component for the editor entity
    gameType = azlmbr.entity.EntityType().Game
    blastMeshDataTypeIdList = editor.EditorComponentAPIBus(bus.Broadcast, 'FindComponentTypeIdsByEntityType', ["Blast Slice Storage Component"], gameType)
    componentOutcome = editor.EditorComponentAPIBus(bus.Broadcast, 'AddComponentOfType', blastDataEntityId, blastMeshDataTypeIdList[0])
    if (componentOutcome.IsSuccess() is False):
        raise_error('failed to add component (Blast Slice Storage Component) to the blast_slice')

    # build the blast slice using the chunk asset paths
    blastMeshComponentId = componentOutcome.GetValue()[0]
    outcome = editor.EditorComponentAPIBus(bus.Broadcast, 'BuildComponentPropertyTreeEditor', blastMeshComponentId)
    if(outcome.IsSuccess() is False):
        raise_error(f'failed to create Property Tree Editor for component ({blastMeshComponentId})')
    pte = outcome.GetValue()
    pte.set_visible_enforcement(True)
    pte.set_value('Mesh Paths', assetPaths)

    # write out an object stream with the extension of .blast_slice
    basePath, sceneFile = os.path.split(request.sourceFile)
    blastFilename = os.path.splitext(sceneFile)[0] + '.blast_slice'
    blastFilename = os.path.join(basePath, blastFilename)
    blastFilename = blastFilename.replace('\\', '/').lower()
    tempFilename = os.path.join(request.tempDirPath, blastFilename)
    entityList = [blastDataEntityId]
    makeDynamic = False
    outcome = azlmbr.asset.entity.PythonBuilderRequestBus(bus.Broadcast, 'WriteSliceFile', tempFilename, entityList, makeDynamic)
    if (outcome.IsSuccess() is False):
        raise_error(f'WriteSliceFile failed for blast_slice file ({blastFilename})')

    # return a job product
    blastSliceAsset = azlmbr.blast.BlastSliceAsset()
    subId = binascii.crc32(blastFilename.encode('utf8'))
    product = azlmbr.asset.builder.JobProduct(blastFilename, blastSliceAsset.GetAssetTypeId(), subId)
    product.dependenciesHandled = True
    return product


def read_in_string(data, dataLength):
    stringData = ''
    for idx in range(4, dataLength - 1):
        char = bytes.decode(data[idx])
        if (str.isascii(char)):
            stringData += char
    return stringData


def import_material_info(fbxFilename):
    _, group_name = os.path.split(fbxFilename)
    group_name = os.path.splitext(group_name)[0]
    output = {}
    output['group_name'] = group_name
    output['material_name_list'] = []
    sceneAsset = pyassimp.load(fbxFilename)
    with sceneAsset as scene:
        for materialIndex in range(0, scene.mNumMaterials):
            material = scene.mMaterials[materialIndex].contents
            for materialPropertyIdx in range(0, material.mNumProperties):
                materialProperty = material.mProperties[materialPropertyIdx].contents
                materialPropertyName = bytes.decode(materialProperty.mKey.data)
                if (materialPropertyName.endswith('mat.name') and materialProperty.mType is 3):
                    stringData = read_in_string(materialProperty.mData, materialProperty.mDataLength)
                    output['material_name_list'].append(stringData)
    return output


def write_material_file(sourceFile, destFolder):
    # preserve source MTL files 
    rootPath, materialSourceFile = os.path.split(sourceFile)
    materialSourceFile = os.path.splitext(materialSourceFile)[0] + '.mtl'
    materialSourceFile = os.path.join(rootPath, materialSourceFile)
    if (os.path.exists(materialSourceFile)):
        print(f'{materialSourceFile} source already exists')
        return None

    # auto-generate a DCC material file 
    info = import_material_info(sourceFile)
    materialGroupName = info['group_name']
    materialNames = info['material_name_list']
    materialFilename = materialGroupName + '.dccmtl.generated'
    subId = binascii.crc32(materialFilename.encode('utf8'))
    materialFilename = os.path.join(destFolder, materialFilename)
    storage = azlmbr.blast.BlastSliceAssetStorageComponent()
    storage.WriteMaterialFile(materialGroupName, materialNames, materialFilename)
    product = azlmbr.asset.builder.JobProduct(materialFilename, dccMaterialType, subId)
    product.dependenciesHandled = True
    return product


def process_fbx_file(request):
    # fill out response object
    response = azlmbr.asset.builder.ProcessJobResponse()
    productOutputs = []

    # write out DCCMTL file as a product (if needed)
    materialProduct = write_material_file(get_source_fbx_filename(request), request.tempDirPath)
    if (materialProduct is not None):
        productOutputs.append(materialProduct)

    # prepare output folder
    basePath, _ = os.path.split(request.sourceFile)
    outputPath = os.path.join(request.tempDirPath, basePath)
    os.makedirs(outputPath)

    # parse FBX for chunk names
    chunkNameList = export_fbx_manifest(request)

    # create assetinfo generated (is product)
    productOutputs.append(generate_asset_info(chunkNameList, request))

    # write out the blast_slice object stream
    productOutputs.append(generate_blast_slice_asset(chunkNameList, request))

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
    builderDescriptor.name = "Blast Gem"
    builderDescriptor.patterns = [assetPattern]
    builderDescriptor.busId = busId
    builderDescriptor.version = 5

    outcome = azlmbr.asset.builder.PythonAssetBuilderRequestBus(azlmbr.bus.Broadcast, 'RegisterAssetBuilder', builderDescriptor)
    if outcome.IsSuccess():
        # created the asset builder to hook into the notification bus
        handler = azlmbr.asset.builder.PythonBuilderNotificationBusHandler()
        handler.connect(busId)
        handler.add_callback('OnCreateJobsRequest', on_create_jobs)
        handler.add_callback('OnProcessJobRequest', on_process_job)
        return handler


# create the asset builder handler
try:
    handler = register_asset_builder()
except:
    handler = None
    log_exception_traceback()
