
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzToolsFramework/Manipulators/TranslationManipulators.h>
#include <Editor/EditorSubComponentModeBase.h>

namespace PhysX
{
    class EditorSubComponentModeVec3
        : public PhysX::EditorSubComponentModeBase
    {
    public:
        EditorSubComponentModeVec3(
            const AZ::EntityComponentIdPair& entityComponentIdPair
            , const AZ::Uuid& componentType
            , const AZStd::string& name);
        ~EditorSubComponentModeVec3();

        // PhysX::EditorSubComponentModeBase
        void Refresh() override;

    private:
        void OnManipulatorMoved(const AZ::Vector3& position);

        AzToolsFramework::TranslationManipulators m_translationManipulators;
    };
} // namespace PhysX
