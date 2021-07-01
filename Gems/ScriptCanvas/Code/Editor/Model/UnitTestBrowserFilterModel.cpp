/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <precompiled.h>

#include <AzFramework/StringFunc/StringFunc.h>
#include <Editor/Model/UnitTestBrowserFilterModel.h>
#include <Editor/Model/moc_UnitTestBrowserFilterModel.cpp>
#include <AzToolsFramework/AssetBrowser/AssetBrowserModel.h>
#include <AzToolsFramework/AssetBrowser/Search/Filter.h>

namespace ScriptCanvasEditor
{

    UnitTestBrowserFilterModel::UnitTestBrowserFilterModel(QObject* parent)
        : AssetBrowserFilterModel(parent)
        , m_iconRunning("Icons/AssetBrowser/in_progress.gif")
        , m_iconFailedToCompile     (":/ScriptCanvasEditorResources/Resources/warning_symbol.png")
        , m_iconFailedToCompileOld  (":/ScriptCanvasEditorResources/Resources/warning_symbol_grey.png")
        , m_iconPassed              (":/ScriptCanvasEditorResources/Resources/valid_icon.png")
        , m_iconPassedOld           (":/ScriptCanvasEditorResources/Resources/valid_icon_grey.png")
        , m_iconFailed              (":/ScriptCanvasEditorResources/Resources/error_icon.png")
        , m_iconFailedOld           (":/ScriptCanvasEditorResources/Resources/error_icon_grey.png")
        , m_hoveredIndex(QModelIndex())
    {
        setDynamicSortFilter(true);

        m_showColumn.insert(AssetBrowserModel::m_column);

        UnitTestWidgetNotificationBus::Handler::BusConnect();

        m_iconRunning.setCacheMode(QMovie::CacheMode::CacheAll);
        m_iconRunning.setScaledSize(QSize(14, 14));
        m_iconRunning.start();

        insertColumn(columnCount());
    }

    UnitTestBrowserFilterModel::~UnitTestBrowserFilterModel()
    {
        UnitTestWidgetNotificationBus::Handler::BusDisconnect();
    }

    QVariant UnitTestBrowserFilterModel::data(const QModelIndex &index, int role) const
    {
        QModelIndex sourceIndex = mapToSource(index);

        if (index.column() == 0 && role == Qt::CheckStateRole)
        {
            return QVariant(GetCheckState(sourceIndex));
        }
        else if (role == Qt::DecorationRole)
        {
            AssetBrowserEntry* entry = GetAssetEntry(sourceIndex);

            if (entry == nullptr)
            {
                AZ_Assert(false, "ERROR - index internal pointer not pointing to an AssetEntry. Tree provided by the AssetBrowser invalid?");
                return Qt::PartiallyChecked;
            }

            if (entry->GetEntryType() == AssetBrowserEntry::AssetEntryType::Source)
            {
                SourceAssetBrowserEntry* sourceEntry = static_cast<SourceAssetBrowserEntry*>(entry);
                AZ::Uuid sourceID = sourceEntry->GetSourceUuid();

                if (m_testResults.find(sourceID) != m_testResults.end())
                {
                    UnitTestResult testResult = m_testResults.at(sourceID);

                    if (testResult.m_running)
                    {
                        return m_iconRunning.currentPixmap();
                    }
                    else
                    {
                        if (!testResult.m_compiled)
                        {
                            return testResult.m_latestTestingRound ? m_iconFailedToCompile : m_iconFailedToCompileOld;
                        }
                        else if (testResult.m_completed)
                        {
                            return testResult.m_latestTestingRound ? m_iconPassed : m_iconPassedOld;
                        }
                        else
                        {
                            return testResult.m_latestTestingRound ? m_iconFailed : m_iconFailedOld;
                        }
                    }
                }
            }

            return QVariant();
        }

        return sourceIndex.data(role);
    }

    bool UnitTestBrowserFilterModel::setData(const QModelIndex &index, const QVariant &value, int role)
    {
        m_folderCheckStateCache.clear();

        QModelIndex sourceIndex = mapToSource(index);

        if (index.column() == 0 && role == Qt::CheckStateRole)
        {
            Qt::CheckState state = static_cast<Qt::CheckState>(value.toInt());
            bool result = SetCheckState(sourceIndex, state);
            UpdateParentsCheckState(sourceIndex);

            UnitTestWidgetNotificationBus::Broadcast(&UnitTestWidgetNotifications::OnCheckStateCountChange, static_cast<int>(m_checkedScripts.size()));

            return result;
        }

        return QSortFilterProxyModel::sourceModel()->setData(sourceIndex, value, role);
    }

    Qt::ItemFlags UnitTestBrowserFilterModel::flags(const QModelIndex &index) const
    {
        if (!index.isValid())
        {
            return Qt::NoItemFlags;
        }

        QModelIndex sourceIndex = mapToSource(index);
        Qt::ItemFlags flags = sourceIndex.flags();
        if (index.column() == 0)
        {
            flags |= Qt::ItemIsUserCheckable;
            if (sourceIndex.model()->hasChildren(sourceIndex))
            {
                flags |= Qt::ItemIsTristate;
            }
        }

        return flags;
    }

    void UnitTestBrowserFilterModel::SetSearchFilter(const QString& filter)
    {
        m_textFilter = filter.toUtf8().data();
        invalidateFilter();
    }

    void UnitTestBrowserFilterModel::OnTestStart(const AZ::Uuid& sourceID)
    {
        UnitTestResult testRunning;
        testRunning.m_running = true;
        testRunning.m_consoleOutput = "Test is running...";

        if (HasTestResults(sourceID))
        {
            m_testResults.at(sourceID) = testRunning;
        }
        else
        {
            m_testResults.insert({ sourceID, testRunning });
        }
    }

    void UnitTestBrowserFilterModel::OnTestResult(const AZ::Uuid& sourceID, const UnitTestResult& result)
    {
        m_testResults[sourceID] = result;
    }

    void UnitTestBrowserFilterModel::GetCheckedScriptsUuidsList(AZStd::vector<AZ::Uuid>& scriptUuids) const
    {
        scriptUuids.assign(m_checkedScripts.begin(), m_checkedScripts.end());
    }

    bool UnitTestBrowserFilterModel::HasTestResults(AZ::Uuid sourceUuid)
    {
        return (m_testResults.find(sourceUuid) != m_testResults.end());
    }

    UnitTestResult* UnitTestBrowserFilterModel::GetTestResult(AZ::Uuid sourceUuid)
    {
        return HasTestResults(sourceUuid) ? &m_testResults.at(sourceUuid) : nullptr;
    }

    void UnitTestBrowserFilterModel::FlushLatestTestRun()
    {
        for (auto& testResult : m_testResults)
        {
            testResult.second.m_latestTestingRound = false;
        }
    }

    void UnitTestBrowserFilterModel::OnTick([[maybe_unused]] float deltaTime, AZ::ScriptTimePoint /*time*/)
    {
        Q_EMIT dataChanged(index(0, 1), index(rowCount() - 1, 1));
    }

    Qt::CheckState UnitTestBrowserFilterModel::GetCheckState(QModelIndex sourceIndex) const
    {
        AssetBrowserEntry* entry = GetAssetEntry(sourceIndex);

        if (entry == nullptr)
        {
            AZ_Error("ScriptCanvasEditor", false, "Error - entry was Null pointer");
            return Qt::PartiallyChecked;
        }

        switch (entry->GetEntryType())
        {
            case AssetBrowserEntry::AssetEntryType::Folder:
            {
                if (m_folderCheckStateCache.find(sourceIndex) == m_folderCheckStateCache.end())
                {
                    m_folderCheckStateCache[sourceIndex] = GetChildrenCheckState(sourceIndex);
                }
                
                return m_folderCheckStateCache[sourceIndex];
            }
            case AssetBrowserEntry::AssetEntryType::Source:
            {
                SourceAssetBrowserEntry* sourceEntry = static_cast<SourceAssetBrowserEntry*>(entry);
                AZ::Uuid sourceID = sourceEntry->GetSourceUuid();

                if(m_checkedScripts.find(sourceID) != m_checkedScripts.end())
                {
                    return Qt::Checked;
                }
                else
                {
                    return Qt::Unchecked;
                }
                
                break;
            }
            default:
            {
                AZ_Error("ScriptCanvasEditor", false, "Inconsistent Unit Test Widget tree! (checking state of entry that is not source or folder)");
                return Qt::PartiallyChecked;
            }
        }
    }


    Qt::CheckState UnitTestBrowserFilterModel::GetChildrenCheckState(QModelIndex sourceIndex) const
    {
        if (!sourceIndex.isValid())
        {
            AZ_Error("ScriptCanvasEditor", false, "Inconsistent states for checkboxes in Unit Test Widget tree! (invalid source index)");
            return Qt::PartiallyChecked;
        }

        int rows = sourceModel()->rowCount(sourceIndex);
        if (rows == 0)
        {
            AZ_Error("ScriptCanvasEditor", false, "Inconsistent states for checkboxes in Unit Test Widget tree! (no children detected)");
            return Qt::PartiallyChecked;
        }

        bool checkedChildFound = false;
        bool uncheckedChildFound = false;

        for (int i = 0; i < rows; ++i)
        {
            if (filterAcceptsRow(i, sourceIndex))
            {
                QModelIndex childIndex = sourceModel()->index(i, 0, sourceIndex);

                switch (GetCheckState(childIndex))
                {
                    case Qt::Checked:
                    {
                        checkedChildFound = true;

                        if (uncheckedChildFound)
                        {
                            return Qt::PartiallyChecked;
                        }

                        break;
                    }
                    case Qt::Unchecked:
                    {
                        uncheckedChildFound = true;

                        if (checkedChildFound)
                        {
                            return Qt::PartiallyChecked;
                        }

                        break;
                    }
                    case Qt::PartiallyChecked:
                    {
                        return Qt::PartiallyChecked;
                    }
                    default:
                    {
                        AZ_Error("ScriptCanvasEditor", false, "Inconsistent states for checkboxes in Unit Test Widget tree! (wrong default child state)");
                        return Qt::PartiallyChecked;
                    }
                }
            }
        }

        if (checkedChildFound)
        {
            return Qt::Checked;
        }
        else if (uncheckedChildFound)
        {
            return Qt::Unchecked;
        }

        AZ_Error("ScriptCanvasEditor", false, "Inconsistent tree in Unit Test Widget tree! (folder with no test children shown)");
        return Qt::PartiallyChecked;
    }

    bool UnitTestBrowserFilterModel::SetCheckState(QModelIndex sourceIndex, Qt::CheckState newState)
    {
        QPersistentModelIndex pIndex(sourceIndex);

        if (newState == Qt::PartiallyChecked)
        {
            AZ_Error("ScriptCanvasEditor", false, "Unexpected input state for checkbox in Unit Test Widget tree!");
            return false;
        }

        AssetBrowserEntry* entry = GetAssetEntry(sourceIndex);

        if (entry == nullptr)
        {
            AZ_Error("ScriptCanvasEditor", false, "Error - entry was Null pointer");
            return Qt::PartiallyChecked;
        }

        switch (entry->GetEntryType())
        {
            case AssetBrowserEntry::AssetEntryType::Folder:
            {
                int rowCount = sourceModel()->rowCount(sourceIndex);

                for (int i = 0; i < rowCount; ++i)
                {
                    if (filterAcceptsRow(i, sourceIndex))
                    {
                        bool updateOk = SetCheckState(sourceIndex.model()->index(i, 0, sourceIndex), newState);

                        if (!updateOk)
                        {
                            AZ_Error("ScriptCanvasEditor", false, "Issue with updating children in SetCheckState.");
                            return false;
                        }
                    }
                }

                if (rowCount > 0)
                {
                    Q_EMIT dataChanged(sourceModel()->index(0, 0, sourceIndex), sourceModel()->index(rowCount - 1, 0, sourceIndex));
                }

                break;
            }
            case AssetBrowserEntry::AssetEntryType::Source:
            {
                SourceAssetBrowserEntry* sourceEntry = static_cast<SourceAssetBrowserEntry*>(entry);

                AZ::Uuid sourceID = sourceEntry->GetSourceUuid();

                if ((newState == Qt::Checked && m_checkedScripts.find(sourceID) != m_checkedScripts.end()) ||
                    (newState == Qt::Unchecked && m_checkedScripts.find(sourceID) == m_checkedScripts.end()))
                {
                    return true;
                }

                switch (newState)
                {
                    case Qt::Checked:
                    {
                        m_checkedScripts.insert(sourceID);
                        break;
                    }
                    case Qt::Unchecked:
                    {
                        m_checkedScripts.erase(sourceID);
                        break;
                    }
                }

                QModelIndex changedIndex = mapFromSource(sourceIndex);
                Q_EMIT dataChanged(changedIndex, changedIndex);

                break;
            }
            default:
            {
                AZ_Error("ScriptCanvasEditor", false, "Inconsistent Unit Test Widget tree! (setting state of entry that is not source or folder)");

                return false;
            }
        }

        return true;
    }

    void UnitTestBrowserFilterModel::UpdateParentsCheckState(QModelIndex sourceIndex)
    {
        for(QModelIndex currentParent = sourceIndex.parent(); currentParent.isValid(); currentParent = currentParent.parent())
        {
            QModelIndex changedParent = mapFromSource(currentParent);
            Q_EMIT dataChanged(changedParent, changedParent);
        }
    }

   AssetBrowserEntry* UnitTestBrowserFilterModel::GetAssetEntry(QModelIndex index) const
    {
        if (index.isValid())
        {
            return static_cast<AssetBrowserEntry*>(index.internalPointer());
        }
        else
        {
            AZ_Error("ScriptCanvasEditor", false, "Invalid Source Index provided to GetAssetEntry.");
            return nullptr;
        }
   }

   void UnitTestBrowserFilterModel::SetHoveredIndex(QModelIndex newHoveredIndex)
   {
       m_hoveredIndex = newHoveredIndex;
   }

   void UnitTestBrowserFilterModel::FilterSetup()
   {
       sort(0, Qt::DescendingOrder);

       AssetTypeFilter* typeFilter = new AssetTypeFilter();
       typeFilter->SetAssetType("Script Canvas");
       typeFilter->SetFilterPropagation(AssetBrowserEntryFilter::PropagateDirection::Down);

       SetFilter(FilterConstType(typeFilter));
   }

   void UnitTestBrowserFilterModel::TestsStart()
   {
       AZ::TickBus::Handler::BusConnect();
   }

   void UnitTestBrowserFilterModel::TestsEnd()
   {
       AZ::TickBus::Handler::BusDisconnect();
       Q_EMIT dataChanged(index(0, 1), index(rowCount() - 1, 1));
   }

   bool UnitTestBrowserFilterModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
   {
       QModelIndex index = sourceModel()->index(source_row, 0, source_parent);

       if (!sourceModel()->hasChildren(index))
       {
           /* Do not display leaf - Asset Browser would show the source file as the leaf, but we only care about the product file */
           return false;
       }

       if (!AssetBrowserFilterModel::filterAcceptsRow(source_row, source_parent))
       {
           return false;
       }


       bool forcedByParent = false;

       if (!m_textFilter.empty())
       {

           for (QModelIndex currentParent = source_parent; currentParent.isValid(); currentParent = currentParent.parent())
           {
               AssetBrowserEntry* parentEntry = GetAssetEntry(currentParent);
               AZStd::string parentName = parentEntry->GetDisplayName().toUtf8().data();

               if (AzFramework::StringFunc::Find(parentName.c_str(), m_textFilter.c_str()) != AZStd::string::npos)
               {
                   forcedByParent = true;
                   break;
               }
           }

       }

       AssetBrowserEntry* entry = GetAssetEntry(index);
       AZStd::string testString = entry->GetDisplayName().toUtf8().data();

       switch (entry->GetEntryType())
       {
            case AssetBrowserEntry::AssetEntryType::Folder:
            {
                int rows = sourceModel()->rowCount(index);

                for (int i = 0; i < rows; ++i)
                {
                    if (filterAcceptsRow(i, index))
                    {
                        return true;
                    }
                }
                break;
            }
            case AssetBrowserEntry::AssetEntryType::Source:
            {
                if(AzFramework::StringFunc::StartsWith(testString, "test_"))
                {
                    if (m_textFilter.empty() || AzFramework::StringFunc::Find(testString.c_str(), m_textFilter.c_str()) != AZStd::string::npos || forcedByParent)
                    {
                        return true;
                    }
                }
                break;
            }
       }

       return false;
   }
}
