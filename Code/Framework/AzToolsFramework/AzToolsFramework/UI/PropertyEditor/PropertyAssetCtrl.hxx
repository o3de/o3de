/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include "PropertyEditorAPI.h"
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzQtComponents/Components/Widgets/BrowseEdit.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/ToolsComponents/EditorAssetReference.h>
#include <AzToolsFramework/AssetBrowser/AssetSelectionModel.h>

AZ_PUSH_DISABLE_WARNING(4244 4251, "-Wunknown-warning-option")
#include <QCompleter>
#include <QMovie>
#include <QStyledItemDelegate>
#include <QLineEdit>
#include <QToolButton>
AZ_POP_DISABLE_WARNING
#endif

class QPushButton;
class QDragEnterEvent;
class QMimeData;

namespace AzToolsFramework
{
    class AssetCompleterModel;
    class AssetCompleterListView;
    class ThumbnailPropertyCtrl;

    namespace Thumbnailer
    {
        class ThumbnailWidget;
    }

    //! Defines a property control for picking base assets.
    //! We can specialize individual asset types (texture) to show previews and such by making specialized handlers, but
    //! at the very least we need a base editor for asset properties in general.
    class PropertyAssetCtrl
        : public QWidget
        , private AssetSystemBus::Handler
        , private AzFramework::AssetCatalogEventBus::Handler
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(PropertyAssetCtrl, AZ::SystemAllocator, 0);

        // This is meant to be used with the "EditCallback" Attribute
        using EditCallbackType = AZ::Edit::AttributeFunction<void(const AZ::Data::AssetId&, const AZ::Data::AssetType&)>;
        using ClearCallbackType = AZ::Edit::AttributeFunction<void()>;
        using DefaultDirectoryCallbackType = AZ::Edit::AttributeFunction<void(AZStd::string&)>;

        PropertyAssetCtrl(QWidget *pParent = NULL, QString optionalValidDragDropExtensions = QString());
        virtual ~PropertyAssetCtrl();

        QWidget* GetFirstInTabOrder() { return m_browseEdit->lineEdit(); }
        QWidget* GetLastInTabOrder() { return m_editButton; }
        void UpdateTabOrder();

        // Resolved asset for this control, this is the user selection with a fallback to the default asset(if exists)
        const AZ::Data::AssetId& GetCurrentAssetID() const { return m_selectedAssetID.IsValid() ? m_selectedAssetID : m_defaultAssetID; }
        const AZ::Data::AssetType& GetCurrentAssetType() const { return m_currentAssetType; }
        const AZStd::string GetCurrentAssetHint() const { return m_currentAssetHint; }

        // User's assetId selection in the UI
        const AZ::Data::AssetId& GetSelectedAssetID() const { return m_selectedAssetID; }

        bool eventFilter(QObject* obj, QEvent* event) override;
        void dragEnterEvent(QDragEnterEvent* event) override;
        void dragLeaveEvent(QDragLeaveEvent* event) override;
        void dropEvent(QDropEvent* event) override;

        virtual AssetSelectionModel GetAssetSelectionModel();

    signals:
        void OnAssetIDChanged(AZ::Data::AssetId newAssetID);

    protected:
        QString m_title;
        ThumbnailPropertyCtrl* m_thumbnail = nullptr;
        QPushButton* m_errorButton = nullptr;
        QToolButton* m_editButton = nullptr;

        AZ::Data::AssetId m_selectedAssetID;
        AZStd::string m_currentAssetHint;

        QCompleter* m_completer = nullptr;
        AssetCompleterModel* m_model = nullptr;
        AssetCompleterListView* m_view = nullptr;

        AZ::Data::AssetId m_defaultAssetID;

        AZ::Data::AssetType m_currentAssetType;

        AzQtComponents::BrowseEdit* m_browseEdit = nullptr;

        AZStd::string m_defaultAssetHint;

        void* m_editNotifyTarget = nullptr;
        EditCallbackType* m_editNotifyCallback = nullptr;
        ClearCallbackType* m_clearNotifyCallback = nullptr;
        QString m_optionalValidDragDropExtensions;
        DefaultDirectoryCallbackType* m_defaultDirectoryCallback = nullptr;

        //! The number of characters after which the autocompleter dropdown will be shown.
        //  Prevents showing too many options.
        static const int s_autocompleteAfterNumberOfChars = 3;

        //! Used to store the last position of the cursor in the lineEdit
        //  so that it can be restored when the text is changed programmatically.
        int m_lineEditLastCursorPosition = 0;

        //! True if the autocompleter has been configured. Prevents multiple configurations.
        bool m_completerIsConfigured = false;

        //! True when the autocompleter dropdown is currently being shown on screen.
        bool m_completerIsActive = false;

        //! This flag is set to true whenever the user alters the value in the lineEdit
        //  At that point, new values can only be selected via the autocompleter or the browse button.
        bool m_incompleteFilename = false;

        //! This flag is set to true when a value is selected from the autocompleter
        //  to avoid the autocompleter to select it again and cancel it.
        bool m_preventAutocomplete = false;

        //! If true, the field is used to reference folders.
        bool m_unnamedType = false;

        //! True if the line edit is on focus (user has clicked on it and is editing it)
        bool m_lineEditFocused = false;

        //! Determines whether the field can be cleared by deleting the value of the lineEdit (or providing an invalid one).
        //  True by default, turns to false if the clear button is disabled or hidden.
        //  If set to false, trying to clear the value of the field will result in the current value being restored.
        bool m_allowEmptyValue = true;

        //! Assets can be either source or product assets generated from source assets. By default, source assets are shown in the property asset. You can override that with this flag.
        bool m_showProductAssetName = true;

        //! Assets can be either source or product assets generated from source assets.
        //! By default the asset picker shows both on an AZ::Asset<> property. You can hide product assets with this flag.
        bool m_hideProductFilesInAssetPicker = false;

        bool m_showThumbnail = false;
        bool m_showThumbnailDropDownButton = false;
        EditCallbackType* m_thumbnailCallback = nullptr;

        // ! Default suffix used in the field's placeholder text when a default value is set.
        const char* m_DefaultSuffix = " (default)";

        bool IsCorrectMimeData(const QMimeData* pData, AZ::Data::AssetId* pAssetId = nullptr, AZ::Data::AssetType* pAssetType = nullptr) const;
        void ClearErrorButton();
        void UpdateErrorButton();
        void UpdateErrorButtonWithLog(const AZStd::string& errorLog);
        void UpdateErrorButtonWithMessage(const AZStd::string& message);
        virtual const AZStd::string GetFolderSelection() const { return AZStd::string(); }
        virtual void SetFolderSelection(const AZStd::string& /* folderPath */) {}
        virtual void ClearAssetInternal();

        virtual void ConfigureAutocompleter();
        void RefreshAutocompleter();
        void EnableAutocompleter();
        void DisableAutocompleter();
        const QModelIndex GetSourceIndex(const QModelIndex& index);

        void HandleFieldClear();
        AZStd::string AddDefaultSuffix(const AZStd::string& filename);

        //////////////////////////////////////////////////////////////////////////
        // AssetSystemBus
        void SourceFileChanged(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
        void SourceFileFailed(AZStd::string relativePath, AZStd::string scanFolder, AZ::Uuid sourceUUID) override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::AssetCatalogEventBus::Handler interface overrides...
        void OnCatalogAssetAdded(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetChanged(const AZ::Data::AssetId& assetId) override;
        void OnCatalogAssetRemoved(const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo) override;
        //////////////////////////////////////////////////////////////////////////

    public slots:
        void SetTitle(const QString& title);
        void SetEditNotifyTarget(void* editNotifyTarget);
        void SetEditNotifyCallback(EditCallbackType* editNotifyCallback); // This is meant to be used with the "EditCallback" Attribute
        void SetClearNotifyCallback(ClearCallbackType* clearNotifyCallback); // This is meant to be used with the "ClearNotify" Attribute
        void SetDefaultDirectoryCallback(DefaultDirectoryCallbackType* callback); // This is meant to be used with the "DefaultStartingDirectoryCallback" Attribute
        void SetEditButtonEnabled(bool enabled);
        void SetEditButtonVisible(bool visible);
        void SetEditButtonIcon(const QIcon& icon);
        void SetEditButtonTooltip(QString tooltip);
        void SetBrowseButtonIcon(const QIcon& icon);
        void SetBrowseButtonEnabled(bool enabled);
        void SetBrowseButtonVisible(bool visible);
        void SetClearButtonEnabled(bool enable);
        void SetClearButtonVisible(bool visible);

        // Otherwise source asset name will shown.
        void SetShowProductAssetName(bool enable);
        bool GetShowProductAssetName() const;

        void SetHideProductFilesInAssetPicker(bool hide);
        bool GetHideProductFilesInAssetPicker() const;

        // Enable and configure a thumbnail widget that displays an asset preview and dropdown arrow for a dropdown menu
        void SetShowThumbnail(bool enable);
        bool GetShowThumbnail() const;
        void SetShowThumbnailDropDownButton(bool enable);
        bool GetShowThumbnailDropDownButton() const;
        void SetThumbnailCallback(EditCallbackType* editNotifyCallback);

        // If enabled, replaces the thumbnail widget content with a custom pixmap
        void SetCustomThumbnailEnabled(bool enabled);
        void SetCustomThumbnailPixmap(const QPixmap& pixmap);

        void SetSelectedAssetID(const AZ::Data::AssetId& newID);
        void SetCurrentAssetType(const AZ::Data::AssetType& newType);
        void SetSelectedAssetID(const AZ::Data::AssetId& newID, const AZ::Data::AssetType& newType);
        void SetCurrentAssetHint(const AZStd::string& hint);
        void SetDefaultAssetID(const AZ::Data::AssetId& defaultID);
        virtual void PopupAssetPicker();
        void OnClearButtonClicked();
        void UpdateAssetDisplay();
        void OnLineEditFocus(bool focus);
        virtual void OnEditButtonClicked();
        void OnThumbnailClicked();
        void OnCompletionModelReset();
        virtual void OnAutocomplete(const QModelIndex& index);
        void OnTextChange(const QString& text);
        void OnReturnPressed();
        void ShowContextMenu(const QPoint& pos);

    private:
        void UpdateThumbnail();
    };

    class AssetPropertyHandlerDefault
        : QObject
        , public PropertyHandler<AZ::Data::Asset<AZ::Data::AssetData>, PropertyAssetCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(AssetPropertyHandlerDefault, AZ::SystemAllocator, 0);

        virtual const AZ::Uuid& GetHandledType() const override;
        virtual AZ::u32 GetHandlerName(void) const override { return AZ_CRC("Asset", 0x02af5a5c); }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyAssetCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        static void ConsumeAttributeInternal(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName);
        void ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    class SimpleAssetPropertyHandlerDefault
        : QObject
        , public PropertyHandler<AzFramework::SimpleAssetReferenceBase, PropertyAssetCtrl>
    {
        // this is a Qt Object purely so it can connect to slots with context.  This is the only reason its in this header.
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(SimpleAssetPropertyHandlerDefault, AZ::SystemAllocator, 0);

        virtual AZ::u32 GetHandlerName(void) const override { return AZ_CRC("SimpleAssetRef", 0x49f51d54); }
        virtual bool IsDefaultHandler() const override { return true; }
        virtual QWidget* GetFirstInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetFirstInTabOrder(); }
        virtual QWidget* GetLastInTabOrder(PropertyAssetCtrl* widget) override { return widget->GetLastInTabOrder(); }
        virtual void UpdateWidgetInternalTabbing(PropertyAssetCtrl* widget) override { widget->UpdateTabOrder(); }

        virtual QWidget* CreateGUI(QWidget* pParent) override;
        virtual void ConsumeAttribute(PropertyAssetCtrl* GUI, AZ::u32 attrib, PropertyAttributeReader* attrValue, const char* debugName) override;
        virtual void WriteGUIValuesIntoProperty(size_t index, PropertyAssetCtrl* GUI, property_t& instance, InstanceDataNode* node) override;
        virtual bool ReadValuesIntoGUI(size_t index, PropertyAssetCtrl* GUI, const property_t& instance, InstanceDataNode* node)  override;
    };

    void RegisterAssetPropertyHandler();
} // namespace AzToolsFramework
