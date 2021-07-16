/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Class description of CBaseObject


#ifndef CRYINCLUDE_EDITOR_OBJECTS_CLASSDESC_H
#define CRYINCLUDE_EDITOR_OBJECTS_CLASSDESC_H
#pragma once

#include "Plugin.h"
#include "Include/ObjectEvent.h"
#include <QString>

class CXmlArchive;

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
//! Virtual base class description of CBaseObject.
//! Override this class to create specific Class descriptions for every base object class.
//! Type name is specified like this:
//! Category\Type ex: "TagPoint\Respawn"
class SANDBOX_API CObjectClassDesc
    : public IClassDesc
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
public:
    CObjectClassDesc()
    {
        m_nTextureIcon = 0;
    }

    //! Release class description.
    virtual ObjectType GetObjectType() = 0;
    virtual QObject* CreateQObject() const { return nullptr; }
    //! If this function return not empty string,object of this class must be created with file.
    //! Return root path where to look for files this object supports.
    //! Also wild card for files can be specified, ex: Objects\*.cgf
    virtual QString GetFileSpec()
    {
        return "";
    }

    virtual ESystemClassID SystemClassID() { return ESYSTEM_CLASS_OBJECT; };
    virtual void ShowAbout() {};
    virtual bool CanExitNow() { return true; }
    virtual void Serialize([[maybe_unused]] CXmlArchive& ar) {};
    //! Ex. Object with creation order 200 will be created after any object with order 100.
    virtual int GameCreationOrder() { return 100; };
    virtual QString GetTextureIcon() { return QString(); };
    int GetTextureIconId();
    virtual bool RenderTextureOnTop() const { return false; }

    virtual QString GetToolClassName() { return "EditTool.ObjectCreate"; }

    QString MenuSuggestion() { return{}; }
    QString Tooltip() { return{}; }
    QString Description() { return{}; }

private:
    int m_nTextureIcon;
};
#endif // CRYINCLUDE_EDITOR_OBJECTS_CLASSDESC_H
