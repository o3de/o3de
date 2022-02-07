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
