/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once
// The aim of IResourceSelectorHost is to unify resource selection dialogs in a one
// API that can be reused with plugins. It also makes possible to register new
// resource selectors dynamically, e.g. inside plugins.
//
// Here is how new selectors are created. In your implementation file you add handler function:
//
//   #include "IResourceSelectorHost.h"
//
//   QString SoundFileSelector(const SResourceSelectorContext& x, const QString& previousValue)
//   {
//     CMyModalDialog dialog(CWnd::FromHandle(x.parentWindow));
//     ...
//     return previousValue;
//   }
//   REGISTER_RESOURCE_SELECTOR("Sound", SoundFileSelector, "Icons/sound_16x16.png")
//
// Here is how it can be invoked directly:
//
//   SResourceSelectorContext x;
//   x.parentWindow = parent.GetSafeHwnd();
//   x.typeName = "Sound";
//   string newValue = GetIEditor()->GetResourceSelector()->SelectResource(x, previousValue).c_str();
//
// If you have your own resource selectors in the plugin you will need to run
//
//    RegisterModuleResourceSelectors(GetIEditor()->GetResourceSelector())
//
// during plugin initialization.
//
// If you want to be able to pass some custom context to the selector (e.g. source of the information for the
// list of items or something similar) then you can add a poitner argument to your selector function, i.e.:
//
//   QString SoundFileSelector(const SResourceSelectorContext& x, const QString& previousValue,
//                             SoundFileList* list) // your context argument

#include <QString>

class QWidget;

struct SResourceSelectorContext
{
    const char* typeName;

    // use either parentWidget or parentWindow (not both) until everything porting to QWidget.
    QWidget* parentWidget;

    unsigned int entityId;
    void* contextObject;

    SResourceSelectorContext()
        : parentWidget(0)
        , typeName(0)
        , entityId(0)
        , contextObject()
    {
    }
};

// TResourceSelecitonFunction is used to declare handlers for specific types.
//
// For canceled dialogs previousValue should be returned.
typedef QString (* TResourceSelectionFunction)(const SResourceSelectorContext& selectorContext, const QString& previousValue);
typedef QString (* TResourceSelectionFunctionWithContext)(const SResourceSelectorContext& selectorContext, const QString& previousValue, void* contextObject);

struct SStaticResourceSelectorEntry;

// See note at the beginning of the file.
struct IResourceSelectorHost
{
    virtual ~IResourceSelectorHost() = default;
    virtual QString SelectResource(const SResourceSelectorContext& context, const QString& previousValue) = 0;
    virtual const char* ResourceIconPath(const char* typeName) const = 0;

    virtual void RegisterResourceSelector(const SStaticResourceSelectorEntry* entry) = 0;

    // secondary responsibility of this class is to store global selections
    virtual void SetGlobalSelection(const char* resourceType, const char* value) = 0;
    virtual const char* GetGlobalSelection(const char* resourceType) const = 0;
};

// ---------------------------------------------------------------------------
#define INTERNAL_RSH_COMBINE_UTIL(A, B) A##B
#define INTERNAL_RSH_COMBINE(A, B) INTERNAL_RSH_COMBINE_UTIL(A, B)
#define REGISTER_RESOURCE_SELECTOR(name, function, icon) \
    static SStaticResourceSelectorEntry INTERNAL_RSH_COMBINE(selector_##function, __LINE__)((name), (function), (icon));

struct SStaticResourceSelectorEntry
{
    const char* typeName;
    TResourceSelectionFunction function;
    TResourceSelectionFunctionWithContext functionWithContext;
    const char* iconPath;

    static SStaticResourceSelectorEntry*& GetFirst() { static SStaticResourceSelectorEntry* first; return first; }
    SStaticResourceSelectorEntry* next;

    SStaticResourceSelectorEntry(const char* typeName, TResourceSelectionFunction function, const char* icon)
        : typeName(typeName)
        , function(function)
        , functionWithContext()
        , iconPath(icon)
    {
        next = GetFirst();
        GetFirst() = this;
    }

    template<class T>
    SStaticResourceSelectorEntry(const char* typeName, QString (*function)(const SResourceSelectorContext&, const QString& previousValue, T * context), const char* icon)
        : typeName(typeName)
        , function()
        , functionWithContext(TResourceSelectionFunctionWithContext(function))
        , iconPath(icon)
    {
        next = GetFirst();
        GetFirst() = this;
    }
};

inline void RegisterModuleResourceSelectors(IResourceSelectorHost* editorResourceSelector)
{
    for (SStaticResourceSelectorEntry* current = SStaticResourceSelectorEntry::GetFirst(); current != 0; current = current->next)
    {
        editorResourceSelector->RegisterResourceSelector(current);
    }
}
