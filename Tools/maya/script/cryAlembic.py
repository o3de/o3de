#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#
from maya import cmds
from time import time
import cryDecorators
import sys

import logging
logging.basicConfig()
log = logging.getLogger(__name__)
#log.setLevel(logging.DEBUG)

standardSGs = set(['initialParticleSE', 'initialShadingGroup'])


def renameShadingEngines(lstShadingEngines=None):
    '''
    Renames ShadingEngines, to match their material's name.
    This is required, because maya exports FaceSets from ShadingEngines
    and GeomCaches derives material IDs from FaceSets.
    '''
    
    if not lstShadingEngines:
        lstShadingEngines = cmds.ls(type="shadingEngine")
    elif isinstance(lstShadingEngines, basestring):
        lstShadingEngines = [lstShadingEngines]
    
    # can't rename standard shading engines
    lstShadingEngines = list(set(lstShadingEngines) - standardSGs)
    
    if not lstShadingEngines:
        return []

    log.info("Renaming ShadingEngines...")
    log.debug("  %s" % ', '.join(lstShadingEngines))
    
    for i in range(len(lstShadingEngines)):
        lstMaterials = cmds.listConnections("%s.surfaceShader" % lstShadingEngines[i], d=False, s=True, p=False)
        if lstMaterials:
            log.debug("  Materials: %s" % ', '.join(lstMaterials))
            dgMaterial = lstMaterials[0]
            destName = "%sSG" % dgMaterial
            if lstShadingEngines[i] == destName:
                continue
            
            log.debug("Renaming " + lstShadingEngines[i] + " to " + destName + " ...")
            try:
                lstShadingEngines[i] = cmds.rename(lstShadingEngines[i], destName)
                print('lstShadingEngines[i]: %s' % lstShadingEngines[i])
            except:
                log.debug("  !!! FAILED TO RENAME !!!")
        else:
            log.debug("No material found for shadingEngine: " + lstShadingEngines[i])
    return lstShadingEngines


def enforcePerFaceAssignment(lstShadingEngines=None):
    '''
    Workaround for Maya 2014/2015 bug.
    Ensures that ShadingEngines contain only faces, rather than full shapes.
    Alembic 1.1.5 contains a bug, so that it doesn't export FaceSets
    for objects where the entire shape is in the ShadingEngine.
    '''
    if lstShadingEngines is None:
        lstShadingEngines = cmds.ls(type="shadingEngine")
    else:
        if isinstance(lstShadingEngines, basestring):
            lstShadingEngines = [lstShadingEngines]
    
    # cant apply per face assignment on standardSGs 
    lstShadingEngines = set(lstShadingEngines) - standardSGs
    
    if not lstShadingEngines:
        return False
    
    log.info("Enforcing per-face material assignment ...")
    
    for dgShadingEngine in lstShadingEngines:
        log.debug("  shadingEngine: " + dgShadingEngine)
        
        lstSetMembers = cmds.sets(dgShadingEngine, q=True)
        if not lstSetMembers:
            continue
        
        for sSetMember in lstSetMembers:
            if ".f[" not in sSetMember:
                log.debug("  Converting shape-assignment to face-assignment: " + sSetMember)
                # make sure sSetMember is a mesh!
                meshMember = cmds.ls(cmds.listHistory(sSetMember), type='mesh')
                if not meshMember:
                    log.debug('SetMember "%s" (from %s) did not yield a mesh!' % sSetMember, dgShadingEngine)
                    continue
                meshMember = meshMember[0]
                try:
                    #iFaceCount = cmds.polyEvaluate(meshMember, f=True)
                    #sFaces = meshMember + ".f[0:" + str(iFaceCount - 1) + "]"
                    sFaces = "%s.f[*]" % meshMember
                    # nasty hack: temporarily apply default material
                    # to generate objectGroup datastructures
                    if dgShadingEngine not in standardSGs:
                        cmds.sets("%s.f[0]" % meshMember, e=True, fe="initialShadingGroup")
                    cmds.sets(sFaces, e=1, fe=dgShadingEngine)
                    # remove original member from that we added all faces if still in
                    if cmds.sets(sSetMember, isMember=dgShadingEngine):
                        cmds.sets(sSetMember, rm=dgShadingEngine)
                except StandardError, error:
                    log.error(error)
                    log.debug("Failed to all faces to material for object: " + sSetMember + "!")
                    log.debug(sys.exc_info())

        # fix for abc import which throws per face warnings if you have 
        # get connected visible shapes
        connMesh = set(cmds.ls(cmds.listConnections('%s.dagSetMembers' % dgShadingEngine, shapes=True), type='mesh'))
        connMesh = set([m for m in connMesh if not cmds.getAttr('%s.intermediateObject' % m)])
        shapeMembers = set(cmds.ls(lstSetMembers, s=True, dag=True, o=True))
        # find nodes connected to dagSetMembers but are not members
        nonMemberShapes = list(connMesh - shapeMembers)
        if nonMemberShapes:
            log.debug('ShadingEngine "%s" has false member connection to mesh shape: %s'
                      % (dgShadingEngine, ', '.join(nonMemberShapes)))
            # find out exact connection and disconnect
            for shape in nonMemberShapes:
                conns = cmds.listConnections(shape, type='shadingEngine', c=True, p=True)
                for i in range(1, len(conns), 2):
                    if conns[i].startswith('%s.dagSetMembers' % dgShadingEngine):
                        log.debug('unplugging "%s" from "%s"' % (conns[i - 1], conns[i]))
                        cmds.disconnectAttr(conns[i - 1], conns[i])

    return True

@cryDecorators.pluginDependency("AbcExport.mll")
def exportAlembicForGeomCache(nodes=[]):
    '''
    Exports objects to Alembic with settings required for GeomCache pipeline.
    '''
    if not nodes:
        nodes = cmds.ls(sl=True, tr=True)
    if not nodes:
        log.error('Select some transforms to export!')
        return
    
    selMesh, selSGs = getMeshAndSGs(nodes)
    if not selMesh:
        log.error('No meshes found to export!')

    if selSGs.intersection(standardSGs):
        log.warning('Selected objects have default materials applied! Please replace!')
    
    #lstFilePath = cmds.fileDialog2(cap="Export Alembic for GeomCache", ff="*.abc", ds=1, fm=0, dir=(cmds.workspace(q=1, rd=1)))
    lstFilePath = cmds.fileDialog2(cap="Export Alembic for GeomCache", fileFilter="Alembic .abc (*.abc)", fileMode=0)
    if not lstFilePath:
        return
   
    t0 = time()
    selSGs = renameShadingEngines(selSGs)
    enforcePerFaceAssignment(selSGs)

    sOptions = "-frameRange %s %s" % (cmds.playbackOptions(q=1, min=1), cmds.playbackOptions(q=1, max=1))
    sOptions += " -uvWrite -writeColorSets -writeFaceSets -writeVisibility "
    sOptions += ' '.join(['-root %s' % n for n in nodes])
    sOptions += ' -file "%s"' % lstFilePath[0]
    log.debug("AbcExport params:\n\t%s" % sOptions)
    
    log.info('Exporting file: "%s" ...' % lstFilePath[0])
    try:
        cmds.AbcExport(jobArg=sOptions)
    except StandardError, error:
        log.error(error)
        log.debug(sys.exc_info())
    
    log.info('Export finished! %ssec' % round(time() - t0, 1))


def getMeshAndSGs(nodes=[]):
    """
    filters visible mesh shapes from given or selected nodes and returns
    them with thier shading groups
    """
    if not nodes:
        nodes = cmds.ls(sl=True)
    if not nodes:
        return None, None
    selMesh = set(cmds.ls(cmds.listRelatives(nodes, allDescendents=True), type='mesh'))
    selMesh = [m for m in selMesh if not cmds.getAttr('%s.intermediateObject' % m)]
    if not selMesh:
        return None, None
    selSGs = set(cmds.ls(cmds.listConnections(selMesh), type='shadingEngine'))
    return selMesh, selSGs


def prepareMaterials(*args):
    """inline call for Ui"""
    mesh, sgs = getMeshAndSGs()
    sgs= renameShadingEngines(sgs)
    enforcePerFaceAssignment(sgs)
