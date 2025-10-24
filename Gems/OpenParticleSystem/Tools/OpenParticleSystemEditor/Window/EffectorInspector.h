/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once
#if !defined(Q_MOC_RUN)
#include <Window/ui_EffectorInspector.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzToolsFramework/UI/PropertyEditor/ReflectedPropertyEditor.hxx>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI_Internals.h>
#include <Window/ParticleCommonData.h>
#include <Serializer/ParticleSourceData.h>
#include <ComboBoxWidget.h>
#include <QListWidget>
#include <QMessageBox>
#include <PropertyEditorWidget.h>
#include <EventHandlerWidgets.h>
#include <Document/ParticleDocumentBus.h>
#include <AzCore/Math/Vector3.h>
#endif

namespace Ui
{
    class effectorInspector;
}

namespace OpenParticleSystemEditor
{
    class EffectorInspector
        : public QWidget
        , public AzToolsFramework::IPropertyEditorNotify
        , public OpenParticle::EditorParticleRequestsBus::Handler
        , public ParticleDocumentNotifyBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(EffectorInspector, AZ::SystemAllocator, 0);
        explicit EffectorInspector(QWidget* parent = nullptr);
        virtual ~EffectorInspector();

        QScopedPointer<Ui::effectorInspector> m_ui;
        OpenParticle::ParticleSourceData::DetailInfo* m_detail;
        OpenParticle::ParticleSourceData* m_sourceData;
        void* m_pItemWidget;
        AZStd::string m_widgetName = "";

    protected:
        bool eventFilter(QObject* obj, QEvent* event);
        void OnEditingFinished();
    public:
        void DeleteEventHandler(AZStd::string& moduleName);
        void AddEventHandler(AZ::u8 index);
        void UpdataEventWidgets();
        void UpdataComboBoxWidget(const AZStd::string moduleName, const AZStd::string className, const AZStd::string lastModuleName);
        void UpdatePropertyEditorWidget(const AZStd::string moduleName, const AZStd::string className, bool bCheck);
        void OnClickLineWidget(LineWidgetIndex index, AZStd::string widgetName);
        void Init(AZStd::string widgetName = "");
        void SetCheckBoxEnabled(bool check) const;
        void DefaultDisplay();
        void ResetDefaultDisplay();

    private:
        // AzToolsFramework::IPropertyEditorNotify overrides...
        void BeforePropertyModified(AzToolsFramework::InstanceDataNode*) override;
        void AfterPropertyModified(AzToolsFramework::InstanceDataNode*) override;
        void SetPropertyEditingActive([[maybe_unused]] AzToolsFramework::InstanceDataNode*) override {};
        void SetPropertyEditingComplete(AzToolsFramework::InstanceDataNode*) override {};
        void SealUndoStack() override {};
        void RequestPropertyContextMenu(AzToolsFramework::InstanceDataNode*, const QPoint&) override {};
        void PropertySelectionChanged(AzToolsFramework::InstanceDataNode*, bool) override {};
        void ClearDetailWidget();

        // OpenParticleSystemEditor::ParticleDocumentNotifyBus::Handler interface overrides...
        void OnDocumentOpened([[maybe_unused]] AZ::Data::Asset<OpenParticle::ParticleAsset> particleAsset, [[maybe_unused]] AZStd::string particleAssetPath) override;

        void ClickComboBoxWidget(const AZStd::string className);
        void ClickPropertyEditorWidget(const AZStd::string className);
        void ClickEventWidget(const AZStd::string className);
        void SetClassName(const QString& className, const QIcon& icon);
        // OpenParticle::EditorParticleRequestsBus overrides
        AZStd::vector<AZStd::string> GetEmitterNames() override;
        int GetEmitterIndex(size_t index) override;

        void CheckClassStatu(const AZStd::string& className);
        bool AfterVortexForceModified(AzToolsFramework::InstanceDataNode* pNode);
        bool AfterSpawnRotationModified(AzToolsFramework::InstanceDataNode* pNode);
        bool AfterVector3Modifed(AzToolsFramework::InstanceDataNode* pNode, AZ::u8 classIndex, AZ::u8 moduleIndex);

    private:
        AZ::SerializeContext* m_serializeContext = nullptr;
        ComboBoxWidget* m_comboBoxWidget = nullptr;
        AZStd::unordered_map<AZStd::string, PropertyEditorWidget*> m_propertyEditorWidgets;
        bool m_isShowMessage = false;
        EventHandlerWidgets* m_eventHandlerWidgets = nullptr;
        PropertyEditorWidget* m_propertyEditorWidget = nullptr;
        AZStd::any m_preWarm;
        OpenParticle::UpdateRotateAroundPoint m_updateRotateAroundPoint;
        AZ::Vector3 m_lastAxis;

    private Q_SLOTS:
        void OnComboBoxMaterialChanged();
        void OnComboBoxModelChanged();
        void OnComboBoxSkeletonModelChanged();
    };
} // namespace OpenParticleSystemEditor
