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
    currentScript = pathlib.Path(__file__).resolve()
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

#
# SceneAPI Processor
#
blastChunksAssetType = azlmbr.math.Uuid_CreateString('{993F0B0F-37D9-48C6-9CC2-E27D3F3E343E}', 0)

def export_chunk_asset(scene, outputDirectory, platformIdentifier, productList):
    import azlmbr.scene
    import azlmbr.object
    import azlmbr.paths
    import json, os

    jsonFilename = os.path.basename(scene.sourceFilename)
    jsonFilename = os.path.join(outputDirectory, jsonFilename + '.blast_chunks')

    # prepare output folder
    basePath, _ = os.path.split(jsonFilename)
    outputPath = os.path.join(outputDirectory, basePath)
    if not os.path.exists(outputPath):
        os.makedirs(outputPath, False)

    # write out a JSON file with the chunk file info
    with open(jsonFilename, "w") as jsonFile:
        jsonFile.write(scene.manifest.ExportToJson())

    exportProduct = azlmbr.scene.ExportProduct()
    exportProduct.filename = jsonFilename
    exportProduct.sourceId = scene.sourceGuid
    exportProduct.assetType = blastChunksAssetType
    exportProduct.subId = 101

    exportProductList = azlmbr.scene.ExportProductList()
    exportProductList.AddProduct(exportProduct)
    return exportProductList

def on_prepare_for_export(args):
    try:
        scene = args[0] # azlmbr.scene.Scene
        outputDirectory = args[1] # string
        platformIdentifier = args[2] # string
        productList = args[3] # azlmbr.scene.ExportProductList
        return export_chunk_asset(scene, outputDirectory, platformIdentifier, productList)
    except:
        log_exception_traceback()

def get_mesh_node_names(sceneGraph):
    import azlmbr.scene as sceneApi
    import azlmbr.scene.graph
    from scene_api import scene_data as sceneData

    meshDataList = []
    node = sceneGraph.get_root()
    children = []

    while node.IsValid():
        # store children to process after siblings
        if sceneGraph.has_node_child(node):
            children.append(sceneGraph.get_node_child(node))

        # store any node that has mesh data content
        nodeContent = sceneGraph.get_node_content(node)
        if nodeContent is not None and nodeContent.CastWithTypeName('MeshData'):
            if sceneGraph.is_node_end_point(node) is False:
                nodeName = sceneData.SceneGraphName(sceneGraph.get_node_name(node))
                nodePath = nodeName.get_path()
                if (len(nodeName.get_path())):
                    meshDataList.append(sceneData.SceneGraphName(sceneGraph.get_node_name(node)))

        # advance to next node
        if sceneGraph.has_node_sibling(node):
            node = sceneGraph.get_node_sibling(node)
        elif children:
            node = children.pop()
        else:
            node = azlmbr.scene.graph.NodeIndex()

    return meshDataList

def update_manifest(scene):
    import uuid, os
    import azlmbr.scene as sceneApi
    import azlmbr.scene.graph
    from scene_api import scene_data as sceneData

    graph = sceneData.SceneGraph(scene.graph)
    meshNameList = get_mesh_node_names(graph)
    sceneManifest = sceneData.SceneManifest()
    sourceFilenameOnly = os.path.basename(scene.sourceFilename)
    sourceFilenameOnly = sourceFilenameOnly.replace('.','_')

    for activeMeshIndex in range(len(meshNameList)):
        chunkName = meshNameList[activeMeshIndex]
        chunkPath = chunkName.get_path()
        meshGroupName = '{}_{}'.format(sourceFilenameOnly, chunkName.get_name())
        meshGroup = sceneManifest.add_mesh_group(meshGroupName)
        meshGroup['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, sourceFilenameOnly + chunkPath)) + '}'
        sceneManifest.mesh_group_select_node(meshGroup, chunkPath)

    return sceneManifest.export()

sceneJobHandler = None

def on_update_manifest(args):
    try:
        scene = args[0]
        return update_manifest(scene)
    except:
        global sceneJobHandler
        sceneJobHandler = None
        log_exception_traceback()

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene as sceneApi
    if (sceneJobHandler == None):
        sceneJobHandler = sceneApi.ScriptBuildingNotificationBusHandler()
        sceneJobHandler.connect()
        sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
        sceneJobHandler.add_callback('OnPrepareForExport', on_prepare_for_export)
except:
    sceneJobHandler = None
