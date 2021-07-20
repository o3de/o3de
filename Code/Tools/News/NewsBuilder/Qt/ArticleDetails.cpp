/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ArticleDetails.h"
#include "SelectImage.h"
#include <NewsShared/ResourceManagement/Resource.h>
#include "ResourceManagement/BuilderResourceManifest.h"
#include "ResourceManagement/ImageDescriptor.h"
#include "NewsShared/ErrorCodes.h"

#include <AzCore/Casting/numeric_cast.h>

#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>

#include "Qt/ui_ArticleDetails.h"

namespace News
{

    ArticleDetails::ArticleDetails(QWidget* parent,
        Resource& resource,
        BuilderResourceManifest& manifest)
        : QWidget(parent)
        , m_ui(new Ui::ArticleDetailsWidget)
        , m_filename("")
        , m_article(resource)
        , m_manifest(manifest)
        , m_imageIdOriginal("")
        , m_imageId("")
    {
        m_ui->setupUi(this);
        m_ui->uidLabel->setText(QString("Article: %1").arg(m_article.GetResource().GetId()));

        resizePreviewImage();

        //! try to load image icon
        auto pImageResource = m_manifest.FindById(m_article.GetImageId());
        if (pImageResource)
        {
            m_imageIdOriginal = pImageResource->GetId();
            SetImage(*pImageResource);
        }
        else
        {
            SetNoImage();
        }

        m_ui->titleText->setText(m_article.GetTitle());
        m_ui->descriptionText->setPlainText(m_article.GetBody());

        connect(m_ui->fromFileButton, &QPushButton::clicked, this, &ArticleDetails::OpenImageFromFile);
        connect(m_ui->fromResourceButton, &QPushButton::clicked, this, &ArticleDetails::OpenImageFromResource);
        connect(m_ui->clearImageButton, &QPushButton::clicked, this, &ArticleDetails::ClearImage);
        connect(m_ui->updateButton, &QPushButton::clicked, this, &ArticleDetails::UpdateArticle);
        connect(m_ui->deleteButton, &QPushButton::clicked, this, &ArticleDetails::DeleteArticle);
        connect(m_ui->cancelButton, &QPushButton::clicked, this, &ArticleDetails::Close);
        connect(m_ui->upButton, &QPushButton::clicked, this, &ArticleDetails::MoveUp);
        connect(m_ui->downButton, &QPushButton::clicked, this, &ArticleDetails::MoveDown);
    }

    ArticleDetails::~ArticleDetails() {}

    void ArticleDetails::resizeEvent(QResizeEvent *event)
    {
        QWidget::resizeEvent(event);
        resizePreviewImage();
    }

    void ArticleDetails::resizePreviewImage()
    {
        // Resize the imagePreview to keep the proportions
        int newHeight = aznumeric_cast<int>(m_imageRatio * width());
        m_ui->imagePreview->setFixedHeight(newHeight);
    }

    ArticleDescriptor& ArticleDetails::GetArticle()
    {
        return m_article;
    }

    QString ArticleDetails::GetId() const
    {
        return m_article.GetResource().GetId();
    }

    void ArticleDetails::OpenImageFromFile()
    {
        m_filename = QFileDialog::getOpenFileName(this,
            "Open Image", ".", "Image Files (*.png *.jpg *.bmp)");
        SetImage(m_filename);
    }

    void ArticleDetails::OpenImageFromResource()
    {
        m_pSelectImage = new SelectImage(m_manifest);
        if (m_pSelectImage->exec() == QDialog::DialogCode::Accepted)
        {
            auto pSelectedResource = m_pSelectImage->GetSelected();
            if (!pSelectedResource)
            {
                QMessageBox msgBox(QMessageBox::Critical,
                    "Error",
                    "No resource pSelectedResource",
                    QMessageBox::Ok,
                    this);
                msgBox.exec();
                return;
            }
            SetImage(*pSelectedResource);
        }
        delete m_pSelectImage;
    }

    //! once the Update button is clicked, article resource is updated in this function
    void ArticleDetails::UpdateArticle()
    {
        //! this flag remains false if no new changes are detected, thus avoiding unnecessary
        //! re-uploading unchanged resources
        bool updated = false;

        m_imageId = LoadImage();
        if (m_imageId.compare(m_imageIdOriginal) != 0)
        {
            m_article.SetImageId(m_imageId);
            m_filename = "";
            m_imageIdOriginal = m_imageId;
            m_ui->imagePathLabel->setText(QString("Image: %1").arg(m_imageId));
            updated = true;
        }

        QString newTitle = m_ui->titleText->text();
        if (m_article.GetTitle().compare(newTitle))
        {
            m_article.SetTitle(newTitle);
            updated = true;
        }
        QString newBody = m_ui->descriptionText->toPlainText();
        if (m_article.GetBody().compare(newBody) != 0)
        {
            m_article.SetBody(newBody);
            updated = true;
        }

        QString newStyle = m_ui->articleStyleButtonGroup->checkedButton()->property("style").toString();
        if (m_article.GetArticleStyle() != newStyle)
        {
            m_article.SetArticleStyle(newStyle);
            updated = true;
        }

        if (updated)
        {
            m_article.Update();
            m_manifest.UpdateResource(&m_article.GetResource());
            emit logSignal(QString("Article %1 updated")
                .arg(m_article.GetResource().GetId()), LogOk);
        }
        else
        {
            emit logSignal("Nothing to update", LogWarning);
        }
        emit updateArticleSignal();
    }

    //! try to load an image either from filename or from another resource,
    //! update resource manifest accordingly
    QString ArticleDetails::LoadImage() const
    {
        if (!m_filename.isEmpty())
        {
            if (!m_imageIdOriginal.isEmpty())
            {
                m_manifest.FreeResource(m_imageIdOriginal);
            }
            auto pResource = m_manifest.AddImage(m_filename);
            if (pResource)
            {
                return pResource->GetId();
            }
            return "";
        }

        if (m_imageId.compare(m_imageIdOriginal) != 0)
        {
            if (!m_imageIdOriginal.isEmpty())
            {
                m_manifest.FreeResource(m_imageIdOriginal);
            }
            if (!m_imageId.isEmpty())
            {
                m_manifest.UseResource(m_imageId);
            }
            return m_imageId;
        }

        return m_imageIdOriginal;
    }

    void ArticleDetails::SetImage(Resource& resource)
    {
        m_ui->imagePathLabel->setText(QString("Image: %1").arg(resource.GetId()));

        QPixmap pixmap;
        if (pixmap.loadFromData(resource.GetData()))
        {
            m_ui->imagePreview->setPixmap(pixmap);
            m_imageId = resource.GetId();
        }
        else
        {
            SetNoImage();
            emit logSignal(QString("Failed to load image: %1.").arg(m_imageId));
        }
    }

    void ArticleDetails::SetImage([[maybe_unused]] QString& filename)
    {
        m_ui->imagePathLabel->setText(QString("Image: %1").arg(m_filename));

        QPixmap pixmap;
        if (pixmap.load(m_filename))
        {
            m_ui->imagePreview->setPixmap(pixmap);
        }
        else
        {
            SetNoImage();
            emit logSignal(QString("Failed to load image: %1.").arg(m_filename));
        }
    }

    void ArticleDetails::SetNoImage() const
    {
        m_ui->imagePreview->setPixmap(QPixmap(":/images/Resources/missing-image.png"));
        m_ui->imagePathLabel->setText("no image");
    }

    void ArticleDetails::ClearImage()
    {
        m_imageId = "";
        m_filename = "";
        SetNoImage();
    }

    void ArticleDetails::DeleteArticle()
    {
        if (!m_article.GetImageId().isEmpty())
        {
            m_manifest.FreeResource(m_article.GetImageId());
        }
        m_manifest.FreeResource(m_article.GetResource().GetId());
        emit deleteArticleSignal();
    }

    void ArticleDetails::Close()
    {
        emit closeArticleSignal();
    }

    void ArticleDetails::MoveUp()
    {
        ErrorCode error;
        if (!m_manifest.UpdateArticleOrder(m_article.GetResource().GetId(), 1, error))
        {
            emit logSignal(GetErrorMessage(error));
        }
        else
        {
            emit orderChangedSignal(m_article.GetResource().GetId(), 1);
        }
    }

    void ArticleDetails::MoveDown()
    {
        ErrorCode error;
        if (!m_manifest.UpdateArticleOrder(m_article.GetResource().GetId(), 0, error))
        {
            emit logSignal(GetErrorMessage(error));
        }
        else
        {
            emit orderChangedSignal(m_article.GetResource().GetId(), 0);
        }
    }

#include "Qt/moc_ArticleDetails.cpp"

}
