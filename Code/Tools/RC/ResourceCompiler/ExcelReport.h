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

// Description : ExcelReporter


#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXCELREPORT_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXCELREPORT_H
#pragma once

#include "ExcelExport.h"
#include "ConvertContext.h"

// Base class for custom CryEngine excel exporters
class CExcelReport
    : public CExcelExportBase
{
public:
    bool Export(IResourceCompiler* pRC, const char* filename, std::vector<CAssetFileInfo*>& files);

    void ExportSummary(const std::vector<CAssetFileInfo*>& files);
    void ExportTextures(const std::vector<CAssetFileInfo*>& files);
    void ExportCGF(const std::vector<CAssetFileInfo*>& files);
    void ExportCHR(const std::vector<CAssetFileInfo*>& files);
    void ExportCAF(const std::vector<CAssetFileInfo*>& files);

protected:
    IResourceCompiler* m_pRC;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXCELREPORT_H
