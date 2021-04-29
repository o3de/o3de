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

#include "EditorDefs.h"

#include "MaterialImageListCtrl.h"

// Qt
#include <QMenu>

// Editor
#include "Material.h"
#include "MaterialManager.h"
#include "MaterialPreviewModelView.h"
#include "MaterialBrowser.h"
#include "Util/Image.h"

#define ME_BG_TEXTURE "Materials/Stripes.dds"


#define MATERIAL_EDITOR_SPHERE_MODEL_FILE "Objects/MtlSphere.cgf"
#define MATERIAL_EDITOR_SPHERE_CAMERA_RADIUS 1.6f
#define MATERIAL_EDITOR_SPHERE_CAMERA_FROM_DIRECTION Vec3(0.1f, -1.0f, -0.1f)

#define MATERIAL_EDITOR_BOX_MODEL_FILE "Objects/MtlBox.cgf"
#define MATERIAL_EDITOR_BOX_CAMERA_RADIUS 2.0f
#define MATERIAL_EDITOR_BOX_CAMERA_FROM_DIRECTION Vec3(0.75f, -0.75f, -0.5f)

#define MATERIAL_EDITOR_TEAPOT_MODEL_FILE "Objects/MtlTeapot.cgf"
#define MATERIAL_EDITOR_TEAPOT_CAMERA_RADIUS 1.6f
#define MATERIAL_EDITOR_TEAPOT_CAMERA_FROM_DIRECTION Vec3(0.1f, -0.75f, -0.25f)

#define MATERIAL_EDITOR_PLANE_MODEL_FILE "Objects/MtlPlane.cgf"
#define MATERIAL_EDITOR_PLANE_CAMERA_RADIUS 1.6f
#define MATERIAL_EDITOR_PLANE_CAMERA_FROM_DIRECTION Vec3(-0.5f, 0.5f, -0.5f)

#define MATERIAL_EDITOR_SWATCH_MODEL_FILE "Objects/MtlSwatch.cgf"
#define MATERIAL_EDITOR_SWATCH_CAMERA_RADIUS 1.0f
#define MATERIAL_EDITOR_SWATCH_CAMERA_FROM_DIRECTION Vec3(0.0f, 0.0f, -1.0f)


_smart_ptr<CMaterial> ResolveTerrainLayerPreviewMaterial(_smart_ptr<CMaterial> material, _smart_ptr<CMaterial> pMatPreview)
{
    if (!QString::compare(material->GetShaderName(), "Terrain.Layer"))
    {
        XmlNodeRef node = XmlHelpers::CreateXmlNode("Material");
        CBaseLibraryItem::SerializeContext ctx(node, false);
        material->Serialize(ctx);

        if (!pMatPreview)
        {
            int flags = 0;
            if (node->getAttr("MtlFlags", flags))
            {
                flags |= MTL_FLAG_UIMATERIAL;
                node->setAttr("MtlFlags", flags);
            }
            pMatPreview = GetIEditor()->GetMaterialManager()->CreateMaterial("_NewPreview_", node);
        }
        else
        {
            CBaseLibraryItem::SerializeContext ctx2(node, true);
            pMatPreview->Serialize(ctx2);
        }
        pMatPreview->SetShaderName("Illum");
        pMatPreview->Update();
        return pMatPreview;
    }
    else
    {
        return material;
    }
}

struct QMaterialImageListModel::Item
{
    QImage image;
    void* pUserData;
    QPoint position;
    QSize size;
    _smart_ptr<CMaterial> pMaterial;
    QStringList vVisibleTextures;
};

//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::OnCreate()
{
    // m_largePreviewCtrl is used to draw the 3D preview for the selected material
    m_largePreviewCtrl.reset(new MaterialPreviewModelView(this));
    m_largePreviewCtrl->hide();
    // m_renderCtrl is used to draw all the sub-materials
    m_renderCtrl.reset(new MaterialPreviewModelView(this, false /* disable idle updates since this is only used to create the preview list images */));
    m_renderCtrl->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID);
    m_renderCtrl->UnSetFlag(CPreviewModelView::PreviewModelViewFlag::SHOW_GRID_AXIS);

    if (gEnv->pSystem)
    {
        gEnv->pSystem->GetISystemEventDispatcher()->RegisterListener(this);
    }

    // m_resizeTimer is used to stall updateGeometries being called via resizeEvents until RESIZE_TIMEOUT ms after resizing
    // This prevents an Editor freeze caused by constant resizing of the Material Editor when viewing
    // a high sub-material count material (LY-58389)
    m_resizeTimer = new QTimer(this);
    connect(m_resizeTimer, &QTimer::timeout, this, &CMaterialImageListCtrl::ResizeTimeout);
}

void CMaterialImageListCtrl::resizeEvent([[maybe_unused]] QResizeEvent* event)
{
    m_resizeTimer->stop();
    m_resizeTimer->start(RESIZE_TIMEOUT);
}

void CMaterialImageListCtrl::ResizeTimeout()
{
    m_resizeTimer->stop();
    updateGeometries();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::OnDestroy()
{
    if (gEnv->pSystem)
    {
        gEnv->pSystem->GetISystemEventDispatcher()->RemoveListener(this);
    }
}

//////////////////////////////////////////////////////////////////////////
CMaterialImageListCtrl::CMaterialImageListCtrl(QWidget* parent)
    : CImageListCtrl(parent)
{
    OnCreate();
}

//////////////////////////////////////////////////////////////////////////
CMaterialImageListCtrl::~CMaterialImageListCtrl()
{
    OnDestroy();
}

//////////////////////////////////////////////////////////////////////////
QModelIndex QMaterialImageListModel::AddMaterial(CMaterial* pMaterial, void* pUserData)
{
    Item* pItem = new Item;
    pItem->pMaterial = pMaterial;
    pItem->pUserData = pUserData;
    pMaterial->GetAnyTextureFilenames(pItem->vVisibleTextures);

    const int row = m_items.count();
    beginInsertRows(QModelIndex(), row, row);
    m_items.insert(row, pItem);
    endInsertRows();

    return index(row, 0);
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::SetMaterial(int nItemIndex, CMaterial* pMaterial, void* pUserData)
{
    assert(nItemIndex <= (int)m_items.size());
    Item* pItem = m_items.at(nItemIndex);

    pItem->vVisibleTextures.clear();
    pItem->pMaterial = pMaterial;
    pItem->pUserData = pUserData;
    pMaterial->GetAnyTextureFilenames(pItem->vVisibleTextures);
    pItem->image = QImage();

    QModelIndex idx = index(nItemIndex, 0);
    emit dataChanged(idx, idx, QVector<int>() << Qt::DisplayRole << Qt::DecorationRole);
}

//////////////////////////////////////////////////////////////////////////
QModelIndex QMaterialImageListModel::FindMaterial(CMaterial* const pMaterial)
{
    for (int i = 0; i < m_items.count(); i++)
    {
        Item* item = m_items[i];
        if (pMaterial == item->pMaterial)
        {
            return index(i, 0);
        }
    }
    return QModelIndex();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::SelectMaterial(CMaterial* pMaterial)
{
    // Force the material to load the highest resolution textures
    pMaterial->GetMatInfo()->DisableTextureStreaming();

    selectionModel()->clearSelection();

    QMaterialImageListModel* materialModel = qobject_cast<QMaterialImageListModel*>(model());
    Q_ASSERT(materialModel);

    QModelIndex index = materialModel->FindMaterial(pMaterial);
    if (index.isValid())
    {
        selectionModel()->select(index, QItemSelectionModel::SelectCurrent);
        m_largePreviewMaterial = pMaterial;
    }
    else
    {
        // If the parent material was selected, set the first sub-material as the large preview's material so it has something to render
        if (pMaterial->GetSubMaterialCount() > 0)
        {
            m_largePreviewMaterial = pMaterial->GetSubMaterial(0);
        }
    }

    GenerateAllImages();
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::InvalidateMaterial(CMaterial* pMaterial)
{
    QModelIndex idx = FindMaterial(pMaterial);
    Item* pItem = ItemFromIndex(idx);
    if ((pItem) && (m_renderCtrl))
    {
        if (pMaterial)
        {
            // Ensure the full resolution textures are loaded for the material editor
            pMaterial->GetMatInfo()->DisableTextureStreaming();
        }

        pItem->vVisibleTextures.clear();
        pMaterial->GetAnyTextureFilenames(pItem->vVisibleTextures);
        pItem->image = QImage();
        GenerateImage(pItem);

        emit dataChanged(idx, idx, QVector<int>() << Qt::DecorationRole);
    }
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::DeleteAllItems()
{
    if (m_renderCtrl)
    {
        m_renderCtrl->SetMaterial(nullptr);
    }
    beginResetModel();
    qDeleteAll(m_items);
    m_items.clear();
    endResetModel();
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::GenerateImage(Item* pItem)
{
    // Make a bitmap from image.
    Q_ASSERT(pItem);
    if (!m_renderCtrl || !pItem->size.isValid())
    {
        return;
    }
    if (pItem->image.size() == pItem->size)
    {
        return;
    }

    Item* pMtlItem = pItem;

    CImageEx image;

    bool bPreview = false;
    if (pMtlItem->pMaterial)
    {
        if (!(pMtlItem->pMaterial->GetFlags() & MTL_FLAG_NOPREVIEW))
        {
            if (!m_renderCtrl->GetStaticModel())
            {
                AZ_Warning("Material Editor", false, "Preview renderer has no object loaded!");
                return;
            }

            // keep m_renderCtrl off screen, but visible
            m_renderCtrl->setGeometry(QRect(-QPoint(pItem->size.width(), pItem->size.height()), pItem->size));

            _smart_ptr<CMaterial> matPreview = ResolveTerrainLayerPreviewMaterial(pItem->pMaterial, m_pMatPreview);

            m_renderCtrl->SetMaterial(matPreview->GetMatInfo());
            m_renderCtrl->GetImageOffscreen(image, pMtlItem->size);
        }
        bPreview = true;
    }

    if (!bPreview)
    {
        image.Allocate(pItem->size.width(), pItem->size.height());
        image.Clear();
    }

    pItem->image = QImage(image.GetWidth(), image.GetHeight(), QImage::Format_RGB32);
    memcpy(pItem->image.bits(), image.GetData(), image.GetSize());
}


#define MENU_USE_DEFAULT 1
#define MENU_USE_BOX 2
#define MENU_USE_PLANE 3
#define MENU_USE_SPHERE 4
#define MENU_USE_TEAPOT 5
#define MENU_BG_BLACK 6
#define MENU_BG_GRAY  7
#define MENU_BG_WHITE 8
#define MENU_BG_TEXTURE 9
#define MENU_USE_BACKLIGHT 10



//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::contextMenuEvent(QContextMenuEvent* event)
{
    QAction* action;
    QMenu menu;

    action = menu.addAction(tr("Use Default Object"));
    action->setData(ModelDefault);
    action->setCheckable(true);
    action->setChecked(m_modelType == ModelType::Default);

    action = menu.addAction(tr("Use Plane"));
    action->setData(ModelPlane);
    action->setCheckable(true);
    action->setChecked(m_modelType == ModelType::Plane);

    action = menu.addAction(tr("Use Box"));
    action->setData(ModelBox);
    action->setCheckable(true);
    action->setChecked(m_modelType == ModelType::Box);

    action = menu.addAction(tr("Use Sphere"));
    action->setData(ModelSphere);
    action->setCheckable(true);
    action->setChecked(m_modelType == ModelType::Sphere);

    action = menu.addAction(tr("Use Teapot"));
    action->setData(ModelTeapot);
    action->setCheckable(true);
    action->setChecked(m_modelType == ModelType::Teapot);

    menu.addSeparator();

    // If there is a currently selected material
    if (m_materialBrowserWidget && m_largePreviewMaterial)
    {
        // Add context menu actions that are common to both the material browser and the preview swatches
        m_materialBrowserWidget->AddContextMenuActionsSingleSelection(menu, m_largePreviewMaterial);
    }

    action = menu.exec(mapToGlobal(event->pos()));
    if (!action)
    {
        return;
    }

    int cmd = action->data().toInt();
    switch (cmd)
    {
    case ModelDefault:
        m_modelType = ModelType::Default;
        LoadModel();
        break;
    case ModelPlane:
        m_modelType = ModelType::Plane;
        LoadModel();
        break;
    case ModelBox:
        m_modelType = ModelType::Box;
        LoadModel();
        break;
    case ModelSphere:
        m_modelType = ModelType::Sphere;
        LoadModel();
        break;
    case ModelTeapot:
        m_modelType = ModelType::Teapot;
        LoadModel();
        break;
    default:
        // If there is a currently selected material
        if (m_materialBrowserWidget && m_largePreviewMaterial)
        {
            // Handle context menu actions that are common to both the material browser and the preview swatches
            m_materialBrowserWidget->OnContextMenuAction(cmd, m_largePreviewMaterial);
        }
        break;
    }

    QMaterialImageListModel* materialModel =
        qobject_cast<QMaterialImageListModel*>(model());
    Q_ASSERT(materialModel);

    materialModel->GenerateImages();
    update();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::LoadModel()
{
    switch (m_modelType)
    {
    case ModelType::Default:
    case ModelType::Sphere:
        m_largePreviewCtrl->LoadModelFile(MATERIAL_EDITOR_SPHERE_MODEL_FILE);
        m_largePreviewCtrl->SetCameraLookAt(MATERIAL_EDITOR_SPHERE_CAMERA_RADIUS, MATERIAL_EDITOR_SPHERE_CAMERA_FROM_DIRECTION);
        break;
    case ModelType::Box:
        m_largePreviewCtrl->LoadModelFile(MATERIAL_EDITOR_BOX_MODEL_FILE);
        m_largePreviewCtrl->SetCameraLookAt(MATERIAL_EDITOR_BOX_CAMERA_RADIUS, MATERIAL_EDITOR_BOX_CAMERA_FROM_DIRECTION);
        break;
    case ModelType::Teapot:
        m_largePreviewCtrl->LoadModelFile(MATERIAL_EDITOR_TEAPOT_MODEL_FILE);
        m_largePreviewCtrl->SetCameraLookAt(MATERIAL_EDITOR_TEAPOT_CAMERA_RADIUS, MATERIAL_EDITOR_TEAPOT_CAMERA_FROM_DIRECTION);
        break;
    case ModelType::Plane:
        m_largePreviewCtrl->LoadModelFile(MATERIAL_EDITOR_PLANE_MODEL_FILE);
        m_largePreviewCtrl->SetCameraLookAt(MATERIAL_EDITOR_PLANE_CAMERA_RADIUS, MATERIAL_EDITOR_PLANE_CAMERA_FROM_DIRECTION);
        break;
    }

    GenerateAllImages();
}


//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::updateGeometries()
{
    ClearItemGeometries();
    m_updatingGeometries = true;

    if (!model())
    {
        return;
    }

    const int rowCount = model()->rowCount();
    if (!rowCount)
    {
        m_largePreviewCtrl->hide();
        m_largePreviewMaterial = nullptr;
        return;
    }
    m_largePreviewCtrl->setParent(nullptr);
    m_largePreviewCtrl->setParent(this);
    m_largePreviewCtrl->show();

    const int bwidth = BorderSize().width();
    const int bheight = BorderSize().height();
    m_largePreviewCtrl->move(bwidth, bheight);

    QRect rc = viewport()->contentsRect()
            .adjusted(bwidth, bheight, -bwidth, -bheight);

    int cy = rc.height();

    // Preview item is big.
    if (model()->rowCount() > 0)
    {
        QSize size(cy, rc.height());
        m_largePreviewCtrl->SetSize(size);
        m_largePreviewCtrl->resize(size);
        rc.setLeft(rc.left() + cy + bwidth * 2);
        m_largePreviewCtrl->show();
    }

    int cx = rc.width() - bwidth;

    int itemSize = cy;

    // Adjust all other bitmaps as tight as possible.
    int numItems = model()->rowCount();
    int div = 0;
    for (div = 1; div < 1000 && itemSize > 0; div++)
    {
        int nX = cx / (itemSize + 2);
        if (nX >= numItems)
        {
            break;
        }
        if (nX > 0)
        {
            int nY = numItems / nX + 1;
            if (nY * (itemSize + 2) < cy)
            {
                //itemSize = itemSize -= 2;
                break;
            }
        }
        itemSize = itemSize -= 2;
    }
    if (itemSize < 0)
    {
        itemSize = 0;
    }

    QPoint pos(rc.topLeft());
    const QSize size(itemSize, itemSize);
    m_renderCtrl->SetSize(size);
    for (int row = 0; row < numItems; ++row)
    {
        QModelIndex index = model()->index(row, 0);
        SetItemGeometry(index, QRect(pos, size));
        model()->setData(index, pos, QMaterialImageListModel::PositionRole);
        model()->setData(index, size, Qt::SizeHintRole);
        pos.rx() += itemSize + 2;
        if (pos.rx() + itemSize >= rc.right())
        {
            pos.rx() = rc.left();
            pos.ry() += itemSize + 2;
        }
    }

    m_updatingGeometries = false;
    update();
}

//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::setModel(QAbstractItemModel* model)
{
    QMaterialImageListModel* materialModel = qobject_cast<QMaterialImageListModel*>(model);
    if (materialModel)
    {
        materialModel->SetPreviewModelCtrl(m_renderCtrl.data());
    }

    CImageListCtrl::setModel(model);

    auto delegate = qobject_cast<QImageListDelegate*>(itemDelegate());
    // in case the delegate misses a pixmap we can generate it (done only once not at every painting)
    connect(delegate, SIGNAL(invalidPixmapGenerated(const QModelIndex&)), materialModel, SLOT(GenerateImage(const QModelIndex&)));
    connect(model, &QAbstractItemModel::dataChanged, this, &CMaterialImageListCtrl::ModelDataChanged);
}

void CMaterialImageListCtrl::ModelDataChanged(const QModelIndex& index)
{
    // Prevent the hundreads of resize calls done in a row
    // to trigger a new image computation that we already have
    if (m_updatingGeometries)
    {
        return;
    }

    GenerateImage(index);
}

//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::showEvent(QShowEvent* event)
{
    CImageListCtrl::showEvent(event);

    if (!m_largePreviewCtrl->GetStaticModel())
    {
        LoadModel();
    }
    if (!m_renderCtrl->GetStaticModel())
    {
        m_renderCtrl->LoadModelFile(MATERIAL_EDITOR_SWATCH_MODEL_FILE);
        m_renderCtrl->SetCameraLookAt(MATERIAL_EDITOR_SWATCH_CAMERA_RADIUS, MATERIAL_EDITOR_SWATCH_CAMERA_FROM_DIRECTION);
    }
}

//////////////////////////////////////////////////////////////////////////
void CMaterialImageListCtrl::OnSystemEvent(ESystemEvent event, UINT_PTR wparam, [[maybe_unused]] UINT_PTR lparam)
{
    switch (event)
    {
    // Toggle visibility of this control whenever the main editor window has a change of focus.
    case ESYSTEM_EVENT_CHANGE_FOCUS:
    {
        setAttribute(Qt::WA_WState_Visible, (wparam != 0));
        break;
    }
    }
    ;
}

void CMaterialImageListCtrl::UpdateLargePreview()
{
    if (m_largePreviewMaterial)
    {
        m_largePreviewCtrl->SetMaterial(ResolveTerrainLayerPreviewMaterial(m_largePreviewMaterial, m_tempTerrainMaterial)->GetMatInfo());
        m_largePreviewCtrl->show();
        m_largePreviewCtrl->update();
    }
}

void CMaterialImageListCtrl::GenerateImage(const QModelIndex& index)
{
    QMaterialImageListModel* materialModel =
        qobject_cast<QMaterialImageListModel*>(model());
    Q_ASSERT(materialModel);
    if (m_largePreviewMaterial == materialModel->MaterialFromIndex(index))
    {
        UpdateLargePreview();
    }

    materialModel->SetPreviewModelCtrl(m_renderCtrl.data());
    materialModel->GenerateImage(index);
    update();
}

void CMaterialImageListCtrl::GenerateAllImages()
{
    QMaterialImageListModel* materialModel =
        qobject_cast<QMaterialImageListModel*>(model());
    Q_ASSERT(materialModel);

    UpdateLargePreview();

    materialModel->SetPreviewModelCtrl(m_renderCtrl.data());
    materialModel->GenerateImages();
    update();
}

//////////////////////////////////////////////////////////////////////////
QMaterialImageListModel::QMaterialImageListModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_renderCtrl(nullptr)
{
    BusConnect(AZ_CRC("dds", 0x780234cb));
}

//////////////////////////////////////////////////////////////////////////
QMaterialImageListModel::~QMaterialImageListModel()
{
    BusDisconnect();

    qDeleteAll(m_items);
}

//////////////////////////////////////////////////////////////////////////
MaterialPreviewModelView* QMaterialImageListModel::PreviewModelCtrl() const
{
    return m_renderCtrl;
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::SetPreviewModelCtrl(MaterialPreviewModelView* ctrl)
{
    if (m_renderCtrl)
    {
        disconnect(m_renderCtrl, &MaterialPreviewModelView::destroyed,
            this, &QMaterialImageListModel::ClearPreviewModelCtrl);
    }

    m_renderCtrl = ctrl;

    if (m_renderCtrl)
    {
        connect(m_renderCtrl, &MaterialPreviewModelView::destroyed,
            this, &QMaterialImageListModel::ClearPreviewModelCtrl);
    }
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::ClearPreviewModelCtrl()
{
    m_renderCtrl = nullptr;
}

//////////////////////////////////////////////////////////////////////////
int QMaterialImageListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid())
    {
        return 0;
    }

    return m_items.count();
}

//////////////////////////////////////////////////////////////////////////
QVariant QMaterialImageListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    Item* pItem = m_items.at(index.row());
    Q_ASSERT(pItem);

    switch (role)
    {
    case Qt::DisplayRole:
        return pItem->pMaterial->GetShortName();

    case Qt::DecorationRole:
        return QPixmap::fromImage(pItem->image);

    case PositionRole:
        return pItem->position;

    case Qt::SizeHintRole:
        return pItem->size;

    default:
        break;
    }

    return QVariant();
}

//////////////////////////////////////////////////////////////////////////
bool QMaterialImageListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid())
    {
        return false;
    }

    Item* pItem = ItemFromIndex(index);
    if (!pItem)
    {
        return false;
    }

    if (Qt::SizeHintRole == role)
    {
        pItem->size = value.toSize();
        emit dataChanged(index, index, QVector<int>() << Qt::DecorationRole << Qt::SizeHintRole);
        return true;
    }

    if (PositionRole == role)
    {
        pItem->position = value.toPoint();
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
Qt::ItemFlags QMaterialImageListModel::flags(const QModelIndex& index) const
{
    return QAbstractListModel::flags(index);
}

//////////////////////////////////////////////////////////////////////////
CMaterial* QMaterialImageListModel::MaterialFromIndex(QModelIndex index) const
{
    Item* pItem = ItemFromIndex(index);
    if (!pItem)
    {
        return nullptr;
    }

    return pItem->pMaterial;
}

//////////////////////////////////////////////////////////////////////////
void* QMaterialImageListModel::UserDataFromIndex(QModelIndex index) const
{
    Item* pItem = ItemFromIndex(index);
    if (!pItem)
    {
        return nullptr;
    }

    return pItem->pUserData;
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::GenerateImages()
{
    if ((!m_renderCtrl) || (m_items.isEmpty()))
    {
        return;
    }

    for (Item* materialImageListItem : m_items)
    {
        GenerateImage(materialImageListItem);
    }
}

void QMaterialImageListModel::GenerateImage(const QModelIndex& index)
{
    if ((!m_renderCtrl) || (m_items.isEmpty()))
    {
        return;
    }

    GenerateImage(ItemFromIndex(index));
}

//////////////////////////////////////////////////////////////////////////
QMaterialImageListModel::Item* QMaterialImageListModel::ItemFromIndex(QModelIndex index) const
{
    if (!index.isValid())
    {
        return nullptr;
    }

    return m_items.at(index.row());
}

//////////////////////////////////////////////////////////////////////////
void QMaterialImageListModel::OnFileChanged(AZStd::string assetPath)
{
    const int rowCount = m_items.count();

    // update all previews who's texture(s) changed
    for (int row = 0; row < rowCount; ++row)
    {
        Item* pItem = m_items.at(row);

        QStringList::const_iterator textureIt = std::find(pItem->vVisibleTextures.begin(), pItem->vVisibleTextures.end(), assetPath.c_str());
        if (textureIt != pItem->vVisibleTextures.end())
        {
            pItem->image = QImage();
            QModelIndex idx = index(row, 0);
            emit dataChanged(idx, idx, QVector<int>() << Qt::DecorationRole);
        }
    }
}

#include <Material/moc_MaterialImageListCtrl.cpp>
