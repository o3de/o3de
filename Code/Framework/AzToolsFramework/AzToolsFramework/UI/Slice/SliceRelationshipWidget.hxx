/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QtWidgets/QWidget>
#include <AzToolsFramework/Slice/SliceDependencyBrowserBus.h>
#include <AzToolsFramework/UI/Slice/SliceRelationshipBus.h>
#endif

class QTreeWidget;
class QTreeWidgetItem;
class QLabel;
class QVBoxLayout;

namespace AzToolsFramework
{
    /**
     * Widget that displays relationships between slices
     */
    class SliceRelationshipWidget
        : public QWidget
        , private SliceDependencyBrowserNotificationsBus::Handler
        , private SliceRelationshipRequestBus::Handler
    {
        Q_OBJECT

    public:

        explicit SliceRelationshipWidget(QWidget* parent = 0);
        ~SliceRelationshipWidget();

        //////////////////////////////////////////////////////////////////////////
        // SliceDependencyBrowserNotificationsBus::Handler
        void OnSliceRelationshipModelUpdated(const AZStd::shared_ptr<SliceRelationshipNode>& focusNode) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SliceRelationshipRequestBus::Handler
        void OnSliceRelationshipViewRequested(const AZ::Data::AssetId& assetId) override;
        //////////////////////////////////////////////////////////////////////////

        void closeEvent(QCloseEvent* event) override;

    public Q_SLOTS:

        void OnBrowseClicked();

        void OnInspectSelectedEntity();

        void OnSliceAssetDrilledDown(QTreeWidgetItem* item, int column);

    private:

        /**
        * \brief Generates both relationship trees for an indicated node
        * \param const AZStd::shared_ptr<SliceRelationshipNode> & node The node being inspected in the widget
        * \return void
        */
        void GenerateTreesForNode(const AZStd::shared_ptr<SliceRelationshipNode>& node);

        /**
        * \brief Recursively generates the dependents tree for an indicated node
        * \param const AZStd::shared_ptr<SliceRelationshipNode> & node Node whose dependents are to be added to the parentItem
        * \param QTreeWidgetItem * parentItem item under which all dependents of 'node' should be shown
        * \return void
        */
        void GenerateDependentsTree(const AZStd::shared_ptr<SliceRelationshipNode>& node, QTreeWidgetItem* parentItem);

        /**
        * \brief Recursively generates the dependency tree for an indicated node
        * \param const AZStd::shared_ptr<SliceRelationshipNode> & node Node whose dependencies are to be added to the parentItem
        * \param QTreeWidgetItem * parentItem item under which all dependencies of 'node' should be shown
        * \return void
        */
        void GenerateDependenciesTree(const AZStd::shared_ptr<SliceRelationshipNode>& node, QTreeWidgetItem* parentItem);

        /**
        * \brief Reset the widget to show relationships for the slice on the indicated entity
        * \param const EntityIdList & entity Entity whose slice is to be inspected
        * \return void
        */
        void ResetTrees(const AZ::EntityId& entity);

        /**
        * \brief Reset the widget to show relationships for a slice at a given asset id
        * \param const AZ::Data::AssetId & assetId asset Id of the slice that the widget should be inspecting
        * \return void
        */
        void ResetTrees(const AZ::Data::AssetId& assetId);
        
        /**
        * \brief Reset the widget to show relationships for a slice at a given relative path
        * \param const AZStd::string & assetRelativePath relative Path of the slice that the widget should be inspecting
        * \return void 
        */
        void ResetTrees(const AZStd::string& assetRelativePath);

        bool CheckForCycle(QTreeWidgetItem* parentItem, QString searchFor);

        QTreeWidget*                                                m_sliceDependentsTree;            ///< Tree widget for fields (left side)
        QTreeWidget*                                                m_sliceDependencyTree;            ///< Tree widget for slice targets (right side)
        QLabel*                                                     m_infoLabel;            ///< Label above slice tree describing selection
        QVBoxLayout*                                                m_bottomLayout;         ///< Bottom layout containing optional status messages, legend and buttons
    };

} // namespace AzToolsFramework
