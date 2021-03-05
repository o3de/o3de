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

// Description : These are helper classes for containing the data from the
//               generic overwrite dialog.

#ifndef CRYINCLUDE_EDITOR_DIALOGS_GENERIC_USEROPTIONS_H
#define CRYINCLUDE_EDITOR_DIALOGS_GENERIC_USEROPTIONS_H
#pragma once


// Small helper class.
// Hint: have one for files and other for directories.
// Hint: used a CUserOptionsReferenceCountHelper to automatically control the reference counts
// of any CUserOptions variable: usefull for recursion when you don't want to use
// only static variables. See example in FileUtill.cpp, function CopyTree.
class CUserOptions
{
    //////////////////////////////////////////////////////////////////////////
    // Types & typedefs
public:
    enum EOption
    {
        ENotSet,
        EYes = 6,
        ENo = 7,
        ECancel = 2,
    };

    class CUserOptionsReferenceCountHelper
    {
    public:
        CUserOptionsReferenceCountHelper(CUserOptions& roUserOptions);
        virtual ~CUserOptionsReferenceCountHelper();
    protected:
        CUserOptions&   m_roReferencedUserOptionsObject;
    };
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Methods
public:
    CUserOptions();

    bool        IsOptionValid();

    int         GetOption();

    bool        IsOptionToAll();

    void        SetOption(int   nNewOption, bool boToAll);

    int DecRef();
    int IncRef();
protected:
private:
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    // Data
public:
protected:
    int                         m_nCurrentOption;
    bool                        m_boToAll;
    int                         m_nNumberOfReferences;
private:
    //////////////////////////////////////////////////////////////////////////
};
#endif // CRYINCLUDE_EDITOR_DIALOGS_GENERIC_USEROPTIONS_H
