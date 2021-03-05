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

#pragma once

#include <QObject>

namespace AzQtComponents
{
    /**
     * An helper class to deal with all the code related to automatic window decorations
     */
    class AutoCustomWindowDecorations : public QObject
    {
    public:
        enum Mode
        {
            Mode_None = 0, // No auto window decorations
            Mode_Approved = 1, // Nice window decorations for hardcoded types (QMessageBox, QInputDialog (add more as you wish))
            Mode_AnyWindow = 2 // Any widget having the Qt::WindowFlag will get custom window decorations
        };
        explicit AutoCustomWindowDecorations(QObject* parent = nullptr);
        // The default is Mode_Approved
        void setMode(Mode);
    protected:
        bool eventFilter(QObject* watched, QEvent* ev) override;
    private:
        void ensureCustomWindowDecorations(QWidget* w);
        Mode m_mode = Mode_Approved;
    };
} // namespace AzQtComponents
