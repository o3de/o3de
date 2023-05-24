#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/Atom/Feature/RenderCommon.h
    Include/Atom/Feature/Utils/EditorRenderComponentAdapter.h
    Include/Atom/Feature/Utils/EditorRenderComponentAdapter.inl
    Include/Atom/Feature/Utils/EditorLightingPreset.h
    Source/Utils/EditorLightingPreset.cpp
    Include/Atom/Feature/Utils/EditorModelPreset.h
    Source/Utils/EditorModelPreset.cpp
    Source/EditorCommonSystemComponent.cpp
    Source/EditorCommonSystemComponent.h
    Source/CommonModule.cpp
    Source/Material/ConvertEmissiveUnitFunctorSourceData.cpp
    Source/Material/ConvertEmissiveUnitFunctorSourceData.h
    Source/Material/MaterialConverterSystemComponent.cpp
    Source/Material/MaterialConverterSystemComponent.h
    Source/Material/SubsurfaceTransmissionParameterFunctorSourceData.cpp
    Source/Material/SubsurfaceTransmissionParameterFunctorSourceData.h
    Source/Material/Transform2DFunctorSourceData.cpp
    Source/Material/Transform2DFunctorSourceData.h
    Source/Material/UseTextureFunctorSourceData.cpp
    Source/Material/UseTextureFunctorSourceData.h
)
