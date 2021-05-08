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

#include "CubemapUtils.h"

// Qt
#include <QAbstractListModel>
#include <QComboBox>
#include <QDialog>
#include <QLabel>
#include <QDialogButtonBox>
#include <QVBoxLayout>

// AzToolsFramework
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

// Editor
#include "Util/ImageTIF.h"
#include "Objects/BaseObject.h"

#include <IEntityRenderState.h>

class CubemapSizeModel
    : public QAbstractListModel
{
public:
    CubemapSizeModel(QObject* parent = nullptr)
        : QAbstractListModel(parent)
    { }

    int rowCount(const QModelIndex& parent = {}) const override
    {
        return parent.isValid() ? 0 : kNumResolutions;
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || index.row() >= kNumResolutions)
        {
            return {};
        }

        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::UserRole:
            return 32 << index.row();
        }

        return {};
    }

private:
    static const int kNumResolutions = 6;
};

class CubemapSizeDialog
    : public QDialog
{
public:
    CubemapSizeDialog(QWidget* parent = nullptr)
        : QDialog(parent)
        , m_model(new CubemapSizeModel(this))
    {
        setWindowTitle(tr("Enter Cubemap Resolution"));

        m_comboBox = new QComboBox;
        m_comboBox->setModel(m_model);
        m_comboBox->setCurrentIndex(3);

        auto horLine = new QLabel;
        horLine->setFrameShape(QFrame::HLine);
        horLine->setFrameShadow(QFrame::Sunken);

        auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

        auto layout = new QVBoxLayout;
        layout->addWidget(m_comboBox);
        layout->addWidget(horLine);
        layout->addWidget(buttonBox);

        setLayout(layout);
    }

    int GetValue() const
    {
        return m_comboBox->currentData().toInt();
    }

private:
    CubemapSizeModel* m_model;
    QComboBox* m_comboBox;
};

///////////////////////////////////////////////////////////////////////////////////
bool CubemapUtils::GenCubemapWithObjectPathAndSize(QString& filename, CBaseObject* pObject, const int size, const bool hideObject)
{
    if (!pObject)
    {
        Warning("Select One Entity to Generate Cubemap");
        return false;
    }

    if (pObject->GetType() != OBJTYPE_AZENTITY)
    {
        Warning("Only Entities are allowed as a selected object. Please Select Entity objects");
        return false;
    }

    int res = 1;
    // Make size power of 2.
    for (int i = 0; i < 16; i++)
    {
        if (res * 2 > size)
        {
            break;
        }
        res *= 2;
    }
    if (res > 4096)
    {
        Warning("Bad texture resolution.\nMust be power of 2 and less or equal to 4096");
        return false;
    }

    IRenderNode* pRenderNode = pObject->GetEngineNode();

    // Hide the object before Cubemap generation (maybe). This is useful for when generating a cubemap at an entity's position, like the player,
    // and you don't want their model showing up in the cubemap. But you want to leave the entity alone if it's a light or something that
    // has a desired contribution to the cubemap.
    bool bIsHidden = false;
    if (pRenderNode)
    {
        bIsHidden = (pRenderNode->GetRndFlags() & ERF_HIDDEN) != 0;
        if (hideObject)
        {
            pRenderNode->SetRndFlags(ERF_HIDDEN, true);
        }
    }

    QString texname = Path::GetFileName(filename);
    QString path = Path::GetPath(filename);

    // Add _CM suffix if missing
    int32 nCMSufixCheck = texname.indexOf("_cm");
    texname = Path::Make(path, texname + ((nCMSufixCheck == -1) ? "_cm.tif" : ".tif"));

    // Assign this texname to current material.
    texname = Path::ToUnixPath(texname);

    // Temporary solution to save both dds and tiff hdr cubemap
    AABB pObjAABB;
    pObject->GetBoundBox(pObjAABB);

    Vec3 pObjCenter = pObjAABB.GetCenter();

    bool success = GenHDRCubemapTiff(texname, res, pObjCenter);

    // restore object's visibility
    if (pRenderNode)
    {
        pRenderNode->SetRndFlags(ERF_HIDDEN, bIsHidden);
    }

    filename = Path::ToUnixPath(texname);

    return success;
}

//////////////////////////////////////////////////////////////////////////
bool CubemapUtils::GenHDRCubemapTiff(const QString& fileName, int nDstSize, Vec3& pos)
{
    int nSrcSize = nDstSize * 4; // Render 16x bigger cubemap (4x4) - 16x SSAA

    TArray<unsigned short> vecData;
    vecData.Reserve(nSrcSize * nSrcSize * 6 * 4);
    vecData.SetUse(0);

    if (!GetIEditor()->GetRenderer()->EF_RenderEnvironmentCubeHDR(nSrcSize, pos, vecData))
    {
        assert(0);
        return false;
    }

    assert(vecData.size() == nSrcSize * nSrcSize * 6 * 4);

    // todo: such big downsampling should be on gpu

    // save data to tiff
    // resample the image at the original size
    CWordImage img;
    img.Allocate(nDstSize * 4 * 6, nDstSize);

    size_t srcPitch = nSrcSize * 4;
    size_t srcSlideSize = nSrcSize * srcPitch;

    size_t dstPitch = nDstSize * 4;
    for (int side = 0; side < 6; ++side)
    {
        for (uint32 y = 0; y < nDstSize; ++y)
        {
            CryHalf4* pSrcSide = (CryHalf4*)&vecData[side * srcSlideSize];
            CryHalf4* pDst = (CryHalf4*)&img.ValueAt(side * dstPitch, y);
            for (uint32 x = 0; x < nDstSize; ++x)
            {
                Vec4 cResampledColor(0.f, 0.f, 0.f, 0.f);

                // resample the image at the original size
                for (uint32 yres = 0; yres < 4; ++yres)
                {
                    for (uint32 xres = 0; xres < 4; ++xres)
                    {
                        const CryHalf4& pSrc = pSrcSide[(y * 4 + yres) * nSrcSize + (x * 4 + xres)];
                        cResampledColor += Vec4(CryConvertHalfToFloat(pSrc.x), CryConvertHalfToFloat(pSrc.y), CryConvertHalfToFloat(pSrc.z), CryConvertHalfToFloat(pSrc.w));
                    }
                }

                cResampledColor /= 16.f;

                *pDst++ = CryHalf4(cResampledColor.x, cResampledColor.y, cResampledColor.z, cResampledColor.w);
            }
        }
    }

    assert(CryMemory::IsHeapValid());

    CImageTIF tif;
    const bool res = tif.SaveRAW(fileName, img.GetData(), nDstSize * 6, nDstSize, 2, 4, true, "HDRCubemap_highQ");
    assert(res);
    return res;
}

//function will recurse all probes and generate a cubemap for each
void CubemapUtils::RegenerateAllEnvironmentProbeCubemaps()
{
    EBUS_EVENT(AzToolsFramework::EditorRequests::Bus, GenerateAllCubemaps);
}
