/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzToolsFramework/AssetBrowser/AssetBrowserFilterModel.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserEntry.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>
#include <ScriptCanvas/Bus/UnitTestVerificationBus.h>

#include <QSharedPointer>
#include <QCollator>
#include <QIcon>
AZ_PUSH_DISABLE_WARNING(4251, "-Wunknown-warning-option") // 4251: 'QImageIOHandler::d_ptr': class 'QScopedPointer<QImageIOHandlerPrivate,QScopedPointerDeleter<T>>' needs to have dll-interface to be used by clients of class 'QImageIOHandler'
#include <QMovie>
#endif
AZ_POP_DISABLE_WARNING

namespace ScriptCanvasEditor
{
    using namespace AzToolsFramework;
    using namespace AzToolsFramework::AssetBrowser;

    class UnitTestBrowserFilterModel
        : public AssetBrowserFilterModel
        , public UnitTestWidgetNotificationBus::Handler
        , public AZ::TickBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(UnitTestBrowserFilterModel, AZ::SystemAllocator);
        explicit UnitTestBrowserFilterModel(QObject* parent = nullptr);
        ~UnitTestBrowserFilterModel();

        QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
        bool setData(const QModelIndex &index, const QVariant &value, int role) Q_DECL_OVERRIDE;
        Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;

        void SetSearchFilter(const QString& filter);

        // ScriptCanvasEditor::UnitTestWidgetNotificationBus
        virtual void OnTestStart(const AZ::Uuid& sourceID) override;
        virtual void OnTestResult(const AZ::Uuid& sourceID, const UnitTestResult& result) override;
        ////

        void GetCheckedScriptsUuidsList(AZStd::vector<AZ::Uuid>& scriptUuids) const;
        bool HasTestResults(AZ::Uuid sourceUuid);
        UnitTestResult* GetTestResult(AZ::Uuid sourceUuid);

        void FlushLatestTestRun();

        // TickBus
        void OnTick(float deltaTime, AZ::ScriptTimePoint /*time*/) override;
        ////

        void SetHoveredIndex(QModelIndex newHoveredIndex);
        void FilterSetup();

        void TestsStart();
        void TestsEnd();

    private:
        Qt::CheckState GetCheckState(QModelIndex index) const;
        Qt::CheckState GetChildrenCheckState(QModelIndex index) const;
        bool SetCheckState(QModelIndex sourceIndex, Qt::CheckState state);
        void UpdateParentsCheckState(QModelIndex sourceIndex);

        AssetBrowserEntry* GetAssetEntry(QModelIndex index) const;

        AZStd::string m_textFilter;

        AZStd::unordered_set<AZ::Uuid> m_checkedScripts;
        AZStd::unordered_map<AZ::Uuid, UnitTestResult> m_testResults;
        mutable QHash<QModelIndex, Qt::CheckState> m_folderCheckStateCache;

        QModelIndex m_hoveredIndex;

        // ICONS
        QMovie m_iconRunning;
        const QIcon m_iconFailedToCompile;
        const QIcon m_iconFailedToCompileOld;
        const QIcon m_iconPassed;
        const QIcon m_iconPassedOld;
        const QIcon m_iconFailed;
        const QIcon m_iconFailedOld;

    protected:
        bool filterAcceptsRow(int source_row, const QModelIndex& source_parent) const override;
    };
}

