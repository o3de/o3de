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
        SceneManagerPlugin*     mPlugin;
        QTableWidget*           mSourceList;
        QTableWidget*           mCurrentList;
        QTableWidget*           mMappingTable;
        QPushButton*            mButtonOpen;
        QPushButton*            mButtonSave;
        QPushButton*            mButtonClear;
        QPushButton*            mButtonGuess;
        QLineEdit*              mLeftEdit;
        QLineEdit*              mRightEdit;
        AzQtComponents::FilteredSearchWidget* m_searchWidgetCurrent;
        AzQtComponents::FilteredSearchWidget* m_searchWidgetSource;
        QIcon*                  mBoneIcon;
        QIcon*                  mNodeIcon;
        QIcon*                  mMeshIcon;
        QIcon*                  mMappedIcon;
        AZStd::vector<size_t>    mCurrentBoneList;
        AZStd::vector<size_t>   mSourceBoneList;
        AZStd::vector<size_t>   mMap;

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
