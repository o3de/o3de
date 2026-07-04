/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/EntityId.h>
#include <AzCore/std/string/string.h>
#include <Shine/UiBase.h>

class EditorWindow;
class HierarchyWidget;

//! Manages UI prefab operations for the Shine UI editor.
//! UI prefabs (.uiprefab) are reusable UI element templates that can be saved from and
//! instantiated into UI canvases. They use the same entity serialization as canvas elements
//! (via SerializeHelpers) stored as files on disk.
class UiPrefabManager
{
public:
    explicit UiPrefabManager(EditorWindow* editorWindow);

    //! Open a file browser and instantiate the selected .uiprefab file's entities
    //! as children of the current selection (or root if nothing selected)
    void InstantiateUsingBrowser(HierarchyWidget* hierarchy);

    //! Instantiate a specific .uiprefab file into the canvas
    //! \param prefabPath   Absolute path to the .uiprefab file
    //! \param hierarchy    The hierarchy widget
    void InstantiatePrefab(const AZStd::string& prefabPath, HierarchyWidget* hierarchy);

    //! Save the currently selected elements (and their descendants) as a .uiprefab file
    //! Opens a file-save dialog for the user to choose the destination
    void CreatePrefabFromSelection(HierarchyWidget* hierarchy);

    //! Get the file filter for UI prefab files
    static const char* GetPrefabFileFilter();

    //! Get the file extension for UI prefab files (without the dot)
    static const char* GetPrefabExtension();

private:
    EditorWindow* m_editorWindow = nullptr;
};
