/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Description : Shaders common interface.
#pragma once

#if defined(LINUX) || defined(APPLE)
  #include <platform.h>
#endif

#include "smartptr.h"
#include "Cry_Vector3.h"
#include "Cry_Color.h"
#include "VertexFormats.h"
#include <Vertex.h>

enum EEfResTextures : int // This needs a fixed size so the enum can be forward declared (needed by IMaterial.h)
{
    EFTT_DIFFUSE = 0,
    EFTT_NORMALS,
    EFTT_SPECULAR,
    EFTT_OPACITY,
    EFTT_SMOOTHNESS,
    EFTT_EMITTANCE,
    EFTT_MAX,
    EFTT_UNKNOWN = EFTT_MAX
};

#if !defined(MAX_JOINT_AMOUNT)
#error MAX_JOINT_AMOUNT is not defined
#endif


//=========================================================================

enum EParamType
{
    eType_UNKNOWN,
    eType_BOOL,
    eType_SHORT,
    eType_INT,
    eType_HALF,
    eType_FLOAT,
    eType_STRING,
    eType_FCOLOR,
    eType_VECTOR,
};

struct IShader;
class CCamera;

union UParamVal
{
    int8 m_Byte;
    bool m_Bool;
    short m_Short;
    int m_Int;
    float m_Float;
    char* m_String;
    float m_Color[4];
    float m_Vector[3];
    CCamera* m_pCamera;
};

struct SShaderParam
{
    AZStd::string m_Name;
    UParamVal m_Value;
    EParamType m_Type;
};

struct IRenderShaderResources
{
    virtual void UpdateConstants(IShader* pSH) = 0;
    // properties
    virtual void SetColorValue(EEfResTextures slot, const ColorF& color) = 0;
    virtual void SetStrengthValue(EEfResTextures slot, float value) = 0;
    virtual AZStd::vector<SShaderParam>& GetParameters() = 0;
};

struct SShaderItem
{
    IShader* m_pShader;
    IRenderShaderResources* m_pShaderResources;
    int32 m_nTechnique;
    uint32 m_nPreprocessFlags;
};

struct IAnimNode;

struct ILightAnimWrapper
    : public _i_reference_target_t
{
public:
    virtual bool Resolve() = 0;
    IAnimNode* GetNode() const { return m_pNode; }

protected:
    ILightAnimWrapper(const char* name)
        : m_name(name)
        , m_pNode(0) {}
    virtual ~ILightAnimWrapper() {}

protected:
    AZStd::string m_name;
    IAnimNode* m_pNode;
};

