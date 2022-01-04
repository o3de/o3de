/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
