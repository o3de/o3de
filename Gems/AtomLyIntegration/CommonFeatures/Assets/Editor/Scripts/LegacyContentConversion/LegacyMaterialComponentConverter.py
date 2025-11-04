"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT



Lumberyard Legacy Mesh Component to Atom Mesh Component Conversion Script
"""
from LegacyConversionHelpers import *

class Material_Assignment_Info(object):
    def __init__(self, slotAssetId, assignmentAssetId):
        self.slotAssetId = slotAssetId
        self.assignmentAssetId = assignmentAssetId

class Material_Component_Converter(object):
    """
    Some material related functions. Since there is no material component in legacy, this doesn't inherit from Component_Converter like other similar classes
    """
    def __init__(self, assetCatalogHelper):
        self.assetCatalogHelper = assetCatalogHelper

    def create_material_map_entry(self, slotAssetId, materialAssetId):
        #                       <Class name="AZStd::pair" field="element" type="{F652A87A-0FDF-527C-B0ED-340C074A4874}">
        #                           <Class name="AZ::Render::MaterialAssignmentId" field="value1" version="1" type="{EB603581-4654-4C17-B6DE-AE61E79EDA97}">
        #                               <Class name="AZ::u64" field="lodIndex" value="18446744073709551615" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #                               <Class name="AssetId" field="materialAssetId" version="1" type="{652ED536-3402-439B-AEBE-4A5DBC554085}">
        #                                   <Class name="AZ::Uuid" field="guid" value="{2A4E7DCF-F5D5-55B3-8D41-A4F89398D53C}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
        #                                   <Class name="unsigned int" field="subId" value="31953" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
        #                               </Class>
        #                           </Class>
        #                           <Class name="AZ::Render::MaterialAssignment" field="value2" version="1" type="{C66E5214-A24B-4722-B7F0-5991E6F8F163}">
        #                               <Class name="Asset" field="MaterialAsset" value="id={0BFD18DD-3A64-5240-A272-600301CE821C}:0,type={522C7BE0-501D-463E-92C6-15184A2B7AD8},hint={valena/valenaactor_jumpsuitmat.azmaterial}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
        #                               <Class name="AZStd::unordered_map" field="PropertyOverrides" type="{6E6962E1-04C9-56F9-89C4-361031CC1384}"/>
        #                           </Class>
        #                       </Class>
        pair = create_xml_element_from_string("<Class name=\"AZStd::pair\" field=\"element\" type=\"{F652A87A-0FDF-527C-B0ED-340C074A4874}\">")
        isMapAssignment = True
        pair.append(self.create_material_assignment_id(slotAssetId, isMapAssignment))
        pair.append(self.create_material_assignment(materialAssetId))
        return pair

    def create_material_asset_string_from_assetid(self, assetId):
        hint_path = ""
        if assetId != "{00000000-0000-0000-0000-000000000000}:0":
            hint_path = self.assetCatalogHelper.assetIdToRelativePathDict[assetId]
        return "".join(("id=", assetId, ",type={522C7BE0-501D-463E-92C6-15184A2B7AD8},hint={", hint_path, "},loadBehavior=1"))

    def create_material_assignment_id(self, slotAssetId, isMapAssignment):
        # Material assignment ids are serialized differently for the map vs the EditorMaterialComponentSlot
        field = ""
        if isMapAssignment:
            field = "value1"
        else:
            field = "id"
        
        #       <Class name="AZ::Render::MaterialAssignmentId" field="id" version="1" type="{EB603581-4654-4C17-B6DE-AE61E79EDA97}">
        #           <Class name="AZ::u64" field="lodIndex" value="18446744073709551615" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #           <Class name="AssetId" field="materialAssetId" version="1" type="{652ED536-3402-439B-AEBE-4A5DBC554085}">
        #               <Class name="AZ::Uuid" field="guid" value="{00000000-0000-0000-0000-000000000000}" type="{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}"/>
        #               <Class name="unsigned int" field="subId" value="0" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
        #           </Class>
        #       </Class>
        materialAssignmentId = create_xml_element_from_string("".join(("<Class name=\"AZ::Render::MaterialAssignmentId\" field=\"", field, "\" version=\"1\" type=\"{EB603581-4654-4C17-B6DE-AE61E79EDA97}\">")))
        # TODO - for now, always using the default lod index 18446744073709551615, which applies to all lods that don't have a specific override
        materialAssignmentId_lodIndex = create_xml_element_from_string("<Class name=\"AZ::u64\" field=\"lodIndex\" value=\"18446744073709551615\" type=\"{D6597933-47CD-4FC8-B911-63F3E2B0993A}\"/>")
        materialAssignmentId_AssetId = create_xml_element_from_string("<Class name=\"AssetId\" field=\"materialAssetId\" version=\"1\" type=\"{652ED536-3402-439B-AEBE-4A5DBC554085}\">")
        uuid = get_uuid_from_assetId(slotAssetId)
        materialAssignmentId_AssetId_Uuid = create_xml_element_from_string("".join(("<Class name=\"AZ::Uuid\" field=\"guid\" value=\"", uuid, "\" type=\"{E152C105-A133-4D03-BBF8-3D4B2FBA3E2A}\"/>")))
        subId = get_subid_from_assetId(slotAssetId)
        materialAssignmentId_AssetId_subid = xml.etree.ElementTree.Element("Class", {'name' : "unsigned int", 'field' : "subId", 'value' : subId, 'type' : "{43DA906B-7DEF-4CA8-9790-854106D3F983}"})
        
        materialAssignmentId_AssetId.append(materialAssignmentId_AssetId_Uuid)
        materialAssignmentId_AssetId.append(materialAssignmentId_AssetId_subid)
        
        materialAssignmentId.append(materialAssignmentId_lodIndex)
        materialAssignmentId.append(materialAssignmentId_AssetId)
        return materialAssignmentId

    def create_editor_material_assignment_slot(self, slotAssetId, materialAssetId, isDefaultSlot):
        field = ""
        if isDefaultSlot:
            field = "defaultMaterialSlot"
        else:
            field = "element"
        #   <Class name="EditorMaterialComponentSlot" field="defaultMaterialSlot" version="2" type="{344066EB-7C3D-4E92-B53D-3C9EBD546488}">
        #       <Class name="AZ::Render::MaterialAssignmentId" ...
        #       <Class name="Asset" field="materialAsset" value="id={00000000-0000-0000-0000-000000000000}:0,type={522C7BE0-501D-463E-92C6-15184A2B7AD8},hint={}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
        #       <Class name="AZStd::unordered_map" field="propertyOverrides" type="{6E6962E1-04C9-56F9-89C4-361031CC1384}"/>
        #   </Class>
        materialComponentSlot = create_xml_element_from_string("".join(("<Class name=\"EditorMaterialComponentSlot\" field=\"", field, "\" version=\"4\" type=\"{344066EB-7C3D-4E92-B53D-3C9EBD546488}\">")))
        isMapAssignment = False
        materialAssignmentId = self.create_material_assignment_id(slotAssetId, isMapAssignment)
        materialAsset = self.create_material_asset(materialAssetId)
        defaultPropertyOverrides = self.create_material_property_overrides()
        materialComponentSlot.append(materialAssignmentId)
        materialComponentSlot.append(materialAsset)
        materialComponentSlot.append(defaultPropertyOverrides)
        return materialComponentSlot

    def create_material_assignment(self, materialAssetId):
        #                           <Class name="AZ::Render::MaterialAssignment" field="value2" version="1" type="{C66E5214-A24B-4722-B7F0-5991E6F8F163}">
        #                               <Class name="Asset" field="MaterialAsset" value="id={B175B5BF-E97C-52BD-9DC8-60A9CE05CCC8}:0,type={522C7BE0-501D-463E-92C6-15184A2B7AD8},hint={valena/valenaactor_glovesbootsmat.azmaterial}" version="1" type="{77A19D40-8731-4D3C-9041-1B43047366A4}"/>
        #                               <Class name="AZStd::unordered_map" field="PropertyOverrides" type="{6E6962E1-04C9-56F9-89C4-361031CC1384}"/>
        #                           </Class>
        materialAssignment = create_xml_element_from_string("<Class name=\"AZ::Render::MaterialAssignment\" field=\"value2\" version=\"1\" type=\"{C66E5214-A24B-4722-B7F0-5991E6F8F163}\">")
        materialAssignment.append(self.create_material_asset(materialAssetId))
        materialAssignment.append(self.create_material_property_overrides())
        return materialAssignment

    def create_material_asset(self, materialAssetId):
        materialAssetString = self.create_material_asset_string_from_assetid(materialAssetId)
        materialAsset = xml.etree.ElementTree.Element("Class", {'name' : "Asset", 'field' : "materialAsset", 'value' : materialAssetString, 'version' : "2", 'type' : "{77A19D40-8731-4D3C-9041-1B43047366A4}"})
        return materialAsset

    def create_material_property_overrides(self):
        return create_xml_element_from_string("<Class name=\"AZStd::unordered_map\" field=\"propertyOverrides\" type=\"{6E6962E1-04C9-56F9-89C4-361031CC1384}\"/>")

    def create_material_component_with_material_assignments(self, atomMaterialInDefaultSlotAssetId, materialAssignmentList):
        # TODO - the relative path might not be in the same project/gem folder as the .slice
        
        #<Class name="EditorMaterialComponent" field="element" version="3" type="{02B60E9D-470B-447D-A6EE-7D635B154183}">
        #   <Class name="EditorRenderComponentAdapter&lt;MaterialComponentController MaterialComponent MaterialComponentConfig &gt;" field="BaseClass1" type="{DF046B40-536D-5D59-96EF-7A40DA6191B2}">
        #       <Class name="EditorComponentAdapter&lt;MaterialComponentController MaterialComponent MaterialComponentConfig &gt;" field="BaseClass1" version="1" type="{6C4D4557-3728-56F8-A0FF-A309C3AAB853}">
        #           <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #               <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #                   <Class name="AZ::u64" field="Id" value="6456931760107146363" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #               </Class>
        #           </Class>
        #           <Class name="MaterialComponentController" field="Controller" version="1" type="{34AD7ED0-9866-44CD-93B6-E86840214B91}">
        #               <Class name="MaterialComponentConfig" field="Configuration" version="3" type="{3366C279-32AE-48F6-839B-7700AE117A54}">
        #                   <Class name="ComponentConfig" field="BaseClass1" version="1" type="{0A7929DF-2932-40EA-B2B3-79BC1C3490D0}"/>
        #                   <Class name="AZStd::unordered_map" field="materials" type="{50F6716F-698B-5A6C-AACD-940597FDEC24}">
        #                   ... (map entries)
        #           </Class>
        #       </Class>
        #   </Class>
        editorMaterialComponent = create_xml_element_from_string("<Class name=\"EditorMaterialComponent\" field=\"element\" version=\"5\" type=\"{02B60E9D-470B-447D-A6EE-7D635B154183}\">")
        # can't use create_xml_element_from_string here because of the spaces in the class name
        editorRenderComponentAdapter = xml.etree.ElementTree.Element("Class", {'name' : "EditorRenderComponentAdapter<MaterialComponentController MaterialComponent MaterialComponentConfig >", 'field' : "BaseClass1", 'type' : "{DF046B40-536D-5D59-96EF-7A40DA6191B2}"})
        editorComponentAdapter = xml.etree.ElementTree.Element("Class", {'name' : "EditorComponentAdapter<MaterialComponentController MaterialComponent MaterialComponentConfig >", 'field' : "BaseClass1", 'version' : "1", 'type' : "{6C4D4557-3728-56F8-A0FF-A309C3AAB853}"})
        editorComponentBase = create_xml_element_from_string("<Class name=\"EditorComponentBase\" field=\"BaseClass1\" version=\"1\" type=\"{D5346BD4-7F20-444E-B370-327ACD03D4A0}\">")
        component = create_xml_element_from_string("<Class name=\"AZ::Component\" field=\"BaseClass1\" type=\"{EDFCB2CF-F75D-43BE-B26B-F35821B29247}\">")
        u64 = create_xml_element_from_string("<Class name=\"AZ::u64\" field=\"Id\" value=\"6456931760107146363\" type=\"{D6597933-47CD-4FC8-B911-63F3E2B0993A}\"/>")
        component.append(u64)
        editorComponentBase.append(component)
        editorComponentAdapter.append(editorComponentBase)
        
        materialComponentController = create_xml_element_from_string("<Class name=\"MaterialComponentController\" field=\"Controller\" version=\"1\" type=\"{34AD7ED0-9866-44CD-93B6-E86840214B91}\">")
        materialComponentConfig = create_xml_element_from_string("<Class name=\"MaterialComponentConfig\" field=\"Configuration\" version=\"3\" type=\"{3366C279-32AE-48F6-839B-7700AE117A54}\">")
        componentConfig = create_xml_element_from_string("<Class name=\"ComponentConfig\" field=\"BaseClass1\" version=\"1\" type=\"{0A7929DF-2932-40EA-B2B3-79BC1C3490D0}\"/>")
        materialMap = create_xml_element_from_string("<Class name=\"AZStd::unordered_map\" field=\"materials\" type=\"{50F6716F-698B-5A6C-AACD-940597FDEC24}\">")

        defaultSlot = "{00000000-0000-0000-0000-000000000000}:0"
        for atomMaterial in materialAssignmentList:
            if atomMaterial.assignmentAssetId and atomMaterial.assignmentAssetId != defaultSlot:
                materialMapElement = self.create_material_map_entry(atomMaterial.slotAssetId, atomMaterial.assignmentAssetId)
                materialMap.append(materialMapElement)
        
        materialComponentConfig.append(componentConfig)
        materialComponentConfig.append(materialMap)
        materialComponentController.append(materialComponentConfig)
        editorComponentAdapter.append(materialComponentController)
        editorRenderComponentAdapter.append(editorComponentAdapter)

        isDefaultSlot = True
        defaultMaterialComponentSlot = self.create_editor_material_assignment_slot(defaultSlot, atomMaterialInDefaultSlotAssetId, isDefaultSlot)

        #   <Class name="AZStd::vector" field="materialSlots" type="{7FDDDE36-46C8-5DBC-8566-E792AA358BD9}">
        #        ... (slots)
        isDefaultSlot = False
        materialsSlots = create_xml_element_from_string("Class name=\"AZStd::vector\" field=\"materialSlots\" type=\"{7FDDDE36-46C8-5DBC-8566-E792AA358BD9}\"")
        for atomMaterial in materialAssignmentList:
            if atomMaterial.assignmentAssetId:
                materialSlotElement = self.create_editor_material_assignment_slot(atomMaterial.slotAssetId, atomMaterial.assignmentAssetId, isDefaultSlot)
                materialsSlots.append(materialSlotElement)
            else:
                # Use the default material if none was specified
                materialSlotElement = self.create_editor_material_assignment_slot(atomMaterial.slotAssetId, self.get_default_material_assetid(), isDefaultSlot)
                materialsSlots.append(materialSlotElement)
         
        #   <Class name="AZStd::vector" field="materialSlotsByLod" type="{22E4F3CF-29C1-54AC-89DD-6FD47A657229}">
        #        <Class name="AZStd::vector" field="element" type="{7FDDDE36-46C8-5DBC-8566-E792AA358BD9}">


        editorMaterialComponent.append(editorRenderComponentAdapter)
        editorMaterialComponent.append(defaultMaterialComponentSlot)
        return editorMaterialComponent

    def convert_legacy_mtl_relative_path_to_atom_material_assetid(self, normalizedProjectDir, oldMaterialRelativePath, oldFbxRelativePathWithoutExtension):
        if len(oldMaterialRelativePath) == 0:
            # if no material was used, try to find one that matches the name of the fbx (in the event the fbx only has a single material)
            cacheMaterialRelativePath = "".join((oldFbxRelativePathWithoutExtension, ".azmaterial"))
            if cacheMaterialRelativePath in self.assetCatalogHelper.relativePathToAssetIdDict:
                assetId = self.assetCatalogHelper.relativePathToAssetIdDict[cacheMaterialRelativePath]
                return assetId
            else:
                return self.get_default_material_assetid()

        # TODO - doesn't work if .mtl is in the path
        atomRelativePath = oldMaterialRelativePath.replace('.mtl', '.azmaterial')
        assetId = self.assetCatalogHelper.get_asset_id_from_relative_path(atomRelativePath)
        if assetId:
            return assetId
        
        return self.get_default_material_assetid()

    def get_default_material_assetid(self):
        return "{00000000-0000-0000-0000-000000000000}:0"
        #return "{2A83451E-0FE6-508E-BAA2-6142AAA53C42}:0" # AtomStarterGame\Materials\Magenta.material" - makes it obvious we couldn't find a material

    def convert_legacy_mtl_relative_path_to_atom_material_list(self, normalizedProjectDir, oldMaterialRelativePath, oldFbxRelativePathWithoutExtension, isActor):
        materialList = []

        # Find all the materials produced by the fbx
        cacheFbxPath = ""
        if isActor:
            cacheFbxPath = "".join((oldFbxRelativePathWithoutExtension, ".actor"))
        else:
            cacheFbxPath = "".join((oldFbxRelativePathWithoutExtension, ".fbx.azmodel"))

        if cacheFbxPath in self.assetCatalogHelper.relativePathToAssetIdDict:
            fbxAssetId = self.assetCatalogHelper.relativePathToAssetIdDict[cacheFbxPath]
            # get the guid portion of the id
            subIdSeparatorIndex = fbxAssetId.find(":")
            fbxGuid = fbxAssetId[:subIdSeparatorIndex]
            fbxProductList = self.assetCatalogHelper.assetUuidToAssetIdsDict[fbxGuid]
            for productAssetId in fbxProductList:
                relativePath = self.assetCatalogHelper.assetIdToRelativePathDict[productAssetId]
                if relativePath.endswith(".azmaterial"):
                    # we found a product material.
                    slot = productAssetId
                    assignment = ""

                    # strip the _#### from it
                    extraCharactersIndex = relativePath.rfind("_")
                    convertedRelativePath = "".join((relativePath[:extraCharactersIndex], ".azmaterial"))
                    
                    if convertedRelativePath in self.assetCatalogHelper.relativePathToAssetIdDict:
                        # try to find an atom material with the same name
                        assignment = self.assetCatalogHelper.relativePathToAssetIdDict[convertedRelativePath]
                    elif cacheFbxPath.replace(".azmodel", ".azmaterial") in self.assetCatalogHelper.relativePathToAssetIdDict:
                        # An fbx with only 1 submesh is going to produce an azmaterial that is fbxname_materialname
                        # even though this is often redundant like rivervista_01_RiverVista01MAT.azmaterial.
                        # Legacy .mtl files like this would end up with a rivervista_01.mtl that was a multi-material
                        # but only had a single sub-material called RiverVista01MAT.
                        # The legacy material converter treats this case as a single material, and instead of
                        # naming the file rivervista_01_RiverVista01MAT.material, it just calls it rivervista_01.material.
                        # However, the atom model builder still follows the fbxname_materialname convention, so in this case
                        # we should look and see if the material converter created an rivervist_01.material
                        assignment = self.assetCatalogHelper.relativePathToAssetIdDict[cacheFbxPath.replace(".azmodel", ".azmaterial")]
                    else:
                        assignment = self.get_default_material_assetid()
                        print("Could not match {0} to a corresponding source atom material".format(convertedRelativePath))

                    materialList.append(Material_Assignment_Info(slot, assignment))
        
        return materialList
