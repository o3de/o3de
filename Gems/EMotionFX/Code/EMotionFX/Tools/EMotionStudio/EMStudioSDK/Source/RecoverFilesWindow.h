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

#if !defined(Q_MOC_RUN)
#include <AzCore/std/containers/vector.h>
#include "EMStudioConfig.h"
#include <QDialog>
#include <QTableWidget>
#endif


namespace EMStudio
{
    class EMSTUDIO_API RecoverFilesWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(RecoverFilesWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_EMSTUDIOSDK)

    public:
        RecoverFilesWindow(QWidget* parent, const AZStd::vector<AZStd::string>& files);
        virtual ~RecoverFilesWindow();

    public slots:
        void Accepted();
        void Rejected();

    private:
        AZStd::string GetOriginalFilenameFromRecoverFile(const char* recoverFilename);

        QTableWidget*                   mTableWidget;
        AZStd::vector<AZStd::string>    mFiles;
    };
} // namespace EMStudio
