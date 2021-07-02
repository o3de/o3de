/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
