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

#include "ContextViewer.h"
#include <Editor/Include/ScriptCanvasDeveloperEditor/Tools/ContextViewer/ui_ContextViewer.h>

#include <aws/lambda/model/ListFunctionsRequest.h>
#include <aws/lambda/model/ListFunctionsResult.h>
#include <aws/lambda/LambdaClient.h>
#include <aws/core/utils/Outcome.h>
#include <GraphCanvas/Widgets/NodePalette/NodePaletteWidget.h>
#include <GraphCanvas/Widgets/NodePalette/TreeItems/NodePaletteTreeItem.h>

namespace ScriptCanvasEditor
{
    static const GraphCanvas::EditorId CONTEXT_VIEWER_EDITOR_ID = AZ_CRC_CE("ContextViewer");

    ContextViewer::ContextViewer(QWidget* widget /*= nullptr*/)
        : AzQtComponents::StyledDockWidget(widget)
        , m_ui(new Ui::ContextViewer())
    {
        setWindowFlags(Qt::WindowFlags::enum_type::WindowCloseButtonHint);

        m_ui->setupUi(this);

        Aws::Client::ClientConfiguration clientConfig;
//        m_client = Aws::MakeShared<Aws::Lambda::LambdaClient>("ContextViewer", clientConfig);

        // How to login to AWS so I have access to these things?

        ListFunctions();


        GraphCanvas::NodePaletteConfig nodePaletteConfig;


        GraphCanvas::NodePaletteTreeItem* rootItem = aznew GraphCanvas::NodePaletteTreeItem("Root", CONTEXT_VIEWER_EDITOR_ID);

        // Setup our default node palette
        //m_nodePalette = AZStd::make_unique(m_config->CreateNodePaletteRoot(), editorId, QObject::tr("Node Palette"), this, m_config->m_mimeType, false, m_config->m_saveIdentifier);

        /// NodePaletteDockWidget(GraphCanvasTreeItem* treeItem, const EditorId& editorId, const QString& windowLabel, QWidget* parent, const char* mimeType, bool inContextMenu, AZStd::string_view identifier);
        m_nodePalette = AZStd::make_unique<GraphCanvas::NodePaletteDockWidget>(rootItem, CONTEXT_VIEWER_EDITOR_ID, QStringLiteral("Context Viewer"), this, "ContextViewer", false, "ContextViewer");
        m_nodePalette->setObjectName("NodePalette");
        m_nodePalette->setWindowTitle("Context Viewer");

        GraphCanvas::NodePaletteTreeItem* category = rootItem->CreateChildNode<GraphCanvas::NodePaletteTreeItem>("Lambda", CONTEXT_VIEWER_EDITOR_ID);
        GraphCanvas::NodePaletteTreeItem* test = category->CreateChildNode<GraphCanvas::NodePaletteTreeItem>("Functions", CONTEXT_VIEWER_EDITOR_ID);

        //m_client->ListTags()

        //Aws::Lambda::Model::ListFunctionsRequest listFunctionsRequest;
        //auto listFunctionsOutcome = m_client->ListFunctions(listFunctionsRequest);
        //if (listFunctionsOutcome.IsSuccess())
        //{
        //    auto functions = listFunctionsOutcome.GetResult().GetFunctions();
        //    
        //    for (const auto& item : functions)
        //    {
        //        GraphCanvas::NodePaletteTreeItem* lambdaFunction = test->CreateChildNode<GraphCanvas::NodePaletteTreeItem>(item.GetFunctionName().c_str(), AWS_LAMBDA_EDITOR_ID);
        //        lambdaFunction->SetToolTip(item.GetDescription().c_str());
        //    }
        //}
    }


    void ContextViewer::ListFunctions()
    {
        //Aws::Lambda::Model::ListFunctionsRequest listFunctionsRequest;
        //auto listFunctionsOutcome = m_client->ListFunctions(listFunctionsRequest);
        //auto functions = listFunctionsOutcome.GetResult().GetFunctions();
        //std::cout << functions.size() << " function(s):" << std::endl;
        //for (const auto& item : functions)
        //    std::cout << item.GetFunctionName() << std::endl;
        //std::cout << std::endl;
    }

#include <Editor/Include/ScriptCanvasDeveloperEditor/Tools/ContextViewer/ContextViewer.moc>
}
