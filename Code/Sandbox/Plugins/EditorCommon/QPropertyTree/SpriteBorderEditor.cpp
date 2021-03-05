//-------------------------------------------------------------------------------
// Copyright (C) Amazon.com, Inc. or its affiliates.
// All Rights Reserved.
//
// Licensed under the terms set out in the LICENSE.HTML file included at the
// root of the distribution; you may not use this file except in compliance
// with the License.
//
// Do not remove or modify this notice or the LICENSE.HTML file.  This file
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
// either express or implied. See the License for the specific language
// governing permissions and limitations under the License.
//-------------------------------------------------------------------------------

// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorCommon_precompiled.h"
#include "SpriteBorderEditorCommon.h"
#include <Util/PathUtil.h> // for Getting the game folder

//-------------------------------------------------------------------------------

#define VIEW_WIDTH  ( 200 )
#define VIEW_HEIGHT  ( 200 )
#define MANIPULATOR_THICKNESS_IN_PIXELS   ( 24 )

//-------------------------------------------------------------------------------

SpriteBorderEditor::SpriteBorderEditor(const char* path, QWidget* parent)
: QDialog( parent )
, hasBeenInitializedProperly( true )
{
    ISprite* sprite = gEnv->pLyShine->LoadSprite( path );
    CRY_ASSERT( sprite );

    // The layout.
    QGridLayout* outerGrid = new QGridLayout(this);

    QGridLayout* innerGrid = new QGridLayout();
    outerGrid->addLayout( innerGrid, 0, 0, 1, 2 );

    // The scene.
    QGraphicsScene* scene( new QGraphicsScene( 0.0f, 0.0f, VIEW_WIDTH, VIEW_HEIGHT, this ) );

    // The view.
    innerGrid->addWidget( new SlicerView( scene, this ), 0, 0, 6, 1 );

    // The image.
    QGraphicsPixmapItem* pixmapItem = nullptr;
    QSize unscaledPixmapSize;
    QSize scaledPixmapSize;
    {
        // assets can be in gems as well as in the current project. Qpixmap requires a path to
        // the asset and doesn't know about such concepts. So adjust the pathname to something
        // that Qpixmap can open.
        QString fullPath = Path::GamePathToFullPath(sprite->GetTexturePathname().c_str());
        QPixmap unscaledPixmap(fullPath);

        bool isVertical = ( unscaledPixmap.size().height() > unscaledPixmap.size().width() );

        // Scale-to-fit, while preserving aspect ratio.
        pixmapItem = scene->addPixmap( isVertical ? unscaledPixmap.scaledToHeight( VIEW_HEIGHT ) : unscaledPixmap.scaledToWidth( VIEW_WIDTH ) );

        unscaledPixmapSize = unscaledPixmap.size();
        scaledPixmapSize = pixmapItem->pixmap().size();
    }

    // Add text fields and manipulators.
    {
        int row = 0;

        innerGrid->addWidget( new QLabel( QString( "Texture is %1 x %2" ).arg( QString::number( unscaledPixmapSize.width() ),
                                                                                QString::number( unscaledPixmapSize.height() ) ),
                                            this ),
                                row++,
                                1 );

        for( SpriteBorder b : SpriteBorder() )
        {
            SlicerEdit* edit = new SlicerEdit( b,
                                                unscaledPixmapSize,
                                                sprite );

            SlicerManipulator* manipulator = new SlicerManipulator( b,
                                                                    unscaledPixmapSize,
                                                                    scaledPixmapSize,
                                                                    MANIPULATOR_THICKNESS_IN_PIXELS,
                                                                    sprite,
                                                                    scene );

            edit->SetManipulator( manipulator );
            manipulator->SetEdit( edit );

            innerGrid->addWidget( new QLabel( SpriteBorderToString( b ), this ), row, 1 );
            innerGrid->addWidget( edit, row, 2 );
            innerGrid->addWidget( new QLabel( "pixels", this ), row, 3 );
            ++row;
        }
    }

    // Add buttons.
    {
        // Save button.
        QPushButton* saveButton = new QPushButton( "Save", this );
        QObject::connect( saveButton,
                            &QPushButton::clicked, this,
                            [ this, sprite ]([[maybe_unused]]  bool checked )
                            {
                                // Sanitize values.
                                //
                                // This is the simplest way to sanitize the
                                // border values. Otherwise, we need to prevent
                                // flipping the manipulators in the UI.
                                {
                                    ISprite::Borders b = sprite->GetBorders();

                                    if( b.m_top > b.m_bottom )
                                    {
                                        std::swap( b.m_top, b.m_bottom );
                                    }

                                    if( b.m_left > b.m_right )
                                    {
                                        std::swap( b.m_left, b.m_right );
                                    }

                                    sprite->SetBorders( b );
                                }

                                QString fullPath = Path::GamePathToFullPath(sprite->GetPathname().c_str());
                                bool result = sprite->SaveToXml(fullPath.toUtf8().data());

                                if (result)
                                {
                                    close();
                                }
                                else
                                {
                                    QMessageBox box(QMessageBox::Critical,
                                        "Error",
                                        "Unable to save file",
                                        QMessageBox::Ok);
                                    box.exec();
                                }
                            } );
        outerGrid->addWidget( saveButton, 1, 0 );

        // Cancel button.
        ISprite::Borders originalBorders = sprite->GetBorders();
        QPushButton* cancelButton = new QPushButton( "Cancel", this );
        QObject::connect( cancelButton,
                            &QPushButton::clicked, this,
                            [ this, sprite, originalBorders ]([[maybe_unused]]  bool checked )
                            {
                                // Restore original borders.
                                sprite->SetBorders( originalBorders );

                                close();
                            } );
        outerGrid->addWidget( cancelButton, 1, 1 );
    }

    setWindowTitle( "SpriteBorderEditor" );
    setModal( true );
    setWindowModality( Qt::ApplicationModal );

    layout()->setSizeConstraint( QLayout::SetFixedSize );
}

bool SpriteBorderEditor::GetHasBeenInitializedProperly()
{
    return hasBeenInitializedProperly;
}

#include <QPropertyTree/moc_SpriteBorderEditor.cpp>
