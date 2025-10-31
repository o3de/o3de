"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT



Lumberyard Transform Component to O3de Non-Linear Transform Component Conversion Script
"""
import math
from LegacyConversionHelpers import *

class Transform_Component_Converter(Component_Converter):
    """
    Converts transform components using non-linear scaling
    """
    def __init__(self, assetCatalogHelper, statsCollector, _):
        Component_Converter.__init__(self, assetCatalogHelper, statsCollector)
        self.scale = ""

    def is_this_the_component_im_looking_for(self, xmlElement, parent):
        if "name" in xmlElement.keys() and xmlElement.get("name") == "TransformComponent":
            return True
        return False

    def gather_info_for_conversion(self, xmlElement, _):
        # First, get the data that we need
        # <Class name="TransformComponent" field="element" version="9" type="{27F1E1A1-8D9D-4C3B-BD3A-AFB9762449C0}">
        #     <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #         <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #             <Class name="AZ::u64" field="Id" value="3473792412003464537" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #         </Class>
        #     </Class>
        #     <Class name="EntityId" field="Parent Entity" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
        #         <Class name="AZ::u64" field="id" value="4294967295" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #     </Class>
        #     <Class name="EditorTransform" field="Transform Data" version="2" type="{B02B7063-D238-4F40-A724-405F7A6D68CB}">
        #         <Class name="Vector3" field="Translate" value="0.0000000 0.0000000 0.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
        #         <Class name="Vector3" field="Rotate" value="0.0000000 0.0000000 0.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
        #         <Class name="Vector3" field="Scale" value="1.0000000 2.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
        #         <Class name="bool" field="Locked" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #     </Class>
        #     <Class name="Transform" field="Cached World Transform" value="1.0000000 0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000 1.0000000 0.0000000 0.0000000 0.0000000" type="{5D9958E9-9F1E-4985-B532-FFFDE75FEDFD}"/>
        #     <Class name="EntityId" field="Cached World Transform Parent" version="1" type="{6383F1D3-BB27-4E6B-A49A-6409B2059EAA}">
        #         <Class name="AZ::u64" field="id" value="4294967295" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #     </Class>
        #     <Class name="unsigned int" field="Parent Activation Transform Mode" value="0" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
        #     <Class name="bool" field="IsStatic" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #     <Class name="bool" field="Sync Enabled" value="false" type="{A0CA880C-AFE4-43CB-926C-59AC48496112}"/>
        #     <Class name="unsigned int" field="InterpolatePosition" value="0" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
        #     <Class name="unsigned int" field="InterpolateRotation" value="0" type="{43DA906B-7DEF-4CA8-9790-854106D3F983}"/>
        # </Class>    
        for transformComponentChild in xmlElement:
            if "name" in transformComponentChild.keys() and transformComponentChild.get("name") == "EditorTransform":
                for editorTransformNodeChild in transformComponentChild:
                    if "field" in editorTransformNodeChild.keys() and editorTransformNodeChild.get("field") == "Scale":
                        self.scale = editorTransformNodeChild.get("value")

    def scale_is_uniform(self, scale):
        xyzValues = scale.split(' ')
        if len(xyzValues) != 3:
            return False
        if not math.isclose(float(xyzValues[0]), float(xyzValues[1]), abs_tol = 0.01):
            return False
        if not math.isclose(float(xyzValues[1]), float(xyzValues[2]), abs_tol = 0.01):
            return False
        return True

    def create_non_uniform_scale_component(self, scale):
        # <Class name="EditorNonUniformScaleComponent" field="element" version="1" type="{2933FB4F-B3DA-4CD1-8106-F37300730777}">
        #     <Class name="EditorComponentBase" field="BaseClass1" version="1" type="{D5346BD4-7F20-444E-B370-327ACD03D4A0}">
        #         <Class name="AZ::Component" field="BaseClass1" type="{EDFCB2CF-F75D-43BE-B26B-F35821B29247}">
        #             <Class name="AZ::u64" field="Id" value="11681351500092629784" type="{D6597933-47CD-4FC8-B911-63F3E2B0993A}"/>
        #         </Class>
        #     </Class>
        #     <Class name="Vector3" field="NonUniformScale" value="2.0000000 1.0000000 1.0000000" type="{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"/>
        #     <Class name="ComponentModeDelegate" field="ComponentMode" version="1" type="{635B28F0-601A-43D2-A42A-02C4A88CD9C2}"/>
        # </Class>
        editorNonUniformScaleComponent = xml.etree.ElementTree.Element("Class", {
            'name': "EditorNonUniformScaleComponent",
            'field': "element", 
            'version': "1", 
            'type': "{2933FB4F-B3DA-4CD1-8106-F37300730777}"
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
            'value': "11681351500092629784",
            'type': "{D6597933-47CD-4FC8-B911-63F3E2B0993A}"
        })

        component.append(u64)
        editorComponentBase.append(component)

        vector3 = xml.etree.ElementTree.Element("Class", {
            'name': "Vector3",
            'field': "NonUniformScale", 
            'value': scale, 
            'type': "{8379EB7D-01FA-4538-B64B-A6543B4BE73D}"
        })
        componentModeDelegate = xml.etree.ElementTree.Element("Class", {
            'name': "ComponentModeDelegate",
            'field': "ComponentMode",
            'version': "1",
            'type': "{635B28F0-601A-43D2-A42A-02C4A88CD9C2}"
        })
        
        editorNonUniformScaleComponent.append(editorComponentBase)
        editorNonUniformScaleComponent.append(vector3)
        editorNonUniformScaleComponent.append(componentModeDelegate)

        return editorNonUniformScaleComponent

    def convert(self, xmlElement, parent):
        if self.scale_is_uniform(self.scale):
            return
        
        # Reset scale
        for transformComponentChild in xmlElement:
            if "name" in transformComponentChild.keys() and transformComponentChild.get("name") == "EditorTransform":
                for editorTransformNodeChild in transformComponentChild:
                    if "field" in editorTransformNodeChild.keys() and editorTransformNodeChild.get("field") == "Scale":
                        editorTransformNodeChild.set("value", "1.0000000 1.0000000 1.0000000")

        parent.append(self.create_non_uniform_scale_component(
            self.scale
        ))

    def reset(self):
        self.scale = ""
