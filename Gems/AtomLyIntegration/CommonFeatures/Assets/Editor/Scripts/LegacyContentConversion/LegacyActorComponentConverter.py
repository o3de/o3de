"""
Copyright (c) Contributors to the Open 3D Engine Project.
For complete copyright and license terms please see the LICENSE at the root of this distribution.

SPDX-License-Identifier: Apache-2.0 OR MIT



Lumberyard Actor Component to Actor+Material Component Conversion Script
"""
from LegacyConversionHelpers import *
from LegacyMaterialComponentConverter import *

class Actor_Component_Converter(Component_Converter):
    """
    Converts point lights
    """
    def __init__(self, assetCatalogHelper, statsCollector, normalizedProjectDir):
        Component_Converter.__init__(self, assetCatalogHelper, statsCollector)
        # These are constant for every component in the file
        self.materialComponentConverter = Material_Component_Converter(assetCatalogHelper)
        self.normalizedProjectDir = normalizedProjectDir
        # These need to be reset between each component
        self.oldMaterialRelativePath = ""
        self.oldFbxRelativePathWithoutExtension = ""
    
    def is_this_the_component_im_looking_for(self, xmlElement, parent):
        if "name" in xmlElement.keys() and xmlElement.get("name") == "EditorActorComponent":
            for sibling in list(parent):
                if "name" in sibling.keys() and sibling.get("name") == "EditorMaterialComponent":
                    # There is already a material component on this actor, so we don't need to convert it again
                    return False
            # Found an actor that doesn't have a material component
            return True
        # Not an actor
        return False
    
    def gather_info_for_conversion(self, xmlElement, parent):
        # We don't need to modify the actor component, but we do need to add a material component
        for possibleActorAssetComponent in xmlElement.iter():
            if "field" in possibleActorAssetComponent.keys() and possibleActorAssetComponent.get("field") == "ActorAsset" and "value" in possibleActorAssetComponent.keys():
                assetId = possibleActorAssetComponent.get("value")
                
                actorPathStartIndex = assetId.find("hint={")
                actorPathStartIndex += len("hint={")
                actorPathEndIndex = assetId.find(".actor")
                self.oldFbxRelativePathWithoutExtension = assetId[actorPathStartIndex: actorPathEndIndex]
            elif "field" in possibleActorAssetComponent.keys() and possibleActorAssetComponent.get("field") == "MaterialPerActor":
                # TODO - support the actor component's "MaterialPerLOD"
                for simpleAssetReferenceChild in possibleActorAssetComponent:
                    if "name" in simpleAssetReferenceChild.keys() and simpleAssetReferenceChild.get("name") == "SimpleAssetReferenceBase":
                        for simpleAssetReferenceBaseChild in simpleAssetReferenceChild:
                            if "field" in simpleAssetReferenceBaseChild.keys() and simpleAssetReferenceBaseChild.get("field") == "AssetPath" and "value" in simpleAssetReferenceBaseChild.keys():
                                # We've found the material override
                                self.oldMaterialRelativePath = simpleAssetReferenceBaseChild.get("value")
        
    def convert(self, xmlElement, parent):
        """
        Adds a sibling material component
        """
        

        if len(self.oldMaterialRelativePath) == 0 or self.oldMaterialRelativePath[0:self.oldMaterialRelativePath.find(".mtl")] == self.oldFbxRelativePathWithoutExtension:
            # There was no material override
            self.statsCollector.noMaterialOverrideCount += 1
        else:
            # There was a material override
            print("Material Override: fbx {0} override {1}".format(self.oldFbxRelativePathWithoutExtension, self.oldMaterialRelativePath))
            self.statsCollector.materialOverrideCount += 1

        # We don't modify the xmlElement here because we do not want to replace the old ActorComponet, we just want to add a material component
        atomMaterialInDefaultSlot = self.materialComponentConverter.convert_legacy_mtl_relative_path_to_atom_material_assetid(self.normalizedProjectDir, self.oldMaterialRelativePath, self.oldFbxRelativePathWithoutExtension)
        isActor = True
        atomMaterialList = self.materialComponentConverter.convert_legacy_mtl_relative_path_to_atom_material_list(self.normalizedProjectDir, self.oldMaterialRelativePath, self.oldFbxRelativePathWithoutExtension, isActor)
        parent.append(self.materialComponentConverter.create_material_component_with_material_assignments(atomMaterialInDefaultSlot, atomMaterialList))
        # TODO - protect against running this more than once on an actor? Kind of handled by the 'already converted' log file. Not a problem with meshes because the legacy mesh component gets stripped, but it is an issue with actors since they don't get removed

    def reset(self):
        self.oldMaterialRelativePath = ""
        self.oldFbxRelativePathWithoutExtension = ""
