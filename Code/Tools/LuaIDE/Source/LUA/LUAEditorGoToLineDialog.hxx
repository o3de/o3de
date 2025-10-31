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
        explicit LUAEditorGoToLineDialog(QWidget *parent = nullptr);
        ~LUAEditorGoToLineDialog() override;
        
        int getLineNumber() const { return m_lineNumber; }
        int getColumnNumber() const { return m_columnNumber; }
    
    public slots:

        void setLineNumber(int line, int column) const;
        void handleLineNumberInput(const QString& input);
        void validateAndAccept();

    private:
        Ui::goToLineDlg* m_gui;
        int m_lineNumber = 0;
        int m_columnNumber = 0;
    };

}

#endif //LUAEDITOR_FINDDIALOG_H
