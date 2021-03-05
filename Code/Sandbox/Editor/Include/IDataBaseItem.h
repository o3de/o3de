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

#ifndef CRYINCLUDE_EDITOR_INCLUDE_IDATABASEITEM_H
#define CRYINCLUDE_EDITOR_INCLUDE_IDATABASEITEM_H
#pragma once

#include <qwindowdefs.h>
#include <IEditor.h>

struct IDataBaseLibrary;
class CUsedResources;

//////////////////////////////////////////////////////////////////////////
/** Base class for all items contained in BaseLibraray.
*/
struct IDataBaseItem
{
    struct SerializeContext
    {
        XmlNodeRef node;
        bool bUndo;
        bool bLoading;
        bool bCopyPaste;
        bool bIgnoreChilds;
        bool bUniqName;
        SerializeContext()
            : node(0)
            , bLoading(false)
            , bCopyPaste(false)
            , bIgnoreChilds(false)
            , bUniqName(false)
            , bUndo(false) {};
        SerializeContext(XmlNodeRef _node, bool bLoad)
            : node(_node)
            , bLoading(bLoad)
            , bCopyPaste(false)
            , bIgnoreChilds(false)
            , bUniqName(false)
            , bUndo(false) {};
        SerializeContext(const SerializeContext& ctx)
            : node(ctx.node)
            , bLoading(ctx.bLoading)
            , bCopyPaste(ctx.bCopyPaste)
            , bIgnoreChilds(ctx.bIgnoreChilds)
            , bUniqName(ctx.bUniqName)
            , bUndo(ctx.bUndo) {};
    };

    virtual EDataBaseItemType GetType() const = 0;

    //! Return Library this item are contained in.
    //! Item can only be at one library.
    virtual IDataBaseLibrary* GetLibrary() const = 0;

    //! Change item name.
    virtual void SetName(const QString& name) = 0;
    //! Get item name.
    virtual const QString& GetName() const = 0;

    //! Get full item name, including name of library.
    //! Name formed by adding dot after name of library
    //! eg. library Pickup and item PickupRL form full item name: "Pickups.PickupRL".
    virtual QString GetFullName() const = 0;

    //! Get only nameof group from prototype.
    virtual QString GetGroupName() = 0;
    //! Get short name of prototype without group.
    virtual QString GetShortName() = 0;

    //! Serialize library item to archive.
    virtual void Serialize(SerializeContext& ctx) = 0;

    //! Generate new unique id for this item.
    virtual void GenerateId() = 0;
    //! Returns GUID of this material.
    virtual const GUID& GetGUID() const = 0;

    //! Validate item for errors.
    virtual void Validate() {};

    //! Gathers resources by this item.
    virtual void GatherUsedResources([[maybe_unused]] CUsedResources& resources) {};
};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IDATABASEITEM_H
