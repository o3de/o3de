/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.
#pragma once

enum EHWShaderClass : AZ::u8
{
    eHWSC_Vertex = 0,
    eHWSC_Pixel = 1,
    eHWSC_Geometry = 2,
    eHWSC_Compute = 3,
    eHWSC_Domain = 4,
    eHWSC_Hull = 5,
    eHWSC_Num = 6
};

enum EConstantBufferShaderSlot : AZ::u8
{
    // Reflected constant buffers:
    //    These are built by the shader system using the parameter system at
    //    shader compilation time and their format varies based on the permutation.
    //
    // These are deprecated and are being replaced by well-defined constant buffer definitions in HLSL.
    eConstantBufferShaderSlot_PerBatch = 0,
    eConstantBufferShaderSlot_PerInstanceLegacy = 1,
    eConstantBufferShaderSlot_PerMaterial = 2,
    eConstantBufferShaderSlot_ReflectedCount = eConstantBufferShaderSlot_PerMaterial + 1, // This slot is used only for counting. It's not a real binding value.
    // !Reflected constant buffers

    eConstantBufferShaderSlot_SPIIndex = 3,
    eConstantBufferShaderSlot_PerInstance = 4,
    eConstantBufferShaderSlot_SPI = 5,
    eConstantBufferShaderSlot_SkinQuat = 6,
    eConstantBufferShaderSlot_SkinQuatPrev = 7,
    eConstantBufferShaderSlot_PerSubPass = 8,
    eConstantBufferShaderSlot_PerPass = 9,
    eConstantBufferShaderSlot_PerView = 10,
    eConstantBufferShaderSlot_PerFrame = 11,
    // OpenGLES 3.X guarantees only 12 uniform slots for VS and PS.
    eConstantBufferShaderSlot_Count
};

enum EResourceLayoutSlot : AZ::u8
{
    EResourceLayoutSlot_PerInstanceCB = 0,
    EResourceLayoutSlot_PerMaterialRS = 1,
    EResourceLayoutSlot_PerInstanceExtraRS = 2,
    EResourceLayoutSlot_PerPassRS = 3,

    // TODO: remove once not needed anymore
    EResourceLayoutSlot_PerBatchCB = 4,
    EResourceLayoutSlot_PerInstanceLegacy = 5,

    // Allocate some extra slots for post effects.
    EResourceLayoutSlot_Count = 8
};

enum EReservedTextureSlot : AZ::u8
{
    EReservedTextureSlot_SkinExtraWeights = EFTT_MAX,
    EReservedTextureSlot_AdjacencyInfo = EReservedTextureSlot_SkinExtraWeights,
    EReservedTextureSlot_PatchID = EReservedTextureSlot_AdjacencyInfo,
};

enum EShaderStage : AZ::u8
{
    EShaderStage_Vertex = BIT(eHWSC_Vertex),
    EShaderStage_Pixel = BIT(eHWSC_Pixel),
    EShaderStage_Geometry = BIT(eHWSC_Geometry),
    EShaderStage_Compute = BIT(eHWSC_Compute),
    EShaderStage_Domain = BIT(eHWSC_Domain),
    EShaderStage_Hull = BIT(eHWSC_Hull),

    EShaderStage_Count = eHWSC_Num,
    EShaderStage_None = 0,
    EShaderStage_All = EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Geometry | EShaderStage_Domain | EShaderStage_Hull | EShaderStage_Compute,
    EShaderStage_AllWithoutCompute = EShaderStage_Vertex | EShaderStage_Pixel | EShaderStage_Geometry | EShaderStage_Domain | EShaderStage_Hull
};
AZ_DEFINE_ENUM_BITWISE_OPERATORS(EShaderStage)
#define SHADERSTAGE_FROM_SHADERCLASS(SHADERCLASS) ::EShaderStage(BIT(SHADERCLASS))

const int InlineConstantsShaderSlot = eConstantBufferShaderSlot_PerInstance;

typedef int ShaderSlot;
