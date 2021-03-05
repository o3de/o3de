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

#ifndef CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXCELEXPORT_H
#define CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXCELEXPORT_H
#pragma once

#include <IXml.h>

// Base class for custom CryEngine excel exporters
class CExcelExportBase
{
public:
    enum CellFlags
    {
        CELL_BOLD = 0x0001,
        CELL_CENTERED = 0x0002,
        CELL_HIGHLIGHT = 0x0004,
    };

    bool SaveToFile(const char* filename) const;

    XmlNodeRef NewWorkbook();
    void InitExcelWorkbook(XmlNodeRef Workbook);

    XmlNodeRef NewWorksheet(const char* name);

    XmlNodeRef AddColumn(const char* name, int nWidth);
    void BeginColumns();
    void EndColumns();

    void AddCell(float number);
    void AddCell(int number);
    void AddCell(uint32 number);
    void AddCell(uint64 number) { AddCell((uint32)number); };
    void AddCell(int64 number) { AddCell((int)number); };
    void AddCell(const char* str, int flags = 0);
    void AddCellAtIndex(int nIndex, const char* str, int flags = 0);
    void SetCellFlags(XmlNodeRef cell, int flags);
    void AddRow();
    void AddCell_SumOfRows(int nRows);
    string GetXmlHeader() const;

    void FreezeFirstRow();
    void AutoFilter(int nRow, int nNumColumns);

protected:
    XmlNodeRef m_Workbook;
    XmlNodeRef m_CurrTable;
    XmlNodeRef m_CurrWorksheet;
    XmlNodeRef m_CurrRow;
    XmlNodeRef m_CurrCell;
    std::vector<string> m_columns;
};

#endif // CRYINCLUDE_TOOLS_RC_RESOURCECOMPILER_EXCELEXPORT_H
