"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT



Lumberyard Legacy Mesh Component to Atom Mesh Component Conversion Script
"""
from LegacyConversionHelpers import *
from LegacyMaterialComponentConverter import *


class Mesh_Component_Converter(Component_Converter):
    """
    Converts point lights
    """
    def __init__(self, assetCatalogHelper, statsCollector, normalizedProjectDir):
        Component_Converter.__init__(self, assetCatalogHelper, statsCollector)
        # These are constant for every component in the file
        self.materialComponentConverter = Material_Component_Converter(assetCatalogHelper)
        self.normalizedProjectDir = normalizedProjectDir
        # These need to be reset between each component
        self.newAssetId = ""
        self.oldMaterialRelativePath = ""
        self.oldFbxRelativePathWithoutExtension = ""
    
    def is_this_the_component_im_looking_for(self, xmlElement, parent):
        if "name" in xmlElement.keys() and xmlElement.get("name") == "EditorMeshComponent":
            return True
        return False
    
    def gather_info_for_conversion(self, xmlElement, parent):
        # First, get the data that we need
        #<Class name="EditorMeshComponent" field="element" version="1" type="{FC315B86-3280-4D03-B4F0-5553D7D08432}">
        #   <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #       <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #           <Class name="AZ::u64" field="Id" value="17006471516512517700" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #       </Class>
        #   </Class>
        #   <Class name="MeshComponentRenderNode" field="Static Mesh Render Node" version="1" type="{46FF2BC4-BEF9-4CC4-9456-36C127C310D7}">
        #       <Class name="bool" field="Visible" value="true" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #       <Class name="Asset" field="Static Mesh" value="id={BCD2BB63-338F-53BE-98E1-BF847138B78E}:3250cdc0,type={C2869E3B-DDA0-4E01-8FE3-6770D788866B},hint={objects/airship/airship_pod_outerwalls.cgf}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
        #       <Class name="AzFramework::SimpleAssetReference&lt;LmbrCentral::MaterialAsset&gt;" field="Material Override" version="1" type="{B7B8ECC7-FF89-4A76-A50E-4C6CA2B6E6B4}">
        #           <Class name="SimpleAssetReferenceBase" field="BaseClass1" version="1" type="{E16CA6C5-5C78-4AD9-8E9B-F8C1FB4D1DB8}">
        #               <Class name="AZStd::string" field="AssetPath" value="" type="{03AAAB3F-5C47-5A66-9EBC-D5FA4DB353C9}"/>
        
        for editorMeshComponentChild in xmlElement:
            if "name" in editorMeshComponentChild.keys() and editorMeshComponentChild.get("name") == "MeshComponentRenderNode":
                for meshComponentRenderNodeChild in editorMeshComponentChild:
                    if "name" in meshComponentRenderNodeChild.keys() and meshComponentRenderNodeChild.get("name") == "Asset":
                        # We've found the mesh asset, now extract the assetId and hit
                        assetId = meshComponentRenderNodeChild.get("value")
                        
                        # Legacy assetId looks like this "id={BCD2BB63-338F-53BE-98E1-BF847138B78E}:3250cdc0,type={C2869E3B-DDA0-4E01-8FE3-6770D788866B},hint={objects/airship/airship_pod_outerwalls.cgf}
                        # atomAssetId looks like this "{BCD2BB63-338F-53BE-98E1-BF847138B78E}:########,
                        # newAssetId should look like "id={BCD2BB63-338F-53BE-98E1-BF847138B78E}:########,type={2C7477B6-69C5-45BE-8163-BCD6A275B6D8},hint={objects/airship/airship_pod_outerwalls.fbx.azmodel}"
                                                #value"id={E01FB8B5-D2B3-52D8-BC36-644FC0E3B5F4}:268435463,type={2C7477B6-69C5-45BE-8163-BCD6A275B6D8},hint={objects/props/barrel_01.fbx.azmodel}"
                        # get the relative path to the mesh
                        meshPathStartIndex = assetId.find("hint={")
                        meshPathStartIndex += len("hint={")
                        meshPathEndIndex = assetId.find(".cgf")
                        self.oldFbxRelativePathWithoutExtension = assetId[meshPathStartIndex: meshPathEndIndex]
                        
                        # swap the sub-id for the atom model sub-id
                        atomModelRelativePath = "{0}.fbx.azmodel".format(self.oldFbxRelativePathWithoutExtension)
                        if atomModelRelativePath in self.assetCatalogHelper.relativePathToAssetIdDict:
                            atomAssetId = self.assetCatalogHelper.relativePathToAssetIdDict[atomModelRelativePath]
                            atomSubId = atomAssetId[atomAssetId.find(":") + 1:]
                            # go from string->int->hex->string to get the hex number as a string
                            hexSubId = str(hex(int(atomSubId)))
                            # remove the leading characters to get plain hex
                            hexString = hexSubId[hexSubId.find("x") + 1:]

                            subIdStartIndex = assetId.find(":")
                            subIdStartIndex += 1
                            subIdEndIndex = assetId.find(",")
                            subIdReplacement = hexString

                            # swap the type for the atom model type
                            typeStartIndex = assetId.find("type={")
                            typeStartIndex += len("type={")
                            typeEndIndex = assetId.find("},hint")
                            typeReplacement = "2C7477B6-69C5-45BE-8163-BCD6A275B6D8"

                            # swap the hint for the atom model extension
                            extensionStartIndex = assetId.find(".cgf")
                            extensionEndIndex = extensionStartIndex + len(".cgf")
                            extensionReplacement = ".azmodel"

                            self.newAssetId = "".join((assetId[:subIdStartIndex], subIdReplacement, assetId[subIdEndIndex:typeStartIndex], typeReplacement, assetId[typeEndIndex:extensionStartIndex], extensionReplacement, assetId[extensionEndIndex:]))
                        else:
                            # we couldn't find the atom model (it was probably a .cgf instead of a .fbx in source)
                            self.newAssetId = "id={00000000-0000-0000-0000-000000000000}:00000000,type={2C7477B6-69C5-45BE-8163-BCD6A275B6D8},hint={}"
                            print("Could not find {0} in the asset catalog. Make sure the corresponding source file ends in .fbx not .cgf, and that the asset has finished processing with no errors.".format(atomModelRelativePath)) 
                    elif "field" in meshComponentRenderNodeChild.keys() and meshComponentRenderNodeChild.get("field") == "Material Override":
                        for simpleAssetReferenceChild in meshComponentRenderNodeChild:
                            if "name" in simpleAssetReferenceChild.keys() and simpleAssetReferenceChild.get("name") == "SimpleAssetReferenceBase":
                                for simpleAssetReferenceBaseChild in simpleAssetReferenceChild:
                                    if "field" in simpleAssetReferenceBaseChild.keys() and simpleAssetReferenceBaseChild.get("field") == "AssetPath" and "value" in simpleAssetReferenceBaseChild.keys():
                                        # We've found the material override
                                        self.oldMaterialRelativePath = simpleAssetReferenceBaseChild.get("value")
    
    def create_adapter_with_new_model_assetId(self, assetIdValue):
        #        <Class name="EditorComponentAdapter&lt;AZ::Render::MeshComponentController AZ::Render::MeshComponent AZ::Render::MeshComponentConfig &gt;" field="BaseClass1" version="1" type="{52DFE044-18C1-5861-BA2A-EDB61107FEE9}">
        #            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #                    <Class name="AZ::u64" field="Id" value="4528867579411318958" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #                </Class>
        #            </Class>
        #            <Class name="AZ::Render::MeshComponentController" field="Controller" type="{D0F35FAC-4194-4C89-9487-D000DDB8B272}">
        #                <Class name="AZ::Render::MeshComponentConfig" field="Configuration" type="{63737345-51B1-472B-9355-98F99993909B}">
        #                    <Class name="Asset" field="ModelAsset" value="id={509D78D3-2196-50C2-808C-FEDC3C31380D}:10000007,type={2C7477B6-69C5-45BE-8163-BCD6A275B6D8},hint={objects/suzanne.fbx.azmodel}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
        #                    <Class name="bool" field="ExcludeFromReflectionCubeMaps" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #                </Class>
        #            </Class>
        #        </Class>
        #    </Class>

        #<Class name="AZ::Render::EditorMeshComponent" field="element" version="1" type="{DCE68F6E-2E16-4CB4-A834-B6C2F900A7E9}">
        #    <Class name="EditorRenderComponentAdapter&lt;AZ::Render::MeshComponentController AZ::Render::MeshComponent AZ::Render::MeshComponentConfig &gt;" field="BaseClass1" type="{3D614286-9164-53B5-833B-4F98D2820BA7}">
        #        <Class name="EditorComponentAdapter&lt;AZ::Render::MeshComponentController AZ::Render::MeshComponent AZ::Render::MeshComponentConfig &gt;" field="BaseClass1" version="1" type="{52DFE044-18C1-5861-BA2A-EDB61107FEE9}">
        #            <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #                <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #                    <Class name="AZ::u64" field="Id" value="4528867579411318958" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #                </Class>
        #            </Class>
        #            <Class name="AZ::Render::MeshComponentController" field="Controller" type="{D0F35FAC-4194-4C89-9487-D000DDB8B272}">
        #                <Class name="AZ::Render::MeshComponentConfig" field="Configuration" type="{63737345-51B1-472B-9355-98F99993909B}">
        #                    <Class name="Asset" field="ModelAsset" value="id={509D78D3-2196-50C2-808C-FEDC3C31380D}:10000007,type={2C7477B6-69C5-45BE-8163-BCD6A275B6D8},hint={objects/suzanne.fbx.azmodel}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
        #                    <Class name="bool" field="ExcludeFromReflectionCubeMaps" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #                </Class>
        #            </Class>
        #        </Class>
        #    </Class>
        #</Class>

        editorRenderComponentAdapter = xml.etree.ElementTree.Element("Class", {'name': "EditorRenderComponentAdapter<AZ::Render::MeshComponentController AZ::Render::MeshComponent AZ::Render::MeshComponentConfig >", 'field': "BaseClass1", 'type': "{3D614286-9164-53B5-833B-4F98D2820BA7}"})
                        
        editorComponentAdapter = xml.etree.ElementTree.Element("Class", {'name': "EditorComponentAdapter<AZ::Render::MeshComponentController AZ::Render::MeshComponent AZ::Render::MeshComponentConfig >", 'field': "BaseClass1", 'version': "1", 'type': "{52DFE044-18C1-5861-BA2A-EDB61107FEE9}"})
        
        editorComponentBase = xml.etree.ElementTree.Element("Class", {'name': "EditorComponentBase", 'field': "BaseClass1", 'version': "1", 'type': "{D5346BD4-7F20-444E-B370-327ACD03D4A0}"})
        component = xml.etree.ElementTree.Element("Class", {'name': "AZ::Component", 'field': "BaseClass1", 'type': "{EDFCB2CF-F75D-43BE-B26B-F35821B29247}"})
        u64 = xml.etree.ElementTree.Element("Class", {'name': "AZ::u64", 'field': "Id", 'value': "4528867579411318958", 'type': "{D6597933-47CD-4FC8-B911-63F3E2B0993A}"})
        component.append(u64)
        editorComponentBase.append(component)

        meshComponentController = create_xml_element_from_string("<Class name=\"AZ::Render::MeshComponentController\" field=\"Controller\" type=\"{D0F35FAC-4194-4C89-9487-D000DDB8B272}\">")

        meshComponentConfig = xml.etree.ElementTree.Element("Class", {'name': "AZ::Render::MeshComponentConfig", 'field': "Configuration", 'type': "{63737345-51B1-472B-9355-98F99993909B}"})
        modelAsset = xml.etree.ElementTree.Element("Class", {'name': "Asset", 'field': "ModelAsset", 'value': assetIdValue, 'version': "1", 'type': "{77A19D40-8731-4D3C-9041-1B43047366A4}"})
        excludeFromReflectionCubeMaps = xml.etree.ElementTree.Element("Class", {'name': "bool", 'field': "ExcludeFromReflectionCubeMaps", 'value': "false", 'type': "{A0CA880C-AFE4-43CB-926C-59AC48496112}"})
        meshComponentConfig.append(modelAsset)
        meshComponentConfig.append(excludeFromReflectionCubeMaps)

        meshComponentController.append(meshComponentConfig)

        editorComponentAdapter.append(editorComponentBase)
        editorComponentAdapter.append(meshComponentController)

        editorRenderComponentAdapter.append(editorComponentAdapter)

        return editorRenderComponentAdapter

    def convert(self, xmlElement, parent):
        """
        Returns a list of xml elements (siblings)
        """
        # Now clear the legacy mesh component
        xmlElement.clear()

        # And replace the content with an Atom mesh component
        xmlElement.set("name", "AZ::Render::EditorMeshComponent")
        xmlElement.set("field", "element")
        xmlElement.set("version", "1")
        xmlElement.set("type", "{DCE68F6E-2E16-4CB4-A834-B6C2F900A7E9}")

        xmlElement.append(self.create_adapter_with_new_model_assetId(self.newAssetId))

        if len(self.oldMaterialRelativePath) == 0 or self.oldMaterialRelativePath[0:self.oldMaterialRelativePath.find(".mtl")] == self.oldFbxRelativePathWithoutExtension:
            # There was no material override
            self.statsCollector.noMaterialOverrideCount += 1
        else:
            # There was a material override
            self.statsCollector.materialOverrideCount += 1

        # Now that we have an Atom MeshComponent, we need an Atom MaterialComponent as a neighbor to 'child' (the new mesh component)
        atomMaterialInDefaultSlot = self.materialComponentConverter.convert_legacy_mtl_relative_path_to_atom_material_assetid(self.normalizedProjectDir, self.oldMaterialRelativePath, self.oldFbxRelativePathWithoutExtension)
        isActor = False
        atomMaterialList = self.materialComponentConverter.convert_legacy_mtl_relative_path_to_atom_material_list(self.normalizedProjectDir, self.oldMaterialRelativePath, self.oldFbxRelativePathWithoutExtension, isActor)
        
        parent.append(self.materialComponentConverter.create_material_component_with_material_assignments(atomMaterialInDefaultSlot, atomMaterialList))

    def reset(self):
        self.newAssetId = ""
        self.oldMaterialRelativePath = ""
        self.oldFbxRelativePathWithoutExtension = ""
