/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// include MCore
#if !defined(Q_MOC_RUN)
#include "../StandardPluginsConfig.h"
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/vector.h>
#include <MCore/Source/StandardHeaders.h>
#include <QWidget>
#include <QDialog>
#endif

QT_FORWARD_DECLARE_CLASS(QLineEdit)
QT_FORWARD_DECLARE_CLASS(QTableWidgetItem)
QT_FORWARD_DECLARE_CLASS(QTableWidget)
QT_FORWARD_DECLARE_CLASS(QPushButton)

EMFX_FORWARD_DECLARE(Actor)

namespace AzQtComponents
{
    class FilteredSearchWidget;
}

namespace EMStudio
{
    // forward declaration
    class SceneManagerPlugin;

    class MirrorSetupWindow
        : public QDialog
    {
        Q_OBJECT
        MCORE_MEMORYOBJECTCATEGORY(MirrorSetupWindow, MCore::MCORE_DEFAULT_ALIGNMENT, MEMCATEGORY_STANDARDPLUGINS);

    public:
        MirrorSetupWindow(QWidget* parent, SceneManagerPlugin* plugin);
        ~MirrorSetupWindow();

        void Reinit(bool reInitMap = true);

    public slots:
        void OnCurrentListDoubleClicked(QTableWidgetItem* item);
        void OnMappingTableDoubleClicked(QTableWidgetItem* item);
        void OnMappingTableSelectionChanged();
        void OnCurrentListSelectionChanged();
        void OnSourceListSelectionChanged();
        void OnCurrentTextFilterChanged(const QString& text);
        void OnSourceTextFilterChanged(const QString& text);
        void OnLinkPressed();
        void OnLoadMapping();
        void OnSaveMapping();
        void OnClearMapping();
        void OnBestGuess();


    private:
        SceneManagerPlugin*     m_plugin;
        QTableWidget*           m_sourceList;
        QTableWidget*           m_currentList;
        QTableWidget*           m_mappingTable;
        QPushButton*            m_buttonOpen;
        QPushButton*            m_buttonSave;
        QPushButton*            m_buttonClear;
        QPushButton*            m_buttonGuess;
        QLineEdit*              m_leftEdit;
        QLineEdit*              m_rightEdit;
        AzQtComponents::FilteredSearchWidget* m_searchWidgetCurrent;
        AzQtComponents::FilteredSearchWidget* m_searchWidgetSource;
        QIcon*                  m_boneIcon;
        QIcon*                  m_nodeIcon;
        QIcon*                  m_meshIcon;
        QIcon*                  m_mappedIcon;
        AZStd::vector<size_t>    m_currentBoneList;
        AZStd::vector<size_t>   m_sourceBoneList;
        AZStd::vector<size_t>   m_map;

        void FillCurrentListWidget(EMotionFX::Actor* actor, const QString& filterString);
        void FillSourceListWidget(EMotionFX::Actor* actor, const QString& filterString);
        void FillMappingTable(EMotionFX::Actor* currentActor, EMotionFX::Actor* sourceActor);
        void PerformMapping(size_t currentNodeIndex, size_t sourceNodeIndex);
        void RemoveCurrentSelectedMapping();
        void keyPressEvent(QKeyEvent* event);
        void keyReleaseEvent(QKeyEvent* event);
        bool CheckIfIsMapEmpty() const;
        void UpdateToolBar();
        void UpdateActorMotionSources();
        void InitMappingTableFromMotionSources(EMotionFX::Actor* actor);
        void ApplyCurrentMapAsCommand();
        EMotionFX::Actor* GetSelectedActor() const;
    };
} // namespace EMStudio
