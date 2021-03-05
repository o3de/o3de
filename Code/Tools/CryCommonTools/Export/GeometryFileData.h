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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYFILEDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYFILEDATA_H
#pragma once


#include "IGeometryFileData.h"
#include "STLHelpers.h"

class GeometryFileData
    : public IGeometryFileData
{
public:
    // IGeometryFileData
    virtual int AddGeometryFile(const void* handle, const char* name, const SProperties& properties);
    virtual const SProperties& GetProperties(int geometryFileIndex) const;
    virtual void SetProperties(int geometryFileIndex, const SProperties& properties);
    virtual int GetGeometryFileCount() const;
    virtual const void* GetGeometryFileHandle(int geometryFileIndex) const;
    virtual const char* GetGeometryFileName(int geometryFileIndex) const;

private:
    struct GeometryFileEntry
    {
        GeometryFileEntry(const void* a_handle, const char* a_name, const SProperties& a_properties)
            : handle(a_handle)
            , name(a_name)
            , properties(a_properties)
        {
        }

        const void* handle;
        std::string name;
        SProperties properties;
    };

    std::vector<GeometryFileEntry> m_geometryFiles;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_GEOMETRYFILEDATA_H
