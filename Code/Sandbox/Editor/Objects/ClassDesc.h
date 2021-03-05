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
