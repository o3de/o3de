#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#
import traceback, sys, uuid, os, json

#
# Example for exporting ActorGroup scene rules
#

def log_exception_traceback():
    exc_type, exc_value, exc_tb = sys.exc_info()
    data = traceback.format_exception(exc_type, exc_value, exc_tb)
    print(str(data))

def get_node_names(sceneGraph, nodeTypeName, testEndPoint = False, validList = None):
    import azlmbr.scene.graph
    import scene_api.scene_data

    node = sceneGraph.get_root()
    nodeList = []
    children = []
    paths = []

    while node.IsValid():
        # store children to process after siblings
        if sceneGraph.has_node_child(node):
            children.append(sceneGraph.get_node_child(node))

        nodeName = scene_api.scene_data.SceneGraphName(sceneGraph.get_node_name(node))
        paths.append(nodeName.get_path())
        
        include = True 
        
        if (validList is not None):
            include = False # if a valid list filter provided, assume to not include node name
            name_parts = nodeName.get_path().split('.')
            for valid in validList:
                if (valid in name_parts[-1]):
                    include = True
                    break

        # store any node that has provides specifc data content
        nodeContent = sceneGraph.get_node_content(node)
        if include and nodeContent.CastWithTypeName(nodeTypeName):
            if testEndPoint is not None:
                include = sceneGraph.is_node_end_point(node) is testEndPoint
            if include:
                if (len(nodeName.get_path())):
                    nodeList.append(scene_api.scene_data.SceneGraphName(sceneGraph.get_node_name(node)))

        # advance to next node
        if sceneGraph.has_node_sibling(node):
            node = sceneGraph.get_node_sibling(node)
        elif children:
            node = children.pop()
        else:
            node = azlmbr.scene.graph.NodeIndex()

    return nodeList, paths

def generate_mesh_group(scene, sceneManifest, meshDataList, paths):
    # Compute the name of the scene file
    clean_filename = scene.sourceFilename.replace('.', '_')
    mesh_group_name = os.path.basename(clean_filename)

    # make the mesh group
    mesh_group = sceneManifest.add_mesh_group(mesh_group_name)
    mesh_group['id'] = '{' + str(uuid.uuid5(uuid.NAMESPACE_DNS, clean_filename)) + '}'

    # add all nodes to this mesh group
    for activeMeshIndex in range(len(meshDataList)):
        mesh_name = meshDataList[activeMeshIndex]
        mesh_path = mesh_name.get_path()
        sceneManifest.mesh_group_select_node(mesh_group, mesh_path)

def create_shape_configuration(nodeName):
    import scene_api.physics_data
    
    if(nodeName in ['_foot_','_wrist_']):
        shapeConfiguration = scene_api.physics_data.BoxShapeConfiguration()
        shapeConfiguration.scale = [1.1, 1.1, 1.1]
        shapeConfiguration.dimensions = [2.1, 3.1, 4.1]
        return shapeConfiguration
    else:
        shapeConfiguration = scene_api.physics_data.CapsuleShapeConfiguration()
        shapeConfiguration.scale = [1.0, 1.0, 1.0]
        shapeConfiguration.height = 1.0
        shapeConfiguration.radius = 1.0
        return shapeConfiguration
    
def create_collider_configuration(nodeName):
    import scene_api.physics_data
    
    colliderConfiguration = scene_api.physics_data.ColliderConfiguration()
    colliderConfiguration.Position = [0.1, 0.1, 0.2]
    colliderConfiguration.Rotation = [45.0, 35.0, 25.0]    
    return colliderConfiguration

def generate_physics_nodes(actorPhysicsSetupRule, nodeNameList):
    import scene_api.physics_data
    
    hitDetectionConfig = scene_api.physics_data.CharacterColliderConfiguration()    
    simulatedObjectColliderConfig = scene_api.physics_data.CharacterColliderConfiguration()
    clothConfig = scene_api.physics_data.CharacterColliderConfiguration()
    ragdollConfig = scene_api.physics_data.RagdollConfiguration()    
    
    for nodeName in nodeNameList:
        shapeConfiguration = create_shape_configuration(nodeName)
        colliderConfiguration = create_collider_configuration(nodeName)
        hitDetectionConfig.add_character_collider_node_configuration_node(nodeName, colliderConfiguration, shapeConfiguration)
        simulatedObjectColliderConfig.add_character_collider_node_configuration_node(nodeName, colliderConfiguration, shapeConfiguration)
        clothConfig.add_character_collider_node_configuration_node(nodeName, colliderConfiguration, shapeConfiguration)
        #
        ragdollNode = scene_api.physics_data.RagdollNodeConfiguration()
        ragdollNode.JointConfig.Name = nodeName
        ragdollConfig.add_ragdoll_node_configuration(ragdollNode)
        ragdollConfig.colliders.add_character_collider_node_configuration_node(nodeName, colliderConfiguration, shapeConfiguration)
    
    actorPhysicsSetupRule.set_simulated_object_collider_config(simulatedObjectColliderConfig)
    actorPhysicsSetupRule.set_hit_detection_config(hitDetectionConfig)
    actorPhysicsSetupRule.set_cloth_config(clothConfig)
    actorPhysicsSetupRule.set_ragdoll_config(ragdollConfig)

def generate_actor_group(scene, sceneManifest, meshDataList, paths):
    import scene_api.scene_data
    import scene_api.physics_data
    import scene_api.actor_group
    
    # fetch bone data
    validNames = ['_neck_','_pelvis_','_leg_','_knee_','_spine_','_arm_','_clavicle_','_head_','_elbow_','_wrist_']
    graph = scene_api.scene_data.SceneGraph(scene.graph)
    nodeList, allNodePaths = get_node_names(graph, 'BoneData', validList = validNames)

    nodeNameList = []
    for activeMeshIndex, nodeName in enumerate(nodeList):
        nodeNameList.append(nodeName.get_name())

    # add comment    
    commentRule = scene_api.actor_group.CommentRule()
    commentRule.text = str(nodeNameList)
    
    # ActorPhysicsSetupRule
    actorPhysicsSetupRule = scene_api.actor_group.ActorPhysicsSetupRule()
    generate_physics_nodes(actorPhysicsSetupRule, nodeNameList)
    
    # add scale of the Actor rule
    actorScaleRule = scene_api.actor_group.ActorScaleRule()
    actorScaleRule.scaleFactor = 2.0
    
    # add coordinate system rule
    coordinateSystemRule = scene_api.actor_group.CoordinateSystemRule()
    coordinateSystemRule.useAdvancedData = False
    
    # add morph target rule
    morphTargetRule = scene_api.actor_group.MorphTargetRule()
    morphTargetRule.targets.select_targets([nodeNameList[0]], nodeNameList)
    
    # add skeleton optimization rule
    skeletonOptimizationRule = scene_api.actor_group.SkeletonOptimizationRule()
    skeletonOptimizationRule.autoSkeletonLOD = True
    skeletonOptimizationRule.criticalBonesList.select_targets([nodeNameList[0:2]], nodeNameList)
    
    # add LOD rule
    lodRule = scene_api.actor_group.LodRule()
    lodRule0 = lodRule.add_lod_level(0)
    lodRule0.select_targets([nodeNameList[1:4]], nodeNameList)

    actorGroup = scene_api.actor_group.ActorGroup()
    actorGroup.name = os.path.basename(scene.sourceFilename)
    actorGroup.add_rule(actorScaleRule)
    actorGroup.add_rule(coordinateSystemRule)
    actorGroup.add_rule(skeletonOptimizationRule)
    actorGroup.add_rule(morphTargetRule)
    actorGroup.add_rule(lodRule)
    actorGroup.add_rule(actorPhysicsSetupRule)
    actorGroup.add_rule(commentRule)
    sceneManifest.manifest['values'].append(actorGroup.to_dict())
    
def update_manifest(scene):
    import json, uuid, os
    import azlmbr.scene.graph
    import scene_api.scene_data

    graph = scene_api.scene_data.SceneGraph(scene.graph)
    mesh_name_list, all_node_paths = get_node_names(graph, 'MeshData')
    scene_manifest = scene_api.scene_data.SceneManifest()   
    generate_actor_group(scene, scene_manifest, mesh_name_list, all_node_paths)
    generate_mesh_group(scene, scene_manifest, mesh_name_list, all_node_paths)

    # Convert the manifest to a JSON string and return it
    return scene_manifest.export()

sceneJobHandler = None

def on_update_manifest(args):
    try:
        scene = args[0]
        return update_manifest(scene)
    except RuntimeError as err:
        print (f'ERROR - {err}')
        log_exception_traceback()
    except:
        log_exception_traceback()

    global sceneJobHandler
    sceneJobHandler.disconnect()
    sceneJobHandler = None

# try to create SceneAPI handler for processing
try:
    import azlmbr.scene

    sceneJobHandler = azlmbr.scene.ScriptBuildingNotificationBusHandler()
    sceneJobHandler.connect()
    sceneJobHandler.add_callback('OnUpdateManifest', on_update_manifest)
except:
    sceneJobHandler = None
