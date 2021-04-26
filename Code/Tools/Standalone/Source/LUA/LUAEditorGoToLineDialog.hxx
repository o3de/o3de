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

#ifndef LUAEDITOR_GOTOLINE_H
#define LUAEDITOR_GOTOLINE_H

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <QDialog>
#endif

#pragma once

namespace Ui
{
    class goToLineDlg;
}

namespace LUAEditor
{
    //////////////////////////////////////////////////////////////////////////
    // Find Dialog
    class LUAEditorGoToLineDialog 
        : public QDialog
    {
        Q_OBJECT
    public:
        LUAEditorGoToLineDialog(QWidget *parent = 0);
        ~LUAEditorGoToLineDialog();
        
        int getLineNumber() { return m_lineNumber; }
    
    signals:
        void lineNumberChanged(int newNumber);        

    public slots:
        void setLineNumber(int number);

    private:
        Ui::goToLineDlg* m_gui;
        int m_lineNumber;
    private slots:
        void spinBoxLineNumberChanged(int newNumber);
        
    };

}

#endif //LUAEDITOR_FINDDIALOG_H
