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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MATERIALDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MATERIALDATA_H
#pragma once


#include "IMaterialData.h"

class MaterialData
    : public IMaterialData
{
public:
    virtual int AddMaterial(const char* name, int id, const void* handle, const char* properties);
    virtual int AddMaterial(const char* name, int id, const char* subMatName, const void* handle, const char* properties);
    virtual int GetMaterialCount() const;
    virtual const char* GetName(int materialIndex) const;
    virtual int GetID(int materialIndex) const;
    virtual const char* GetSubMatName(int materialIndex) const;
    virtual const void* GetHandle(int materialIndex) const;
    virtual const char* GetProperties(int materialIndex) const;

private:
    struct MaterialEntry
    {
        MaterialEntry(const char* a_name, int a_id, const char* a_subMatName, const void* a_handle, const char* a_properties)
            : name(a_name)
            , id(a_id)
            , subMatName(a_subMatName)
            , handle(a_handle)
            , properties(a_properties ? a_properties : "")
        {
        }

        string name;
        int id;
        string subMatName;
        const void* handle;
        string properties;
    };

    std::vector<MaterialEntry> m_materials;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_MATERIALDATA_H
