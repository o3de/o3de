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

namespace ScriptCanvasEditor
{
    //! A tool that collects and upgrades all Script Canvas graphs in the asset catalog
    class UpgradeHelper
        : public AzQtComponents::StyledDialog
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(UpgradeHelper, AZ::SystemAllocator, 0);

        UpgradeHelper(QWidget* parent = nullptr);
        ~UpgradeHelper();

    private:

        AZStd::unique_ptr<Ui::UpgradeHelper> m_ui;

        void OpenGraph(AZ::Data::AssetId assetId);
    };
}
