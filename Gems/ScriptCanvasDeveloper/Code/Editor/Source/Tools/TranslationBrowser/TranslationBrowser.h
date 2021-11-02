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
#include <QSortFilterProxyModel>
#include <QProgressBar>
#include <QModelIndex>
#include <QIcon>
#include <QHeaderView>
#include <QTableView>
#include <AzQtComponents/Components/StyledDialog.h>
#include <AzQtComponents/Components/StyledDockWidget.h>

#include <AzCore/Component/EntityId.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Debug/TraceMessageBus.h>

#include <ScriptCanvas/Bus/EditorScriptCanvasBus.h>

#include <ScriptCanvasDeveloperEditor/TSGenerateAction.h>
#endif

class QCompleter;
class QPushButton;
class QStringListModel;

namespace Ui
{
    class TranslationBrowser;
}

namespace AzQtComponents
{
    class StyledBusyLabel;
}

namespace ScriptCanvasDeveloper
{

    class TranslationHeaderView : public QTableView
    {
        Q_OBJECT

    public:
        TranslationHeaderView(QWidget* parent = nullptr);
    };


    class BehaviorClassModel : public QAbstractItemModel
    {
        Q_OBJECT

    public:

        struct TreeNode
        {
            QString m_name;
            QString m_type;
            AZ::BehaviorClass* m_behaviorClass;
            const AZ::SerializeContext::ClassData* m_classData;

            TreeNode() {}
            TreeNode(const TreeNode& rhs)
            {
                m_name = rhs.m_name;
                m_type = rhs.m_type;
                m_behaviorClass = rhs.m_behaviorClass;
                m_classData = rhs.m_classData;
            }
            virtual ~TreeNode() {}

            TreeNode(const QString& name, const QString& type, AZ::BehaviorClass* bcClass)
                : m_name(name)
                , m_type(type)
                , m_behaviorClass(bcClass)
                , m_classData(nullptr)
            {}

            TreeNode(const QString& name, const QString& type, const AZ::SerializeContext::ClassData* classData)
                : m_name(name)
                , m_type(type)
                , m_behaviorClass(nullptr)
                , m_classData(classData)
            {}
        };

        const TreeNode* nodeForIndex(const QModelIndex& index) const
        {
            if (!index.isValid())
            {
                return nullptr;
            }

            Q_ASSERT(index.row() >= 0 && index.row() < static_cast<int>(m_topLevelItems.size()));
            return m_topLevelItems[index.row()].get();
        }

        std::vector<std::shared_ptr<TreeNode>> m_topLevelItems;
        size_t Count() const { return m_topLevelItems.size(); }

        BehaviorClassModel();


        void PopulateScriptCanvasNodes();

        /*QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override
        {
            if (row >= 0 && row < m_classes.size() && column == 0)
            {
                return createIndex(row, column);
            }

            return {};
        }

        int rowCount(const QModelIndex& parent = QModelIndex()) const override
        {
            return aznumeric_cast<int>(m_topLevelItems.size());
        }


        int columnCount(const QModelIndex& parent = QModelIndex()) const override
        {
            return 1;
        }


        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
        {
            AZ::BehaviorClass* bcClass = m_classes[index.row()];
            if (bcClass)
            {

                switch (role)
                {
                case Qt::DisplayRole:
                {
                    switch (index.column())
                    {
                    case 0: return bcClass->m_name.c_str();
                    }
                    break;
                }

                }
            }

            return {};
        }


        QModelIndex parent(const QModelIndex& child) const override
        {
            return {};
        }*/

        QModelIndex parent(const QModelIndex& child) const override
        {
            const auto* node = nodeForIndex(child);
            Q_ASSERT(node);

            return QModelIndex();
        }


        QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override
        {
            const TreeNode* parentNode = nullptr;
            if (parent.isValid())
            {
                parentNode = nodeForIndex(parent);
                Q_ASSERT(parentNode);
            }
            const auto& children = m_topLevelItems;
            Q_ASSERT(row >= 0 && row < static_cast<int>(children.size()) && column == 0);
            return createIndex(row, column, const_cast<TreeNode*>(parentNode));
        }

        int rowCount(const QModelIndex&) const override
        {
            return static_cast<int>(m_topLevelItems.size());
        }

        enum DataRoles
        {
            Name = Qt::UserRole + 1,
            BehaviorClass
        };

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
        {
            const auto* node = nodeForIndex(index);
            Q_ASSERT(node);
            switch (role)
            {
            case Qt::DisplayRole:

                if (index.column() == ColumnIndex::Column_Name)
                {
                    return node->m_name;
                }
                else if (index.column() == ColumnIndex::Column_Type)
                {
                    return node->m_type;
                }

            case Qt::EditRole:
            case DataRoles::Name:
                return node->m_name;

            case DataRoles::BehaviorClass:
                return QVariant::fromValue<TreeNode>(*(static_cast<const TreeNode*>(node)));

            case Qt::DecorationRole:
            {
                static const auto defaultIcon = QIcon(":/TreeView/default-icon.svg");
                return defaultIcon;
            }
            default:
                break;
            }
            return {};
        }

        enum ColumnIndex
        {
            Column_Name,
            Column_Type,
            Column_Count
        };

        const char* m_columnNames[static_cast<int>(ColumnIndex::Column_Count)] =
        {
            "Name",
            "Type"
        };

        int columnCount([[maybe_unused]] const QModelIndex& parent) const override
        {
            return static_cast<int>(ColumnIndex::Column_Count);
        }

        QVariant headerData(int section, Qt::Orientation orientation, int role /*= Qt::DisplayRole*/) const override
        {
            if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
            {
                return tr(m_columnNames[section]);
            }

            return QAbstractItemModel::headerData(section, orientation, role);
        }

    };

    Q_DECLARE_METATYPE(BehaviorClassModel::TreeNode);

    class BehaviorClassModelSortFilterProxyModel
        : public QSortFilterProxyModel
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(BehaviorClassModelSortFilterProxyModel, AZ::SystemAllocator, 0);

        BehaviorClassModelSortFilterProxyModel(BehaviorClassModel* behaviorClassModel, QObject* parent = nullptr);

        bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

        void SetInput(const AZStd::string& input)
        {
            m_input = input;
            invalidate();
        }

    protected:

        AZStd::string m_input;

    };

    class TranslationBrowser
        : public AzQtComponents::StyledDialog
        , private AZ::SystemTickBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(TranslationBrowser, AZ::SystemAllocator, 0);

        explicit TranslationBrowser(QWidget* parent = nullptr);
        ~TranslationBrowser();

    public Q_SLOTS:

        void OnFilterChanged(const QString& filterString);

    private:


        static constexpr int ColumnAsset = 0;
        static constexpr int ColumnAction = 1;
        static constexpr int ColumnBrowse = 2;
        static constexpr int ColumnStatus = 3;

        // SystemTickBus::Handler
        void OnSystemTick() override;
        //

        AZStd::unique_ptr<Ui::TranslationBrowser> m_ui;

        AZStd::recursive_mutex m_mutex;

        void closeEvent(QCloseEvent* event) override;

        void Populate();

        AZ::SerializeContext* m_serializeContext;
        AZ::BehaviorContext* m_behaviorContext;

        void PopulateScriptCanvasNodes();
        void PopulateBehaviorContextClasses();

        void OnSelectionChanged();
        void OnDoubleClick();

        void ShowBehaviorClass(AZ::BehaviorClass* behaviorClass);
        void ShowClassData(const AZ::SerializeContext::ClassData*);
        AZStd::string MakeJSON(const ScriptCanvasDeveloperEditor::TranslationGenerator::TranslationFormat& translationRoot);
        void LoadJSONForClass(const AZStd::string& className);

        BehaviorClassModel* m_behaviorContextClassesModel;
        BehaviorClassModelSortFilterProxyModel* m_proxyModel;

        void SaveSource();
        void SaveOverride();
        void Generate();
        void DumpDatabase();
        void ShowOverrideInExplorer();
        void ReloadDatabase();

        enum TranslationMode
        {
            BehaviorClass,
            OnDemandReflection,
            Nodes,
            EBus
        };

        TranslationMode m_translationMode = TranslationMode::BehaviorClass;

        AZStd::string m_selection;

    };

    template <typename T>
    bool ShouldSkip(const T* object)
    {
        using namespace AZ::Script::Attributes;

        // Check for "ignore" attribute for ScriptCanvas
        auto excludeClassAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<ExcludeFlags>*>(AZ::FindAttribute(ExcludeFrom, object->m_attributes));
        const bool excludeClass = excludeClassAttributeData && (static_cast<AZ::u64>(excludeClassAttributeData->Get(nullptr))& static_cast<AZ::u64>(ExcludeFlags::List | ExcludeFlags::Documentation));

        if (excludeClass)
        {
            return true; // skip this class
        }

        return false;
    }


}
