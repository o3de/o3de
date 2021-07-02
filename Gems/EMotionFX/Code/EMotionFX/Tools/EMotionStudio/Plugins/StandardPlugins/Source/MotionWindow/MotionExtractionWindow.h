/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#ifndef __EMSTUDIO_MOTIONEXTRACTIONWINDOW_H
#define __EMSTUDIO_MOTIONEXTRACTIONWINDOW_H

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <MCore/Source/StandardHeaders.h>
#include "../../../../EMStudioSDK/Source/NodeSelectionWindow.h"
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <EMotionFX/Source/PlayBackInfo.h>
#include <QWidget>
#endif


QT_FORWARD_DECLARE_CLASS(QPushButton)
QT_FORWARD_DECLARE_CLASS(QCheckBox)
QT_FORWARD_DECLARE_CLASS(QVBoxLayout)

namespace MysticQt
{
    class LinkWidget;
}

namespace EMStudio
{
    // forward declarations
    class MotionWindowPlugin;

    class MotionExtractionWindow
        : public QWidget
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MotionExtractionWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MotionExtractionWindow(QWidget* parent, MotionWindowPlugin* motionWindowPlugin);
        ~MotionExtractionWindow();

        void Init();

        EMotionFX::EMotionExtractionFlags GetMotionExtractionFlags() const;

    public slots:
        void UpdateInterface();
        void OnMotionExtractionFlagsUpdated();

        void OnSelectMotionExtractionNode();
        void OnMotionExtractionNodeSelected(MCore::Array<SelectionItem> selection);

    private:
        // callbacks
        MCORE_DEFINECOMMANDCALLBACK(CommandSelectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandUnselectCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandClearSelectionCallback);
        MCORE_DEFINECOMMANDCALLBACK(CommandAdjustActorCallback);
        CommandSelectCallback*          mSelectCallback;
        CommandUnselectCallback*        mUnselectCallback;
        CommandClearSelectionCallback*  mClearSelectionCallback;
        CommandAdjustActorCallback*     mAdjustActorCallback;

        // general
        MotionWindowPlugin*             mMotionWindowPlugin;
        QCheckBox*                      mAutoMode;

        // flags widget
        QWidget*                        mFlagsWidget;
        QCheckBox*                      mCaptureHeight;

        //
        QVBoxLayout*                    mMainVerticalLayout;
        QVBoxLayout*                    mChildVerticalLayout;
        QWidget*                        mWarningWidget;
        bool                            mWarningShowed;

        // motion extraction node selection
        NodeSelectionWindow*            mMotionExtractionNodeSelectionWindow;
        AzQtComponents::BrowseEdit*     mWarningSelectNodeLink;

        // helper functions
        void CreateFlagsWidget();
        void CreateWarningWidget();
    };
} // namespace EMStudio


#endif
