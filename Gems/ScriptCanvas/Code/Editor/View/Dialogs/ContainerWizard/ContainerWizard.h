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
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QDialog>
#include <QIcon>
AZ_POP_DISABLE_WARNING

#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <ScriptCanvas/Core/Core.h>
#endif

namespace Ui
{
    class ContainerWizard;
}

namespace ScriptCanvasEditor
{
    class ContainerTypeLineEdit;

    class ContainerWizard
        : public QDialog
    {
        Q_OBJECT
    private:        
        typedef AZStd::unordered_set< AZ::TypeId > DataTypeSet;

    public:        
        AZ_CLASS_ALLOCATOR(ContainerWizard, AZ::SystemAllocator);

        ContainerWizard(QWidget* parent = nullptr);
        ~ContainerWizard() override;

        void SetActiveScriptCanvasId(const ScriptCanvas::ScriptCanvasId& scriptCanvasId);
        
        void RegisterType(const AZ::TypeId& dataType);
        void ShowWizard(const AZ::TypeId& genericContainerType);

        void accept() override;
        void reject() override;

        void hideEvent(QHideEvent* hideEvent) override;

        bool eventFilter(QObject* object, QEvent* event) override;

        const AZStd::unordered_map< AZ::Crc32, AZ::TypeId >& GetFinalTypeMapping() const;

    public Q_SLOTS:
        void ReparseDisplay();

        void OnFinished(int result);
        void OnContainerTypeChanged(int index);
        void OnTypeChanged(int index, const AZ::TypeId& typeId);
        void OnDataTypeMenuVisibilityChanged(bool visible);

        void ValidateName(const QString& newName);

    Q_SIGNALS:
        
        void ContainerPinned(const AZ::TypeId& typeId);
        void CreateContainerVariable(const AZStd::string& variableName, const AZ::TypeId& typeId);
        
    protected:    
        void OnCreate();
        void OnCancel();        

        void ClearDisplay();

        void InitializeDisplay(const AZ::TypeId& typeId);
        void PopulateMapDisplay();
        void PopulateGeneralDisplay(const AZStd::string& patternFallback = "Type %i", const AZStd::string& singleTypeString = "Type", const AZStd::vector< AZStd::string >& typeLabels = AZStd::vector< AZStd::string>());

    private:

        void RegisterDataType(const AZ::TypeId& dataType);
        void RegisterContainerType(const AZ::TypeId& containerType);

        ContainerTypeLineEdit* GetLineEdit(int paramIndex);

        AZ::SerializeContext* m_serializeContext;

        ScriptCanvas::ScriptCanvasId m_activeScriptCanvasId;

        QAction* m_validationAction;

        QIcon m_invalidIcon;

        bool m_dataTypeMenuVisibile;

        bool m_releaseVariable;
        AZ::u32 m_variableCounter;

        AZ::TypeId                  m_genericType;
        AZStd::vector< AZ::TypeId > m_containerTypes;

        AZStd::vector< ContainerTypeLineEdit* > m_containerTypeLineEdit;

        DataTypeSet m_genericContainerTypes;
        AZStd::vector< AZStd::pair<AZStd::string, AZ::TypeId> > m_genericContainerTypeNames;

        // Temporarily unused. When we can reflect combinations on demand, we can use this list to populate, rather then the pre-generated lists
        AZStd::unordered_map< AZ::TypeId, AZStd::string > m_dataTypes;

        AZStd::unordered_map< AZ::Crc32, DataTypeSet > m_containerDataTypeSets;
        AZStd::unordered_map< AZ::Crc32, AZ::TypeId > m_finalContainerTypeIds;

        AZStd::unique_ptr< Ui::ContainerWizard > m_ui;
    };
}
