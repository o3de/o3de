"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT



Lumberyard Legacy Point Light Component to Atom Point Light Component Conversion Script
"""
from LegacyConversionHelpers import *
from LegacyMaterialComponentConverter import *

class Point_Light_Component_Converter(Component_Converter):
    """
    Converts point lights
    """
    def __init__(self, assetCatalogHelper, statsCollector, _):
        Component_Converter.__init__(self, assetCatalogHelper, statsCollector)

        self.color = ""
        self.diffuse_multiplier = ""
        self.point_max_distance = ""

    def convert_legacy_light_intensity_to_atom_intensity(self, diffuse_multiplier: str) -> str:
        return str(float(diffuse_multiplier) * 100)

    def is_this_the_component_im_looking_for(self, xmlElement, parent):
        if "name" in xmlElement.keys() and xmlElement.get("name") == "EditorPointLightComponent":
            return True
        return False

    def gather_info_for_conversion(self, xmlElement, _):
        # First, get the data that we need
        # <Class name="EditorPointLightComponent" field="element" version="1" type="{00818135-138D-42AD-8657-FF3FD38D9E7A}">
        #     <Class name="EditorLightComponent" field="BaseClass1" version="2" type="{7C18B273-5BA3-4E0F-857D-1F30BD6B0733}">
        #         <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #             <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #                 <Class name="AZ::u64" field="Id" value="14732701159327458740" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #             </Class>
        #         </Class>
        #         <Class name="EditorLightConfiguration" field="EditorLightConfiguration" version="1" type="{1D3B114F-8FB2-47BD-9C21-E089F4F37861}">
        #             <Class name="LightConfiguration" field="BaseClass1" version="8" type="{F4CC7BB4-C541-480C-88FC-C5A8F37CC67F}">
        #                 <Class name="unsigned int" field="LightType" value="0" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
        #                 <Class name="bool" field="Visible" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #                 <Class name="bool" field="OnInitially" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #                 <Class name="Color" field="Color" value="1.0000000 1.0000000 1.0000000 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
        #                 <Class name="float" field="DiffuseMultiplier" value="50000.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="SpecMultiplier" value="1.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="bool" field="Ambient" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #                 <Class name="float" field="PointMaxDistance" value="500.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="PointAttenuationBulbSize" value="0.0500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="AreaWidth" value="5.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="AreaHeight" value="5.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="AreaMaxDistance" value="2.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="AreaFOV" value="45.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="ProjectorDistance" value="5.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="ProjectorAttenuationBulbSize" value="0.0500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="ProjectorFOV" value="90.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 <Class name="float" field="ProjectorNearPlane" value="0.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                   ...
        for editorPointLightComponentChild in xmlElement:
            #print("Editor Point Light Component Config")
            for editorLightComponentChild in editorPointLightComponentChild:
                #print("Editor Light Component Config")
                #print(f"Name: {editorLightComponentChild.get('name')}, Field: {editorLightComponentChild.get('field')}")
                if "name" in editorLightComponentChild.keys() and editorLightComponentChild.get("name") == "EditorLightConfiguration":
                    #print(f"Editor Light Config. {editorLightComponentChild.get('name')}, {editorLightComponentChild.get('field')}")
                    for editorLightConfigurationChild in editorLightComponentChild:
                        for lightConfigurationChild in editorLightConfigurationChild:
                            #print(f"Name: {lightConfigurationChild.get('name')}, Field: {lightConfigurationChild.get('field')}")
                            if "field" in lightConfigurationChild.keys() and lightConfigurationChild.get("field") == "Color":
                                self.color = lightConfigurationChild.get("value")
                            elif "field" in lightConfigurationChild.keys() and lightConfigurationChild.get("field") == "DiffuseMultiplier":
                                self.diffuse_multiplier = lightConfigurationChild.get("value")
                            elif "field" in lightConfigurationChild.keys() and lightConfigurationChild.get("field") == "PointMaxDistance":
                                self.point_max_distance = lightConfigurationChild.get("value")

        #print(f"Color: {self.color}")
        #print(f"Diffuse: {self.diffuse_multiplier}")
        #print(f"Attenuation Dist.: {self.point_max_distance}")
        print('Editor Light Component was touched!')

    def __create_adapter_xml(self, color, intensity, attenuationRadius):
        # <Class name="AZ::Render::EditorPointLightComponent" field="element" version="1" type="{C4D354BE-5247-41FD-9A8D-550C6772EE5B}">
        #     <Class name="EditorRenderComponentAdapter&lt;AZ::Render::PointLightComponentController AZ::Render::PointLightComponent PointLightComponentConfi" field="BaseClass1" type="{B09B7A31-789F-5996-AD50-EF71942A5271}">
        #         <Class name="EditorComponentAdapter&lt;AZ::Render::PointLightComponentController AZ::Render::PointLightComponent PointLightComponentConfig &gt;" field="BaseClass1" version="1" type="{DC9066D5-4557-52C7-B901-4B5626C4F35A}">
        #             <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #                 <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #                     <Class name="AZ::u64" field="Id" value="12988262800448744331" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #                 </Class>
        #             </Class>
        #             <Class name="AZ::Render::PointLightComponentController" field="Controller" version="1" type="{23F82E30-2E1F-45FE-A9A7-B15632ED9EBD}">
        #                 <Class name="PointLightComponentConfig" field="Configuration" version="2" type="{B6FC35BA-D22F-4C20-BFFC-3FE7A48858FA}">
        #                     <Class name="ComponentConfig" field="BaseClass1" version="1" type="{0A7929DF-2932-40EA-B2B3-79BC1C3490D0}"/>
        #                     <Class name="Color" field="Color" value="1.0000000 1.0000000 1.0000000 1.0000000" type="{7894072A-9050-4F0F-901B-34B1A0D29417}"/>
        #                     <Class name="char" field="ColorIntensityMode" value="0" type="{3AB0037F-AF8D-48CE-BCA0-A170D18B2C03}"/>
        #                     <Class name="float" field="Intensity" value="800.0000000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                     <Class name="unsigned char" field="AttenuationRadiusMode" value="1" type="{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"/>
        #                     <Class name="float" field="AttenuationRadius" value="89.4427185" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                     <Class name="float" field="BulbRadius" value="0.0500000" type="{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"/>
        #                 </Class>
        #             </Class>
        #         </Class>
        #     </Class>
        # </Class>
        editorRenderComponentAdapter = xml.etree.ElementTree.Element("Class", {
            'name': "EditorRenderComponentAdapter<AZ::Render::PointLightComponentController AZ::Render::PointLightComponent PointLightComponentConfig >",
            'field': "BaseClass1", 'type': "{B09B7A31-789F-5996-AD50-EF71942A5271}"
        })
        editorComponentAdapter = xml.etree.ElementTree.Element("Class", {
            'name': "EditorComponentAdapter<AZ::Render::PointLightComponentController AZ::Render::PointLightComponent PointLightComponentConfig >",
            'field': "BaseClass1", 'version': "1", 'type': "{DC9066D5-4557-52C7-B901-4B5626C4F35A}"
        })
        editorComponentBase = xml.etree.ElementTree.Element("Class", {
            'name': "EditorComponentBase",
            'field': "BaseClass1",
            'version': "1",
            'type': "{D5346BD4-7F20-444E-B370-327ACD03D4A0}"
        })
        component = xml.etree.ElementTree.Element("Class", {
            'name': "AZ::Component",
            'field': "BaseClass1",
            'type': "{EDFCB2CF-F75D-43BE-B26B-F35821B29247}"
        })
        u64 = xml.etree.ElementTree.Element("Class", {
            'name': "AZ::u64",
            'field': "Id",
            'value': "12988262800448744331",
            'type': "{D6597933-47CD-4FC8-B911-63F3E2B0993A}"
        })
        component.append(u64)
        editorComponentBase.append(component)

        pointLightComponentController = create_xml_element_from_string(
            "<Class name=\"AZ::Render::PointLightComponentController\" field=\"Controller\" version=\"1\" type=\"{23F82E30-2E1F-45FE-A9A7-B15632ED9EBD}\">"
        )
        pointLightComponentConfig = xml.etree.ElementTree.Element("Class", {
            'name': "PointLightComponentConfig",
            'field': "Configuration",
            'version': "2",
            'type': "{B6FC35BA-D22F-4C20-BFFC-3FE7A48858FA}"
        })
        componentConfig = xml.etree.ElementTree.Element("Class", {
            'name': "ComponentConfig",
            'field': "BaseClass1",
            'version': "1",
            'type': "{0A7929DF-2932-40EA-B2B3-79BC1C3490D0}"
        })
        colorConfig = xml.etree.ElementTree.Element("Class", {
            'name': "Color",
            'field': "Color",
            'value': color,
            'type': "{7894072A-9050-4F0F-901B-34B1A0D29417}"
        })
        colorIntesntiyModeConfig = xml.etree.ElementTree.Element("Class", {
            'name': "char",
            'field': "ColorIntensityMode",
            'value': "0",
            'type': "{3AB0037F-AF8D-48CE-BCA0-A170D18B2C03}"
        })
        inensityConfig = xml.etree.ElementTree.Element("Class", {
            'name': "float",
            'field': "Intensity",
            'value': intensity,
            'type': "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"
        })
        attenuationRadiusModeConfig = xml.etree.ElementTree.Element("Class", {
            'name': "unsigned char",
            'field': "AttenuationRadiusMode",
            'value': "1",
            'type': "{72B9409A-7D1A-4831-9CFE-FCB3FADD3426}"
        })
        attenuationRadiusConfig = xml.etree.ElementTree.Element("Class", {
            'name': "float",
            'field': "AttenuationRadius",
            'value': attenuationRadius,
            'type': "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"
        })
        bulbRadiusConfig = xml.etree.ElementTree.Element("Class", {
            'name': "float",
            'field': "BulbRadius",
            'value': "0.05",
            'type': "{EA2C3E90-AFBE-44D4-A90D-FAAF79BAF93D}"
        })
        pointLightComponentConfig.append(componentConfig)
        pointLightComponentConfig.append(colorConfig)
        pointLightComponentConfig.append(colorIntesntiyModeConfig)
        pointLightComponentConfig.append(inensityConfig)
        pointLightComponentConfig.append(attenuationRadiusModeConfig)
        pointLightComponentConfig.append(attenuationRadiusConfig)
        pointLightComponentConfig.append(bulbRadiusConfig)

        pointLightComponentController.append(pointLightComponentConfig)

        editorComponentAdapter.append(editorComponentBase)
        editorComponentAdapter.append(pointLightComponentController)

        editorRenderComponentAdapter.append(editorComponentAdapter)

        return editorRenderComponentAdapter

    def convert(self, xmlElement, _):
        # Now clear the legacy component
        xmlElement.clear()

        # And replace the content with an Atom point light component
        xmlElement.set("name", "AZ::Render::EditorPointLightComponent")
        xmlElement.set("field", "element")
        xmlElement.set("version", "1")
        xmlElement.set("type", "{C4D354BE-5247-41FD-9A8D-550C6772EE5B}")

        xmlElement.append(self.__create_adapter_xml(
            self.color,
            self.convert_legacy_light_intensity_to_atom_intensity(self.diffuse_multiplier),
            self.point_max_distance
        ))

    def reset(self):
        self.color = ""
        self.diffuse_multiplier = ""
        self.point_max_distance = ""
