/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#if !defined(Q_MOC_RUN)
#include <QAbstractTableModel>
#include <QItemSelectionModel>
#include <QMainWindow>

#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <GraphCanvas/Types/ConstructPresets.h>
#include <GraphCanvas/Types/EntitySaveData.h>
#include <GraphCanvas/Types/Types.h>

#include <GraphCanvas/Editor/AssetEditorBus.h>
#endif

namespace Ui
{
    class ConstructPresetDialog;
}

namespace GraphCanvas
{
    class ConstructPresetsTableModel
        : public QAbstractTableModel
        , public AssetEditorSettingsNotificationBus::Handler
    {
        Q_OBJECT

    private:
        struct PresetStructure
        {
            bool     m_isDefault = false;
            QPixmap* m_pixmap = nullptr;
            AZStd::shared_ptr< ConstructPreset > m_preset;
        };

    public:

        enum ColumnIndex
        {
            Name,
            DefaultPreset,

            Count
        };

        AZ_CLASS_ALLOCATOR(ConstructPresetsTableModel, AZ::SystemAllocator);
        ConstructPresetsTableModel(QObject* parent = nullptr);
        ~ConstructPresetsTableModel();

        void SetEditorId(EditorId editorId);

        // QAbstractTableModel
        int columnCount(const QModelIndex &parent = QModelIndex()) const override;
        int rowCount(const QModelIndex &parent = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

        bool setData(const QModelIndex& index, const QVariant& value, int role) override;

        Qt::ItemFlags flags(const QModelIndex &index) const override;
        ////
        
        void SetConstructType(const ConstructType& constructType);
        void RefreshDisplay(bool refreshPresets = false);

        void RemoveRows(const AZStd::vector<int>& rows);

        // AssetEditorSettingsNotificationBus
        void OnSettingsChanged() override;
        ////

        EditorConstructPresets* GetPresetContainer() const;

    signals:

        void OnPresetModificationBegin();
        void OnPresetModificationEnd();

    private:

        PresetStructure* ModPresetStructureForIndex(const QModelIndex& modelIndex);
        PresetStructure* ModPresetStructureForRow(int row);
        const PresetStructure* FindPresetStructureForIndex(const QModelIndex& modelIndex) const;

        EditorId                         m_editorId;
        ConstructType                    m_constructType;
        
        EditorConstructPresets*          m_presetsContainer;
        AZStd::vector< PresetStructure > m_activePresets;
    };


    class ConstructPresetDialog
        : public QMainWindow
        , public AssetEditorPresetNotificationBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(ConstructPresetDialog, AZ::SystemAllocator);

        ConstructPresetDialog(QWidget* parent);
        virtual ~ConstructPresetDialog();

        void SetEditorId(EditorId editorId);
        void AddConstructType(ConstructType constructType);

        // AssetEditorPresetNotificationBus
        void OnPresetsChanged() override;
        void OnConstructPresetsChanged(ConstructType constructType) override;
        ////

        void showEvent(QShowEvent* showEvent) override;                

        ConstructType GetActiveConstructType() const;
        void SetActiveConstructType(ConstructType constructType) const;

        void OnSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void RemoveSelected();
        void RestoreDefaults();

        void UpdateButtonStates();

        void OnActiveConstructTypeChanged(int currentIndex);
        

    protected:

        void OnPresetModificationBegin();
        void OnPresetModificationEnd();

    private:

        AZStd::unique_ptr<Ui::ConstructPresetDialog> m_ui;

        AZStd::vector< ConstructType >              m_constructType;

        bool                                        m_ignorePresetChanges;
        ConstructPresetsTableModel*                 m_presetsModel;
        EditorId                                    m_editorId;       
    };
}
