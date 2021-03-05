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
#include "QPropertyTree.h"
#include "PropertyRowField.h"
#include "PropertyRowSprite.h"
#include "PropertyDrawContext.h"
#include "PropertyTreeModel.h"
#include <Serialization/Decorators/Sprite.h>
#include <Serialization/Decorators/SpriteImpl.h>
#include <Serialization/Decorators/IconXPM.h>
#include <QFileDialog>

extern QString AssetRelativePathFromAbsolutePath(const QString& absPath);

bool PropertyRowSprite::onActivateButton(int buttonIndex, const PropertyActivationEvent& ev)
{
    Show show = FromButtonIndexToShow( buttonIndex );

    if( show == Show::kFilePicker )
    {
        return showFilePicker( ev );
    }

    if( show == Show::kSpriteBorderEditor )
    {
        return showSpriteBorderEditor( ev );
    }

    // This is to avoid a compiler warning.
    // We should NEVER get here.
    CRY_ASSERT( 0 );
    return false;
}

bool PropertyRowSprite::onActivate(const PropertyActivationEvent& ev)
{
    if( PropertyRowField::onActivate( ev ) )
    {
        // PropertyRowSprite::onActivateButton() has handled this event.
        // Nothing else to do.
        return true;
    }

    return showFilePicker( ev );
}

void PropertyRowSprite::setValueAndContext(const Serialization::SStruct& ser, [[maybe_unused]] Serialization::IArchive& ar) 
{
    Serialization::Sprite* value = (Serialization::Sprite *)ser.pointer();

    m_path = value->m_path->c_str();
}

bool PropertyRowSprite::assignTo(const Serialization::SStruct& ser) const 
{
    if( ser.size() != sizeof(Serialization::Sprite) )
    {
        return false;
    }

    Serialization::Sprite* s = (Serialization::Sprite *)ser.pointer();

    *s->m_path = m_path.c_str();

    return true;
}

void PropertyRowSprite::serializeValue(Serialization::IArchive& ar)
{
    ar(m_path, "path");
    ar(m_filter, "filter");
    ar(m_startFolder, "startFolder");
}

int PropertyRowSprite::buttonCount() const
{
    return ( CanBeEdited() ? 2 : 1 );
}

const QIcon& PropertyRowSprite::buttonIcon(const QPropertyTree* tree, int index) const
{
    Show show = FromButtonIndexToShow( index );

    if( show == Show::kFilePicker )
    {
        #include "file_open.xpm"
        static QIcon icon = QIcon(QPixmap::fromImage(*tree->_iconCache()->getImageForIcon(Serialization::IconXPM(file_open_xpm))));
        return icon;
    }

    if( show == Show::kSpriteBorderEditor )
    {
        #include "gear.xpm"
        static QIcon icon = QIcon(QPixmap::fromImage(*tree->_iconCache()->getImageForIcon(Serialization::IconXPM(gear_xpm))));
        return icon;
    }

    // This is to avoid a compiler warning.
    // We should NEVER get here.
    CRY_ASSERT( 0 );

    #include "gear.xpm"
    static QIcon icon;
    return icon;
}

string PropertyRowSprite::valueAsString() const
{
    return m_path;
}

void PropertyRowSprite::clear()
{
    m_path.clear();
}

bool PropertyRowSprite::onContextMenu(QMenu &menu, QPropertyTree* tree)
{
    QAction* action = nullptr;

    action = menu.addAction("Clear");
    QObject::connect( action,
                        &QAction::triggered,
                        tree,
                        [ this, tree ]
                        {
                            Clear( tree );
                        } );

    int buttonIndex = ( buttonCount() - 1 );

    action = menu.addAction(buttonIcon(tree, buttonIndex--), "Pick Resource...");
    QObject::connect( action,
                        &QAction::triggered,
                        tree,
                        [ this, tree ]
                        {
                            PropertyActivationEvent ev;
                            ev.tree = tree;
                            showFilePicker( ev );
                        } );

    if( buttonIndex >= 0 )
    {
        action = menu.addAction(buttonIcon(tree, buttonIndex), "Edit");
        QObject::connect( action,
                            &QAction::triggered,
                            tree,
                            [ this, tree ]
                            {
                                PropertyActivationEvent ev;
                                ev.tree = tree;
                                showSpriteBorderEditor( ev );
                            } );
    }

    return true;
}

bool PropertyRowSprite::processesKey(QPropertyTree* tree, const QKeyEvent* ev)
{
    if (ev->key() == Qt::Key_Delete)
    {
        return true;
    }

    return PropertyRowField::processesKey(tree, ev);
}

bool PropertyRowSprite::onKeyDown(QPropertyTree* tree, const QKeyEvent* ev)
{
    if( ev->key() == Qt::Key_Delete )
    {
        Clear( tree );
        return true;
    }
    return PropertyRowField::onKeyDown(tree, ev);
}

void PropertyRowSprite::Clear(QPropertyTree* tree)
{
    tree->model()->rowAboutToBeChanged(this);
    clear();
    tree->model()->rowChanged(this);
}

bool PropertyRowSprite::showFilePicker(const PropertyActivationEvent& ev)
{
    if (ev.reason == ev.REASON_RELEASE)
        return false;

    // Open the file picker and get the filename the user selects
    // (QFileDialog can traverse symlinks and shortcuts)
    QString filename = QFileDialog::getOpenFileName(ev.tree,
                                                    "Choose file",
                                                    QString(),
                                                    "*.tif;;*.sprite" );

    // Early out.
    {
        if (filename.isEmpty())
        {
            // Nothing selected.
            return true;
        }

        QFileInfo fileInfo( filename );
        if( ! ( ( fileInfo.suffix() == "tif" ) ||
                ( fileInfo.suffix() == "sprite" ) ) )
        {
            // Incompatible files selected.
            return true;
        }
    }

    ev.tree->model()->rowAboutToBeChanged(this);
    m_path = AssetRelativePathFromAbsolutePath(filename).toStdString().c_str();;
    ev.tree->model()->rowChanged(this);

    return true;
}

bool PropertyRowSprite::showSpriteBorderEditor(const PropertyActivationEvent& ev)
{
    SpriteBorderEditor sbe( m_path.c_str(), ev.tree );
    if (sbe.GetHasBeenInitializedProperly())
    {
        sbe.exec();
        return true;
    }

    return false;
}

bool PropertyRowSprite::CanBeEdited() const
{
    return ( ! m_path.empty() );
}

PropertyRowSprite::Show PropertyRowSprite::FromButtonIndexToShow(int index) const
{
    bool showFile = false;
    bool showGear = false;

    if( index )
    {
        // Second icon from the right.

        showFile = true;
    }
    else
    {
        // First icon from the right (right-most icon).

        if( CanBeEdited() )
        {
            showGear = true;
        }
        else
        {
            showFile = true;
        }
    }

    if( showFile )
    {
        return Show::kFilePicker;
    }

    if( showGear )
    {
        return Show::kSpriteBorderEditor;
    }

    // This is to avoid a compiler warning.
    // We should NEVER get here.
    CRY_ASSERT( 0 );
    return (Show)0;
}

DECLARE_SEGMENT(PropertyRowSprite)
REGISTER_PROPERTY_ROW(Serialization::Sprite, PropertyRowSprite); 

