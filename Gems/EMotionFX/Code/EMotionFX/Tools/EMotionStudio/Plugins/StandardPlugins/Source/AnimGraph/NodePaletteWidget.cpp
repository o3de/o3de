/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "NodePaletteWidget.h"
#include <MCore/Source/LogManager.h>
#include "../../../../EMStudioSDK/Source/EMStudioManager.h"
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/AnimGraphObjectFactory.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/BlendTreeFinalNode.h>
#include "AnimGraphPlugin.h"
#include <QVBoxLayout>
#include <QIcon>
#include <QAction>
#include <QMimeData>
#include <QLabel>
#include <QTextEdit>
#include <QTreeView>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(NodePaletteWidget::EventHandler, EMotionFX::EventHandlerAllocator, 0)

    class NodePaletteModel
        : public QAbstractItemModel
    {
    public:
        explicit NodePaletteModel(AnimGraphPlugin* plugin, QObject* parent = nullptr);

        QModelIndex index(int row, int column, const QModelIndex& parent = {}) const override;
        QModelIndex parent(const QModelIndex& index) const override;

        int columnCount(const QModelIndex& parent = {}) const override;
        int rowCount(const QModelIndex& parent = {}) const override;

        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

        Qt::ItemFlags flags(const QModelIndex& index) const override;

        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;

        void setNode(EMotionFX::AnimGraphNode* node);

        const auto& GetCategoryNames() const { return m_categoryNames; }

    private:
        void initializeGroups();

        struct CategoryGroup
        {
            EMotionFX::AnimGraphNode::ECategory m_category;
            AZStd::vector<AZStd::pair<EMotionFX::AnimGraphObject*, /*enabled=*/bool>> m_nodes;
            bool m_enabled = true;
        };

        AnimGraphPlugin* m_plugin;
        EMotionFX::AnimGraphNode* m_node = nullptr;
        AZStd::vector<AZStd::unique_ptr<CategoryGroup>> m_groups;

        // Registered categories and names for this model.
        AZStd::unordered_map<EMotionFX::AnimGraphNode::ECategory, QString> m_categoryNames;
    };

    NodePaletteModel::NodePaletteModel(AnimGraphPlugin* plugin, QObject* parent)
        : QAbstractItemModel(parent)
        , m_plugin(plugin)
    {
        m_categoryNames.emplace(EMotionFX::AnimGraphNode::CATEGORY_SOURCES, NodePaletteWidget::tr("Sources"));
        m_categoryNames.emplace(EMotionFX::AnimGraphNode::CATEGORY_BLENDING, NodePaletteWidget::tr("Blending"));
        m_categoryNames.emplace(EMotionFX::AnimGraphNode::CATEGORY_CONTROLLERS, NodePaletteWidget::tr("Controllers"));
        m_categoryNames.emplace(EMotionFX::AnimGraphNode::CATEGORY_PHYSICS, NodePaletteWidget::tr("Physics"));
        m_categoryNames.emplace(EMotionFX::AnimGraphNode::CATEGORY_LOGIC, NodePaletteWidget::tr("Logic"));
        m_categoryNames.emplace(EMotionFX::AnimGraphNode::CATEGORY_MATH, NodePaletteWidget::tr("Math"));
        m_categoryNames.emplace(EMotionFX::AnimGraphNode::CATEGORY_MISC, NodePaletteWidget::tr("Misc"));
    }

    QModelIndex NodePaletteModel::index(int row, int column, const QModelIndex& parent) const
    {
        if (!parent.isValid())
        {
            if (row >= 0 && row < static_cast<int>(m_groups.size()))
            {
                return createIndex(row, column, nullptr);
            }
        }
        else
        {
            if (parent.row() >= 0 && parent.row() < static_cast<int>(m_groups.size()))
            {
                const auto& group = m_groups[parent.row()];
                if (row >= 0 && row < static_cast<int>(group->m_nodes.size()))
                {
                    return createIndex(row, column, const_cast<CategoryGroup*>(group.get()));
                }
            }
        }
        return {};
    }

    QModelIndex NodePaletteModel::parent(const QModelIndex& index) const
    {
        if (!index.isValid())
        {
            return {};
        }
        if (const auto* group = static_cast<CategoryGroup*>(index.internalPointer()))
        {
            auto it = std::find_if(m_groups.begin(), m_groups.end(),
                    [category = group->m_category](const auto& group) { return group->m_category == category; });
            if (it != m_groups.end())
            {
                return createIndex(aznumeric_cast<int>(std::distance(m_groups.begin(), it)), 0, nullptr);
            }
        }
        return {};
    }

    int NodePaletteModel::columnCount(const QModelIndex&) const
    {
        return 1;
    }

    int NodePaletteModel::rowCount(const QModelIndex& parent) const
    {
        if (!parent.isValid())
        {
            return static_cast<int>(m_groups.size());
        }
        else if (parent.internalPointer() == nullptr && parent.row() >= 0 && parent.row() < static_cast<int>(m_groups.size()))
        {
            const auto& group = m_groups[parent.row()];
            return static_cast<int>(group->m_nodes.size());
        }
        return 0;
    }

    QVariant NodePaletteModel::data(const QModelIndex& index, int role) const
    {
        if (!hasIndex(index.row(), index.column(), index.parent()))
        {
            return {};
        }

        if (const auto* group = static_cast<CategoryGroup*>(index.internalPointer()))
        {
            const auto& nodePair = group->m_nodes[index.row()];
            EMotionFX::AnimGraphObject* node = nodePair.first;
            switch (role)
            {
            case Qt::DisplayRole:
            {
                return node->GetPaletteName();
            }
            case Qt::DecorationRole:
            {
                return NodePaletteWidget::GetNodeIcon(static_cast<const EMotionFX::AnimGraphNode*>(node));
            }
            case Qt::UserRole:
            {
                return azrtti_typeid(node).ToString<AZStd::string>().c_str();
            }
            default:
                break;
            }
        }
        else
        {
            switch (role)
            {
            case Qt::DisplayRole:
            {
                const EMotionFX::AnimGraphNode::ECategory category = m_groups[index.row()]->m_category;

                const auto& categoryNames = m_categoryNames;
                auto it = categoryNames.find(category);
                if (it != categoryNames.end())
                {
                    return it->second;
                }
                break;
            }
            default:
                break;
            }
        }
        return {};
    }

    Qt::ItemFlags NodePaletteModel::flags(const QModelIndex& index) const
    {
        Qt::ItemFlags flags = QAbstractItemModel::flags(index);

        if (const auto* group = static_cast<CategoryGroup*>(index.internalPointer()))
        {
            const auto& nodePair = group->m_nodes[index.row()];

            // Node item in the tree view is disabled.
            if (!nodePair.second)
            {
                flags.setFlag(Qt::ItemIsEnabled, false);
            }
            // Node item in the tree view is enabled.
            else
            {
                flags.setFlag(Qt::ItemIsDragEnabled, true);
            }
        }

        return flags;
    }

    QStringList NodePaletteModel::mimeTypes() const
    {
        return {QStringLiteral("text/plain")};
    }

    QMimeData* NodePaletteModel::mimeData(const QModelIndexList& indexes) const
    {
        if (indexes.isEmpty())
        {
            return nullptr;
        }

        const auto& index = indexes.first();

        auto* mimeData = new QMimeData;
        QString textData = QStringLiteral("EMotionFX::AnimGraphNode;");
        textData += index.data(Qt::UserRole).toString();
        textData += ";" + index.data(Qt::DisplayRole).toString(); // add the palette name as generated name prefix (spaces will be removed from it
        mimeData->setText(textData);

        return mimeData;
    }

    void NodePaletteModel::setNode(EMotionFX::AnimGraphNode* node)
    {
        if (node == m_node)
        {
            return;
        }

        beginResetModel();

        m_node = node;
        initializeGroups();

        endResetModel();
    }

    void NodePaletteModel::initializeGroups()
    {
        m_groups.clear();
        m_groups.reserve(m_categoryNames.size());

        const AZStd::vector<EMotionFX::AnimGraphObject*>& objectPrototypes = m_plugin->GetAnimGraphObjectFactory()->GetUiObjectPrototypes();
        AZStd::vector<AZStd::pair<EMotionFX::AnimGraphObject*, bool>> nodes;
        for (const auto& categoryName : m_categoryNames)
        {
            nodes.clear();
            const EMotionFX::AnimGraphNode::ECategory category = categoryName.first;
            for (EMotionFX::AnimGraphObject* objectPrototype : objectPrototypes)
            {
                if (objectPrototype->GetPaletteCategory() == categoryName.first)
                {
                    const bool isEnabled = m_plugin->CheckIfCanCreateObject(m_node, objectPrototype, category);
                    nodes.emplace_back(objectPrototype, isEnabled);
                }
            }

            if (!nodes.empty())
            {
                auto group = AZStd::make_unique<CategoryGroup>();
                group->m_category = category;
                group->m_nodes = AZStd::move(nodes);
                m_groups.emplace_back(std::move(group));
            }
        }
    }

    NodePaletteWidget::EventHandler::EventHandler(NodePaletteWidget* widget)
        : mWidget(widget)
    {}

    void NodePaletteWidget::EventHandler::OnCreatedNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        if (mWidget->mNode && node->GetParentNode() == mWidget->mNode)
        {
            mWidget->Init(animGraph, mWidget->mNode);
        }
    }


    void NodePaletteWidget::EventHandler::OnRemovedChildNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* parentNode)
    {
        if (mWidget->mNode && parentNode && parentNode == mWidget->mNode)
        {
            mWidget->Init(animGraph, mWidget->mNode);
        }
    }


    // constructor
    NodePaletteWidget::NodePaletteWidget(AnimGraphPlugin* plugin)
        : QWidget()
        , mPlugin(plugin)
        , mModel(new NodePaletteModel(plugin, this))
    {
        mNode   = nullptr;

        // create the default layout
        mLayout = new QVBoxLayout();
        mLayout->setMargin(0);
        mLayout->setSpacing(0);

        // create the initial text
        mInitialText = new QLabel("<c>Create and activate a <b>Anim Graph</b> first.<br>Then <b>drag and drop</b> items from the<br>palette into the <b>Anim Graph window</b>.</c>");
        mInitialText->setAlignment(Qt::AlignCenter);
        mInitialText->setTextFormat(Qt::RichText);
        mInitialText->setMaximumSize(10000, 10000);
        mInitialText->setMargin(0);
        mInitialText->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        // add the initial text in the layout
        mLayout->addWidget(mInitialText);

        // create the tree view
        mTreeView = new QTreeView(this);
        mTreeView->setHeaderHidden(true);
        mTreeView->setModel(mModel);
        mTreeView->setDragDropMode(QAbstractItemView::DragOnly);

        // add the tree view in the layout
        mLayout->addWidget(mTreeView);

        // set the default layout
        setLayout(mLayout);

        // register the event handler
        mEventHandler = aznew NodePaletteWidget::EventHandler(this);
        EMotionFX::GetEventManager().AddEventHandler(mEventHandler);

        connect(&mPlugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &NodePaletteWidget::OnFocusChanged);
    }


    // destructor
    NodePaletteWidget::~NodePaletteWidget()
    {
        EMotionFX::GetEventManager().RemoveEventHandler(mEventHandler);
        delete mEventHandler;
    }


    // init everything for editing a blend tree
    void NodePaletteWidget::Init(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        // set the node
        mNode = node;

        // check if the anim graph is not valid
        // on this case we show a message to say no one anim graph is activated
        if (animGraph == nullptr)
        {
            // set the layout params
            mLayout->setMargin(0);
            mLayout->setSpacing(0);

            // set the widget visible or not
            mInitialText->setVisible(true);
            mTreeView->setVisible(false);
        }
        else
        {
            // set the layout params
            mLayout->setMargin(2);
            mLayout->setSpacing(2);

            // set the widget visible or not
            mInitialText->setVisible(false);
            mTreeView->setVisible(true);
        }

        SaveExpandStates();
        mModel->setNode(mNode);
        RestoreExpandStates();
    }


    void NodePaletteWidget::SaveExpandStates()
    {
        m_expandedCatagory.clear();
        const auto& catagoryNames = mModel->GetCategoryNames();

        // Save the expand state.
        for (const auto& categoryName : catagoryNames)
        {
            const QString& str = categoryName.second;
            const QModelIndexList items = mModel->match(mModel->index(0, 0), Qt::DisplayRole, QVariant::fromValue(str));
            if (!items.isEmpty())
            {
                if (mTreeView->isExpanded(items.first()))
                {
                    m_expandedCatagory.emplace(categoryName.first);
                }
            }
        }
    }


    void NodePaletteWidget::RestoreExpandStates()
    {
        const auto& catagoryNames = mModel->GetCategoryNames();

        // Restore the expand state.
        for (const auto& categoryName : catagoryNames)
        {
            const EMotionFX::AnimGraphNode::ECategory category = categoryName.first;
            if (m_expandedCatagory.find(category) == m_expandedCatagory.end())
            {
                continue;
            }

            const QString& str = categoryName.second;
            const QModelIndexList items = mModel->match(mModel->index(0, 0), Qt::DisplayRole, QVariant::fromValue(str));
            if (!items.isEmpty())
            {
                mTreeView->setExpanded(items.first(), true);
            }
        }
        m_expandedCatagory.clear();
    }


    QIcon NodePaletteWidget::GetNodeIcon(const EMotionFX::AnimGraphNode* node)
    {
        QPixmap pixmap(QSize(12, 8));
        QColor nodeColor;
        nodeColor.setRgbF(node->GetVisualColor().GetR(), node->GetVisualColor().GetG(), node->GetVisualColor().GetB(), 1.0f);
        pixmap.fill(nodeColor);
        QIcon icon(pixmap);
        icon.addPixmap(pixmap, QIcon::Selected);

        // Create a disabled state for the icon.
        QPixmap disabledPixmap(QSize(12, 8));
        QColor disabledNodeColor(51, 51, 51, 255);
        disabledPixmap.fill(disabledNodeColor);
        icon.addPixmap(disabledPixmap, QIcon::Disabled);

        return icon;
    }


    void NodePaletteWidget::OnFocusChanged(const QModelIndex& newFocusIndex, const QModelIndex& newFocusParent, const QModelIndex& oldFocusIndex, const QModelIndex& oldFocusParent)
    {
        AZ_UNUSED(newFocusIndex);
        AZ_UNUSED(oldFocusIndex);

        if (newFocusParent.isValid())
        {
            if (newFocusParent != oldFocusParent)
            {
                EMotionFX::AnimGraphNode* node = newFocusParent.data(AnimGraphModel::ROLE_NODE_POINTER).value<EMotionFX::AnimGraphNode*>();
                Init(node->GetAnimGraph(), node);
            }
        }
        else
        {
            Init(nullptr, nullptr);
        }
    }

} // namespace EMStudio
