/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
