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

// Description : Export geometry OBJ file format


#ifndef CRYINCLUDE_EDITOR_EXPORT_OBJEXPORTER_H
#define CRYINCLUDE_EDITOR_EXPORT_OBJEXPORTER_H
#pragma once


#include <IExportManager.h>


class COBJExporter
    : public IExporter
{
public:
    virtual const char* GetExtension() const;
    virtual const char* GetShortDescription() const;
    virtual bool ExportToFile(const char* filename, const Export::IData* pExportData);
    virtual bool ImportFromFile([[maybe_unused]] const char* filename, [[maybe_unused]] Export::IData* pData){return false; };
    virtual void Release();

private:
    const char* TrimFloat(float fValue) const;
    QString MakeRelativePath(const char* pMainFileName, const char* pFileName) const;
};


#endif // CRYINCLUDE_EDITOR_EXPORT_OBJEXPORTER_H
