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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IGEOMETRYFILEDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IGEOMETRYFILEDATA_H
#pragma once


#include "ExportFileType.h"
#include <string>

class IGeometryFileData
{
public:
    struct SProperties
    {
        int  filetypeInt; // combination of flags from CryFileType
        bool bDoNotMerge;
        bool bUseCustomNormals;
        bool bUseF32VertexFormat;
        bool b8WeightsPerVertex;
        std::string customExportPath;

        SProperties()
            : filetypeInt(CRY_FILE_TYPE_NONE)
            , bDoNotMerge(false)
            , bUseCustomNormals(false)
            , bUseF32VertexFormat(false)
            , b8WeightsPerVertex(false)
        {
        }
    };
public:
    virtual int AddGeometryFile(const void* handle, const char* name, const SProperties& properties) = 0;
    virtual const SProperties& GetProperties(int geometryFileIndex) const = 0;
    virtual int GetGeometryFileCount() const = 0;

    // return an implementation-specific handle (for example a maya Dag Path string, or a MAX node name or whatever)
    // its opaque to the exporter, but  you can cast it yourself.
    virtual const void* GetGeometryFileHandle(int geometryFileIndex) const = 0;
    virtual const char* GetGeometryFileName(int geometryFileIndex) const = 0;
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_IGEOMETRYFILEDATA_H
