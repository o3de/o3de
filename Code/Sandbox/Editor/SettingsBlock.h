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

#pragma once

// ---------------------------------------------------------------------------
// Following utility can be used to add blocks of per-project settings.
// Example:
//
// MyComponent.cpp:
//
//   struct SProjectSettingsMy : SProjectSettingsBlock
//   {
//     bool bMyOption;
//
//     SProjectSettingsMy()
//     : SProjectSettingsBlock("my", "My")
//     , bMyOption(false)
//     {}
//
//     void Serialize(Serialization::IArchive& ar)
//     {
//        ar(bMyOption, "myOption", "My Option");
//     }
//
//   } static gMySettings;
//
//
// Now gMySettings will be loaded and saved automatically and available for
// editing through:
//
//   GetIEditor()->OpenProjectSettings("my");
//
// ---------------------------------------------------------------------------
#ifndef CRYINCLUDE_EDITOR_SETTINGSBLOCK_H
#define CRYINCLUDE_EDITOR_SETTINGSBLOCK_H

namespace Serialization
{
    class IArchive;
    struct SStruct;
};

struct SProjectSettingsBlock
{
    SProjectSettingsBlock(const char* name, const char* label);

    virtual void Serialize(Serialization::IArchive& ar) = 0;

    const char* GetName() const{ return m_name; }
    const char* GetLabel() const{ return m_label; }

    static void GetAllSettingsSerializer(Serialization::SStruct* serializer);
    static SProjectSettingsBlock* Find(const char* name);
    static bool Load();
    static bool Save();
    static const char* GetFilename();
private:
    const char* m_name;
    const char* m_label;
    SProjectSettingsBlock* m_pPrevious;
    static SProjectSettingsBlock* s_pLastBlock;
    friend struct SAllSettingsSerializer;
};

#endif // CRYINCLUDE_EDITOR_SETTINGSBLOCK_H
