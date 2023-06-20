/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/BlendGraphWidget.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>
#include <AzQtComponents/Utilities/Conversions.h>
#include "AnimGraphPlugin.h"
#include <AzQtComponents/Components/StyleManager.h>
#include <QVBoxLayout>
#include <QIcon>
#include <QAction>
#include <QMimeData>
#include <QLabel>
#include <QTextEdit>
#include <QTreeView>


namespace EMStudio
{
    AZ_CLASS_ALLOCATOR_IMPL(NodePaletteWidget::EventHandler, EMotionFX::EventHandlerAllocator)

    NodePaletteWidget::EventHandler::EventHandler(NodePaletteWidget* widget)
        : m_widget(widget)
    {}

    void NodePaletteWidget::EventHandler::OnCreatedNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        if (m_widget->m_node && node->GetParentNode() == m_widget->m_node)
        {
            m_widget->Init(animGraph, m_widget->m_node);
        }
    }


    void NodePaletteWidget::EventHandler::OnRemovedChildNode(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* parentNode)
    {
        if (m_widget->m_node && parentNode && parentNode == m_widget->m_node)
        {
            m_widget->Init(animGraph, m_widget->m_node);
        }
    }


    // constructor
    NodePaletteWidget::NodePaletteWidget(AnimGraphPlugin* plugin)
        : QWidget()
        , m_plugin(plugin)
        , m_modelUpdater(plugin)
    {
        m_node   = nullptr;

        // create the default layout
        m_layout = new QVBoxLayout();
        m_layout->setMargin(0);
        m_layout->setSpacing(0);

        // create the initial text
        m_initialText = new QLabel("<c>Create and activate a <b>Anim Graph</b> first.<br>Then <b>drag and drop</b> items from the<br>palette into the <b>Anim Graph window</b>.</c>");
        m_initialText->setAlignment(Qt::AlignCenter);
        m_initialText->setTextFormat(Qt::RichText);
        m_initialText->setMaximumSize(10000, 10000);
        m_initialText->setMargin(0);
        m_initialText->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

        // add the initial text in the layout
        m_layout->addWidget(m_initialText);

        GraphCanvas::NodePaletteConfig config;
        config.m_editorId = AnimGraphEditorId;
        config.m_rootTreeItem = m_modelUpdater.GetRootItem();
        config.m_isInContextMenu = false;
        config.m_mimeType = BlendGraphMimeEvent::BlendGraphMimeEventType;
        m_palette = new GraphCanvas::NodePaletteWidget(this);
        m_palette->SetupNodePalette(config);
        m_palette->hide();
        m_layout->addWidget(m_palette);
        // GHI-13382 Investigate why we need to apply the style here even though it's applied globally to the editor
        AzQtComponents::StyleManager::setStyleSheet(m_palette, QStringLiteral("style:Editor.qss"));

        // set the default layout
        setLayout(m_layout);

        // register the event handler
        m_eventHandler = aznew NodePaletteWidget::EventHandler(this);
        EMotionFX::GetEventManager().AddEventHandler(m_eventHandler);

        connect(&m_plugin->GetAnimGraphModel(), &AnimGraphModel::FocusChanged, this, &NodePaletteWidget::OnFocusChanged);
    }


    // destructor
    NodePaletteWidget::~NodePaletteWidget()
    {
        EMotionFX::GetEventManager().RemoveEventHandler(m_eventHandler);
        delete m_eventHandler;
    }

    // init everything for editing a blend tree
    void NodePaletteWidget::Init(EMotionFX::AnimGraph* animGraph, EMotionFX::AnimGraphNode* node)
    {
        // set the node
        m_node = node;

        // check if the anim graph is not valid
        // on this case we show a message to say no one anim graph is activated
        if (animGraph == nullptr)
        {
            // set the layout params
            m_layout->setMargin(0);
            m_layout->setSpacing(0);

            // set the widget visible or not
            m_initialText->show();
            m_palette->hide();
        }
        else
        {
            // set the layout params
            m_layout->setMargin(2);
            m_layout->setSpacing(2);

            // set the widget visible or not
            m_initialText->hide();
            m_palette->show();
        }
        m_modelUpdater.InitForNode(node);
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
