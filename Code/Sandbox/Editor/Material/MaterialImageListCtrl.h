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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_EDITOR_MATERIAL_MATERIALIMAGELISTCTRL_H
#define CRYINCLUDE_EDITOR_MATERIAL_MATERIALIMAGELISTCTRL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include "Controls/ImageListCtrl.h"
#include <QAbstractListModel>
#include "Material.h"
#include "IRenderer.h" // IAsyncTextureCompileListener
#include "IResourceCompilerHelper.h"
#include <AzFramework/Asset/AssetCatalogBus.h>

#include <QScopedPointer>
#include <QPointer>
#endif

class MaterialPreviewModelView;
class MaterialBrowserWidget;

//////////////////////////////////////////////////////////////////////////
class CMaterialImageListCtrl
    : public CImageListCtrl
    , public ISystemEventListener
{
    Q_OBJECT
public:

    enum class ModelType
    {
        Default,
        Box,
        Sphere,
        Teapot,
        Plane
    };

    enum MenuAction
    {
        ModelDefault = 0,
        ModelPlane,
        ModelBox,
        ModelSphere,
        ModelTeapot,
        // Context menu actions that are common to both the material browser and the preview swatches
        // are handled by the same switch statement, so they need to be unique. This sets the starting point for the common actions
        MaterialBrowserWidgetActionsStart
    };

    CMaterialImageListCtrl(QWidget* parent = nullptr);
    ~CMaterialImageListCtrl();

    void setModel(QAbstractItemModel* model) override;

    void EnableAutoRefresh(bool autoRefreshState, unsigned int refreshInterval);
    void SelectMaterial(CMaterial* pMaterial);
    void LoadModel();
    void SetMaterialBrowserWidget(MaterialBrowserWidget* materialBrowserWidget){ m_materialBrowserWidget = materialBrowserWidget; }
    // ISystemEventListener
    // Due to the material editor working on a ProcessEvents -> Timer based system, rather than the OnIdle update event loops
    // that the other editor windows use, make sure that when the editor loses focus that the Material Editor itself loses focus.
    // This will pause updates/renderings in the material editor when it does not have focus.
    // This prevents certain materials from re-creating themselves and eventually overflowing a few resource buffers.
    // The main window cleans up those resources during its main update, which is bypassed when the window does not have focus.
    virtual void OnSystemEvent(ESystemEvent event, UINT_PTR wparam, UINT_PTR lparam);
    // ~ISystemEventListener

    // Override resizeEvent and use this const to rate limit it such that it only fires RESIZE_TIMEOUT ms after resizing stops
    static const int RESIZE_TIMEOUT = 100;
    void resizeEvent(QResizeEvent* event) override;

public slots:
    void ModelDataChanged(const QModelIndex& index);
    void ResizeTimeout();
protected:
    void OnCreate();
    void OnDestroy();

    void contextMenuEvent(QContextMenuEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void updateGeometries() override;

private:
    void GenerateImage(const QModelIndex& index);
    void GenerateAllImages();
    void UpdateLargePreview();

    QScopedPointer<MaterialPreviewModelView> m_largePreviewCtrl; // Used to draw the main 3D preview viewport for the selected sub-material
    _smart_ptr<CMaterial> m_largePreviewMaterial;
    _smart_ptr<CMaterial> m_tempTerrainMaterial;
    QScopedPointer<MaterialPreviewModelView> m_renderCtrl; // Used to draw the swatches for all the sub-materials
    int m_nColor;
    bool m_updatingGeometries = false;
    ModelType m_modelType = ModelType::Default;
    MaterialBrowserWidget* m_materialBrowserWidget;
    QTimer* m_resizeTimer; // Used to stall a resizeEvent from firing until RESIZE_TIMEOUT ms have passed since resizing stopped
};

class QMaterialImageListModel
    : public QAbstractListModel
    , public AzFramework::LegacyAssetEventBus::Handler
{
    struct Item;
    Q_OBJECT

public:
    enum CustomRoles
    {
        PositionRole = Qt::UserRole
    };

    QMaterialImageListModel(QObject* parent = nullptr);
    ~QMaterialImageListModel();

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    QModelIndex AddMaterial(CMaterial* pMaterial, void* pUserData = nullptr);
    void SetMaterial(int nItemIndex, CMaterial* pMaterial, void* pUserData = nullptr);
    QModelIndex FindMaterial(CMaterial* pMaterial);
    void InvalidateMaterial(CMaterial* pMaterial);
    void DeleteAllItems();

    MaterialPreviewModelView* PreviewModelCtrl() const;
    void SetPreviewModelCtrl(MaterialPreviewModelView* ctrl);

    CMaterial* MaterialFromIndex(QModelIndex index) const;
    void* UserDataFromIndex(QModelIndex index) const;

public slots:
    void GenerateImages();
    void GenerateImage(const QModelIndex& index);

protected:
    void GenerateImage(Item* pItem);
    Item* ItemFromIndex(QModelIndex index) const;

private slots:
    void ClearPreviewModelCtrl();

private:
    void OnFileChanged(AZStd::string assetPath) override;

private:
    _smart_ptr<CMaterial> m_pMatPreview;
    QPointer<MaterialPreviewModelView> m_renderCtrl;
    QVector<Item*> m_items;
};

#endif // CRYINCLUDE_EDITOR_MATERIAL_MATERIALIMAGELISTCTRL_H
