/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#if !defined(Q_MOC_RUN)
#include <QMainWindow>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <Source/Editor/EditorCommon.h>
#endif

namespace Ui
{
    class MipmapSettingWidget;
}

namespace ImageProcessingAtomEditor
{
    class MipmapSettingWidget
        : public QWidget
        , public AzToolsFramework::IPropertyEditorNotify
        , protected EditorInternalNotificationBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MipmapSettingWidget, AZ::SystemAllocator, 0);
        explicit MipmapSettingWidget(EditorTextureSetting& textureSetting, QWidget* parent = nullptr);
        ~MipmapSettingWidget();

        //IPropertyEditorNotify Interface
        void BeforePropertyModified([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode* pNode) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SetPropertyEditingComplete([[maybe_unused]] AzToolsFramework::InstanceDataNode* pNode) override {}
        void SealUndoStack() override {}

    public slots:
        void OnCheckBoxStateChanged(bool checked);

    protected:
        ////////////////////////////////////////////////////////////////////////
        //EditorInternalNotificationBus
        void OnEditorSettingsChanged(bool needRefresh, const AZStd::string& platform) override;
        ////////////////////////////////////////////////////////////////////////

    private:
        void RefreshUI();
        QScopedPointer<Ui::MipmapSettingWidget> m_ui;
        EditorTextureSetting* m_textureSetting;
    };
} //namespace ImageProcessingAtomEditor

