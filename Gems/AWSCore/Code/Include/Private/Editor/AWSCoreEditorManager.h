/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

namespace AWSCore
{
    class AWSCoreEditorMenu;

    class AWSCoreEditorManager
    {
    public:
        static constexpr const char AWS_MENU_TEXT[] = "&AWS";

        AWSCoreEditorManager();
        virtual ~AWSCoreEditorManager();

        //! Get AWSCoreEditorMenu UI component
        AWSCoreEditorMenu* GetAWSCoreEditorMenu() const;

    private:
        AWSCoreEditorMenu* m_awsCoreEditorMenu;
    };
} // namespace AWSCore
