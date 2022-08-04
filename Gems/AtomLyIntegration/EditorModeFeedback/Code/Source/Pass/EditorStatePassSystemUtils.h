/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Pass/State/EditorStateBase.h>

#pragma once

namespace AZ::Render
{
    inline constexpr const char* const StatePassTemplatePassClassName = "EditorStateParentPass";
    inline constexpr const char* const BufferCopyStatePassTemplatePassClassName = "EditorStateBufferCopyPass";

    //! Returns the name for the mask pass template for the specified draw list.
    Name GetMaskPassTemplateNameForDrawList(const Name& drawList);

    //! Returns the name for the mask pass for the specified draw list.
    Name GetMaskPassNameForDrawList(const Name& drawList);

    //! Returns the name for the buffer copy pass template for the specified editor state.
    Name GetBufferCopyPassTemplateName(const EditorStateBase& state);
    
    //! Returns the name for the buffer copy pass for the specified editor state.
    Name GetBufferCopyPassNameForState(const EditorStateBase& state);

    //! Creates and adds to the Atom pass system the parent pass template for the specified editor state.
    void CreateAndAddStateParentPassTemplate(const EditorStateBase& state);

    //! Creates and adds to the Atom pass system the buffer copy pass template for the specified editor state.
    void CreateAndAddBufferCopyPassTemplate(const EditorStateBase& state);

    //! Creates and adds to the Atom pass system the pass template for the mask with the specified draw list.
    void CreateAndAddMaskPassTemplate(const Name& drawList);

    //! Creates the mask pass templates from the list of editor state parent passes and returns the drawlists used by the masks.
    AZStd::unordered_set<Name> CreateMaskPassTemplatesFromEditorStates(const EditorStateList& editorStates);
} // namespace AZ::Render
