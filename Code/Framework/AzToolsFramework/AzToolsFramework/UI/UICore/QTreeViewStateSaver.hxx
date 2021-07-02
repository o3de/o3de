/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <QObject>
#include <QPointer>
#include <QTreeView>
#endif

class QModelIndex;
class QByteArray;
class QAbstractItemModel;

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    class QTreeViewStateSaverData;
    class QTreeViewStateSaver;

    // Special class to wrap saving and restoring tree state
    // Wouldn't be necessary if there was a way to listen for changes to QTreeView's
    // data model and selection model.
    // Use this instead of QTreeView to handle most of the details of saving and loading
    // tree view state.
    class QTreeViewWithStateSaving
        : public QTreeView
    {
        Q_OBJECT

    public:
        explicit QTreeViewWithStateSaving(QWidget* parent = nullptr);
        ~QTreeViewWithStateSaving() override;

        // One of the Initialize methods must be called before tree view state can be saved or loaded
        void InitializeTreeViewSaving(AZ::u32 storageID);

        void CaptureTreeViewSnapshot() const;
        void ApplyTreeViewSnapshot() const;

        void WriteTreeViewStateTo(QSet<QString>& target);
        void ReadTreeViewStateFrom(QSet<QString>& source);

        void PauseTreeViewSaving();
        void UnpauseTreeViewSaving();

        virtual bool IsIndexExpandedByDefault(const QModelIndex& index) const;

        bool IsTreeViewSavingReady() const { return m_treeStateSaver != nullptr; }

        // These are overridden so that this class knows when the models change
        void setModel(QAbstractItemModel* model) override;
        void setSelectionModel(QItemSelectionModel* selectionModel) override;

        static void Reflect(AZ::ReflectContext* context);

    protected:
        void SetupSaver(QTreeViewStateSaver* stateSaver);

        QTreeViewStateSaver* m_treeStateSaver = nullptr;
    };


    // NOTE: This class should never be used directly anymore. Using it, unless you know what you're doing,
    // can result in weird crashes in destructors because things are deleted in the wrong order.
    // It's being left here for backwards compatibility. If you have a branch building with this class
    // either switch your QTreeView into a QTreeViewWithStateSaving or make QTreeViewStateSaver's main methods public
    // in your branch.
    class QTreeViewStateSaver
        : public QObject
    {
        Q_OBJECT

    public:
        static void Reflect(AZ::ReflectContext* context);
        void ApplySnapshot() const;

    private:

        friend class QTreeViewWithStateSaving;

        explicit QTreeViewStateSaver(AZ::u32 storageID, QObject* pParent = nullptr);
        ~QTreeViewStateSaver() override;

        // The dataModel and the selectionModel have to be initialized before this method is called!!!!
        void Attach(QTreeView* attach, QAbstractItemModel* dataModel, QItemSelectionModel* selectionModel);

        // BEWARE: CaptureSnapshot() must be called manually in your destructor because expandAll() does not emit any signals
        void CaptureSnapshot() const;


        void WriteStateTo(QSet<QString>& target);
        void ReadStateFrom(QSet<QString>& source);

        void Detach();

    private:
        void VerticalScrollChanged(int newValue);
        void HorizontalScrollChanged(int newValue);
        void ModelReset();
        void CurrentChanged(const QModelIndex& current, const QModelIndex&);
        void SelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
        void RowExpanded(const QModelIndex& modelIndex);
        void RowCollapsed(const QModelIndex& modelIndex);

        bool AttachTreeView(QTreeView* treeView);
        bool DetachTreeView();

        bool AttachDataModel(QAbstractItemModel* dataModel);
        bool DetachDataModel();

        bool AttachSelectionModel(QItemSelectionModel* selectionModel);
        bool DetachSelectionModel();

        static bool DefaultExpandedFunction(const QModelIndex& /* idx */) { return false; }

        QTreeView* m_attachedTree = nullptr;
        QPointer<QAbstractItemModel> m_dataModel;
        QPointer<QItemSelectionModel> m_selectionModel;
        AZStd::intrusive_ptr<QTreeViewStateSaverData> m_data;
        bool m_defaultToExpandIndexes = false;

        Q_DISABLE_COPY(QTreeViewStateSaver)
    };

    class TreeViewState
    {
    public:
        static AZStd::unique_ptr<TreeViewState> CreateTreeViewState();

        virtual ~TreeViewState() = default;

        virtual void CaptureSnapshot(QTreeView* treeView) = 0;
        virtual void ApplySnapshot(QTreeView* treeView) = 0;
    };

} //namespace AzToolsFramework
