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

// Description : Enumerate Installed Shaders.


#ifndef CRYINCLUDE_EDITOR_SHADERENUM_H
#define CRYINCLUDE_EDITOR_SHADERENUM_H

#pragma once

/*!
 *  CShaderEnum class enumerates shaders installed on system.
 *  It scans all effector files, and gather from them all defined effectors.
 */
class CShaderEnum
{
public:
    struct ShaderDesc
    {
        QString name;
        QString file;
    };

    CShaderEnum();
    virtual ~CShaderEnum();

    //! Enumerate shaders installed on system.
    //! @return Number of enumerated shaders.
    virtual int EnumShaders();

    //! Get number of shaders in system.
    //! @return Number of installed shaders.
    virtual int GetShaderCount() const;

    //! Get name of shader by index.
    //! index must be between 0 and number returned by EnumShaders.
    //! @return Name of shader.
    virtual QString GetShader(int i) const;
    virtual QString GetShaderFile(int i) const;

private:
    bool m_bEnumerated;

    //! Array of shader names.
    std::vector<ShaderDesc> m_shaders;
};


#endif // CRYINCLUDE_EDITOR_SHADERENUM_H
