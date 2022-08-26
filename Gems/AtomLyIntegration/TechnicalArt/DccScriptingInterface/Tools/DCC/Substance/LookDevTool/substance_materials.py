from pysbs import context
from pysbs import sbsenum
from pysbs import substance
from pysbs import sbsgenerator
from pysbs import batchtools
from pysbs.batchtools import thumbnail
from pathlib import Path

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


_LOGGER = logging.getLogger('Tools.DCC.Substance.LookDevTool.substance_materials')


# Create SAT Context >>>>>>>>>>>>
aContext = context.Context()
aContext.getUrlAliasMgr().setAliasAbsPath(aAliasName='myAlias', aAbsPath='temp')


def create_designer_files(output_setting, material_info):
    _LOGGER.info(f'\n\nCreate Designer Files::::::::::::::({output_setting}')
    for dcc_file, object_data in material_info.items():
        _LOGGER.info('Source File: {}'.format(dcc_file))
        for object_name, texture_values in object_data.items():
            target_directory = texture_values['location']
            sbs_path = get_clean_path(os.path.join(target_directory, '{}.sbs'.format(object_name)))
            build_sbs_file(sbs_path, texture_values, output_setting)


def cookAndRender(context, inputSbs, inputGraphPath, outputCookPath, outputRenderPath, outputSize, udim):
    """
    Call sbscooker with the provided UDIM, and then sbsrender on the resulting .sbsar

    :param context: API execution context
    :param inputSbs: Path to the .sbs file to cook
    :param inputGraphPath: Internal path of the graph to render
    :param context: Path to the default packages folder
    :param outputCookPath: Output folder path of the .sbsar
    :param outputRenderPath: Output folder path of the rendered maps
    :param outputSize: Output size as a power of two
    :param udim: The udim to process
    :type context: :class:`context.Context`
    :type inputSbs: str
    :type inputGraphPath: str
    :type outputCookPath: str
    :type outputRenderPath: str
    :type outputSize: int
    :type udim: str
    """
    sbsar_name = os.path.splitext(os.path.basename(inputSbs))[0]
    output_name = f'{sbsar_name}_{udim}'

    batchtools.sbscooker(inputs=inputSbs,
                         includes=context.getDefaultPackagePath(),
                         alias=context.getUrlAliasMgr().getAllAliases(),
                         udim=udim,
                         output_path=outputCookPath,
                         output_name=output_name,
                         compression_mode=2).wait()

    batchtools.sbsrender_render(inputs=(Path(outputCookPath) / f'{output_name}.sbsar').as_posix(),
                                input_graph=inputGraphPath,
                                output_path=Path(outputRenderPath).parent.as_posix(),
                                output_name=sbsar_name,
                                set_value=f'$outputsize@{outputSize},{outputSize}',
                                png_format_compression="best_speed").wait()


def build_sbs_file(sbs_path, texture_set, output_setting):
    _LOGGER.info('Creating SBS File: {}'.format(sbs_path))
    for texture_type, texture_path in texture_set.items():
        _LOGGER.info('--> TextureSetItem: {}  Path: {}'.format(texture_type, texture_path))

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
                sbsenum.CompNodeParamEnum.OUTPUT_SIZE: sbsenum.ParamInheritanceEnum.ABSOLUTE,
                sbsenum.CompNodeParamEnum.OUTPUT_FORMAT: sbsenum.ParamInheritanceEnum.ABSOLUTE}
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

        # Create Thumbnail
        texture_set['thumbnail'] = set_material_thumbnail(sbs_path)

        # Create archive file (.sbsar)
        graph = sbs_file.getSBSGraphList()[0]
        graph_path = 'pkg://' + graph.mIdentifier
        base_sbs_path = Path(sbs_path).parent.as_posix()
        sd_resources_path = (Path(base_sbs_path) / 'textures').as_posix()
        output_path = Path(sbs_path).parent.as_posix()
        output_render_path = Path(sd_resources_path).parent.as_posix()
        cookAndRender(aContext, sbs_path, graph_path, output_path, output_render_path, output_setting, '1001')

        set_json_manifest(sbs_path, texture_set)
        return True

    except BaseException as error:
        _LOGGER.error('::::::: Failed to create SBS File ::::::: {}'.format(error))
        raise error


def get_sbs_values(sbs_path):
    sbs_document = substance.SBSDocument(aContext, sbs_path)
    sbs_document.parseDoc()
    sbs_graph = sbs_document.getSBSGraphList()[0]
    _LOGGER.info('SBS Graph: {}'.format(sbs_graph))


def get_clean_path(path_string):
    return path_string.replace('\\', '/')


def set_json_manifest(sbs_path, manifest_dict):
    _LOGGER.info('SBS File saved to: {}'.format(sbs_path))
    json_output = Path(sbs_path).parent / 'manifest.json'
    output_dict = {sbs_path: manifest_dict}
    with open(json_output, 'w') as json_manifest:
        json.dump(output_dict, json_manifest, indent=4, sort_keys=True)


def set_material_thumbnail(sbs_path):
    output_path = sbs_path.replace('.sbs', '_thumbnail.png')
    thumbnail.generate(sbs_path, aOutputPath=output_path)
    return output_path



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
