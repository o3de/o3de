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
    Include/Atom/Feature/Utils/EditorRenderComponentAdapter.h
    Include/Atom/Feature/Utils/EditorRenderComponentAdapter.inl
    Include/Atom/Feature/Utils/EditorLightingPreset.h
    Source/Utils/EditorLightingPreset.cpp
    Include/Atom/Feature/Utils/EditorModelPreset.h
    Source/Utils/EditorModelPreset.cpp
    Source/EditorCommonSystemComponent.cpp
    Source/EditorCommonSystemComponent.h
    Source/CommonModule.cpp
    Source/RenderCommon.h
    Source/Material/ConvertEmissiveUnitFunctorSourceData.cpp
    Source/Material/ConvertEmissiveUnitFunctorSourceData.h
    Source/Material/MaterialConverterSystemComponent.cpp
    Source/Material/MaterialConverterSystemComponent.h
    Source/Material/ShaderEnableFunctorSourceData.cpp
    Source/Material/ShaderEnableFunctorSourceData.h
    Source/Material/SubsurfaceTransmissionParameterFunctorSourceData.cpp
    Source/Material/SubsurfaceTransmissionParameterFunctorSourceData.h
    Source/Material/Transform2DFunctorSourceData.cpp
    Source/Material/Transform2DFunctorSourceData.h
    Source/Material/UseTextureFunctorSourceData.cpp
    Source/Material/UseTextureFunctorSourceData.h
    Source/Material/PropertyVisibilityFunctorSourceData.cpp
    Source/Material/PropertyVisibilityFunctorSourceData.h
    Source/Material/DrawListFunctorSourceData.cpp
    Source/Material/DrawListFunctorSourceData.h
)
