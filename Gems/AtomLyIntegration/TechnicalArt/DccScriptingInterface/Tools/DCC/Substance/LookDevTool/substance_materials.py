from pysbs import context
from pysbs import sbsenum
from pysbs import substance
from pysbs import sbsgenerator
from pysbs import batchtools

# from pysbs.batchtools import thumbnail

from pysbs import mdl
from pysbs import sbsbakers
from pysbs import sbsexporter
from pysbs import sbslibrary
from pysbs.api_decorators import doc_source_code
from pysbs import python_helpers
from pysbs import base
import subprocess
import logging
import constants
import json
import os

_LOGGER = logging.getLogger('LABC.substance_materials')

# Create SAT Context >>>>>>>>>>>>
aContext = context.Context()
# Declaration of alias 'myAlias'
aContext.getUrlAliasMgr().setAliasAbsPath(aAliasName='myAlias', aAbsPath='<myAliasAbsolutePath>')


def create_designer_files(output_setting, material_info):
    _LOGGER.info('\n\nCreate Designer Files::::::::::::::')
    for dcc_file, object_data in material_info.items():
        _LOGGER.info('Maya File: {}'.format(dcc_file))
        for object_name, texture_values in object_data.items():
            target_directory = texture_values['location']
            sbs_path = get_clean_path(os.path.join(target_directory, '{}.sbs'.format(object_name)))
            build_sbs_file(sbs_path, texture_values, output_setting)


def cook_sbsar(sd_resources_path, sbs_file_path, sbsar_output_path):
    cookAndRender(aContext, sbs_file_path)
    # try:
    #     pysbs_batch.sbscooker(
    #         quiet=True,
    #         inputs=sbs_file_path,
    #         includes=aContext.getDefaultPackagePath(),
    #         output_path=sbsar_output_path,
    #         compression_mode=2
    #     ).wait()
    # except Exception as e:
    #     _LOGGER.info('Fail... Exception: {}'.format(e))
    #     return False
    # return True


def cookAndRender(_context, _inputSbs, _inputGraphPath, _outputCookPath, _outputRenderPath, _outputSize, _udim):
    """
    Call sbscooker with the provided udim, and then sbsrender on the resulting .sbsar

    :param _context: API execution context
    :param _inputSbs: Path to the .sbs file to cook
    :param _inputGraphPath: Internal path of the graph to render
    :param _context: Path to the default packages folder
    :param _outputCookPath: Output folder path of the .sbsar
    :param _outputRenderPath: Output folder path of the rendered maps
    :param _outputSize: Output size as a power of two
    :param _udim: The udim to process
    :type _context: :class:`context.Context`
    :type _inputSbs: str
    :type _inputGraphPath: str
    :type _outputCookPath: str
    :type _outputRenderPath: str
    :type _outputSize: int
    :type _udim: str
    """
    _sbsarName = os.path.splitext(os.path.basename(_inputSbs))[0]
    _outputName = '%s_%s' % (_sbsarName, _udim)

    batchtools.sbscooker(inputs=_inputSbs,
                         includes=_context.getDefaultPackagePath(),
                         alias=_context.getUrlAliasMgr().getAllAliases(),
                         udim=_udim,
                         output_path=_outputCookPath,
                         output_name=_outputName,
                         compression_mode=2).wait()

    batchtools.sbsrender_render(inputs=os.path.join(_outputCookPath, _outputName+'.sbsar'),
                                input_graph=_inputGraphPath,
                                output_path=_outputRenderPath,
                                output_name=_outputName,
                                set_value='$outputsize@%s,%s' % (_outputSize,_outputSize),
                                png_format_compression="best_speed").wait()

#     cook_sbsar_cmd = [constants.SBS_COOKER_LOCATION, "--inputs", sbs_file_path, "--includes", sd_resources_path,
#                       "--quiet", "--size-limit", "13", "--output-path", output_path]
#     return run_command_popen(cook_sbsar_cmd)
#
#
# def run_command_popen(cmd):
#     success = True
#     sp = subprocess.Popen(cmd, stderr=subprocess.PIPE)
#     out, err = sp.communicate()
#     if err:
#         success = False
#         _LOGGER.error("__________________________\nSubprocess standard error:\nerr.decode('ascii')")
#     sp.wait()
#     return success


def build_sbs_file(sbs_path, texture_set, output_setting):
    _LOGGER.info('\n**********************\nBuild SBS File::::: Path: {}'.format(sbs_path))
    for texture_type, texture_path in texture_set.items():
        _LOGGER.info('TextureSetItem: {}  Path: {}'.format(texture_type, texture_path))

    y_offset = [0, 150, 0]
    start_position = [0, 0, 0]

    try:
        sbs_file = sbsgenerator.createSBSDocument(
            aContext,
            aFileAbsPath=sbs_path,
            aGraphIdentifier='DCCTransferPreview',
            aParameters={
                sbsenum.CompNodeParamEnum.OUTPUT_SIZE: [output_setting, output_setting],
                sbsenum.CompNodeParamEnum.OUTPUT_FORMAT: sbsenum.OutputFormatEnum.FORMAT_16BITS
            },
            aInheritance={
                sbsenum.CompNodeParamEnum.OUTPUT_SIZE:
                    sbsenum.ParamInheritanceEnum.ABSOLUTE,
                sbsenum.CompNodeParamEnum.OUTPUT_FORMAT:
                    sbsenum.ParamInheritanceEnum.ABSOLUTE}
        )

        # Add custom mesh preview
        fbx_object_path = sbs_path.replace('.sbs', '.fbx')
        if os.path.exists(fbx_object_path):
            sbs_file.createLinkedResource(aIdentifier='LowResMesh', aResourcePath=fbx_object_path,
                                          aResourceTypeEnum=sbsenum.ResourceTypeEnum.SCENE)

        node_tree = sbs_file.getSBSGraph(aGraphIdentifier='DCCTransferPreview')

        file_textures = [v for k, v in texture_set.items() if k not in ['type', 'location']]
        for x in range(0, len(file_textures)):
            texture_path = file_textures[x]['path']
            texture_id = os.path.splitext(os.path.basename(texture_path))[0]
            texture_type = texture_id.split('_')[-1]

            if texture_type in constants.OUTPUT_MAPPING.keys():
                y_position = [y * x for y in y_offset]
                bitmap_node = node_tree.createBitmapNode(
                    aSBSDocument=sbs_file,
                    aResourcePath=texture_path,
                    aGUIPos=y_position,
                    aParameters={sbsenum.CompNodeParamEnum.COLOR_MODE: True},
                    aCookedFormat=sbsenum.BitmapFormatEnum.JPG,
                    aCookedQuality=1)

                output_node = node_tree.createOutputNode(
                    aIdentifier=texture_type,
                    aGUIPos=[150, y_position[1]],
                    aOutputFormat=sbsenum.TextureFormatEnum.DEFAULT_FORMAT,
                    aAttributes={sbsenum.AttributesEnum.Description: 'Exported from DCC Package: %s' % texture_type},
                    aMipmaps=sbsenum.MipmapEnum.LEVELS_12,
                    aUsages=constants.OUTPUT_MAPPING[texture_type])

                node_tree.connectNodes(aLeftNode=bitmap_node, aRightNode=output_node,
                                       aRightNodeInput=sbsenum.InputEnum.INPUT_NODE_OUTPUT)

            # Update node position
            start_position += y_offset

        # Write back the document structure into the destination .sbs file
        sbs_file.writeDoc(sbs_path)

        # Create archive file and generate thumbnail
        graph = sbs_file.getSBSGraphList()[0]
        graphPath = 'pkg://' + graph.mIdentifier
        base_sbs_path = os.path.dirname(sbs_path)
        thumbnail_path = sbs_path.replace('.sbs', '_thumbnail.png')
        sd_resources_path = os.path.join(base_sbs_path, 'textures')
        output_path = sbs_path.replace('.sbs', 'sbsar')
        cookAndRender(aContext, sbs_path, graphPath, output_path, thumbnail_path, 512, '1001')
        # (_context, _inputSbs, _inputGraphPath, _outputCookPath, _outputRenderPath, _outputSize, _udim)

        if cook_sbsar(sd_resources_path, sbs_path, output_path):
            _LOGGER.info('Welp it didnt error out!')
            # sbs_thumbnail = set_material_thumbnail(sbs_path)
            # texture_set['thumbnail'] = sbs_thumbnail

        set_json_manifest(sbs_path, texture_set)
        _LOGGER.info('SBS File saved to: {}'.format(sbs_path))
        return True

    except BaseException as error:
        _LOGGER.error('++++ Failed to create SBS File ::::::: {}'.format(error))
        raise error


def get_sbs_values(sbs_path):
    sbs_document = substance.SBSDocument(aContext, sbs_path)
    sbs_document.parseDoc()
    sbs_graph = sbs_document.getSBSGraphList()[0]
    _LOGGER.info('SBS Graph: {}'.format(sbs_graph))


def get_clean_path(path_string):
    return path_string.replace('\\', '/')


def set_json_manifest(target_path, manifest_dict):
    json_output = os.path.join(os.path.dirname(target_path), 'manifest.json')
    output_dict = {target_path: manifest_dict}
    with open(json_output, 'w') as json_manifest:
        json.dump(output_dict, json_manifest, indent=4, sort_keys=True)


def set_material_thumbnail(sbs_path):
    output_path = sbs_path.replace('.sbs', '_thumbnail.png')
    thumbnail.generate(sbs_path, aOutputPath=output_path)
    return output_path