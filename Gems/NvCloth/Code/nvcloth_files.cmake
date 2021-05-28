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

set(FILES
    Include/NvCloth/IClothSystem.h
    Include/NvCloth/ISolver.h
    Include/NvCloth/ICloth.h
    Include/NvCloth/IClothConfigurator.h
    Include/NvCloth/IFabricCooker.h
    Include/NvCloth/ITangentSpaceHelper.h
    Include/NvCloth/Types.h
    Source/Components/ClothComponent.cpp
    Source/Components/ClothComponent.h
    Source/Components/ClothConfiguration.cpp
    Source/Components/ClothConfiguration.h
    Source/Components/ClothComponentMesh/ClothComponentMesh.cpp
    Source/Components/ClothComponentMesh/ClothComponentMesh.h
    Source/Components/ClothComponentMesh/ClothConstraints.cpp
    Source/Components/ClothComponentMesh/ClothConstraints.h
    Source/Components/ClothComponentMesh/ActorClothColliders.cpp
    Source/Components/ClothComponentMesh/ActorClothColliders.h
    Source/Components/ClothComponentMesh/ActorClothSkinning.cpp
    Source/Components/ClothComponentMesh/ActorClothSkinning.h
    Source/Components/ClothComponentMesh/ClothDebugDisplay.cpp
    Source/Components/ClothComponentMesh/ClothDebugDisplay.h
    Source/System/SystemComponent.cpp
    Source/System/SystemComponent.h
    Source/System/Factory.cpp
    Source/System/Factory.h
    Source/System/Solver.cpp
    Source/System/Solver.h
    Source/System/Cloth.cpp
    Source/System/Cloth.h
    Source/System/Fabric.h
    Source/System/FabricCooker.cpp
    Source/System/FabricCooker.h
    Source/System/TangentSpaceHelper.cpp
    Source/System/TangentSpaceHelper.h
    Source/System/NvTypes.h
    Source/System/NvTypes.cpp
    Source/Utils/Allocators.h
    Source/Utils/AssetHelper.h
    Source/Utils/AssetHelper.cpp
    Source/Utils/MeshAssetHelper.cpp
    Source/Utils/MeshAssetHelper.h
)
