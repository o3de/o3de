/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>

AZ_PUSH_DISABLE_WARNING(4244 4251 4800, "-Wunknown-warning-option")
#include <QProgressBar>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzQtComponents/Components/StyledDialog.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ISystem.h>
#include <IConsole.h>
#include <AzCore/Debug/TraceMessageBus.h>
#endif

class QPushButton;

namespace Ui
{
    class UpgradeHelper;
}

namespace ScriptCanvas
{
    class SourceHandle;
}

namespace ScriptCanvasEditor
{
    //! A tool that collects and upgrades all Script Canvas graphs in the asset catalog
    class UpgradeHelper
        : public AzQtComponents::StyledDialog
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(UpgradeHelper, AZ::SystemAllocator);

        UpgradeHelper(QWidget* parent = nullptr);
        ~UpgradeHelper();

    private:

        AZStd::unique_ptr<Ui::UpgradeHelper> m_ui;

        void OpenGraph(const SourceHandle& assetId);
    };
}
