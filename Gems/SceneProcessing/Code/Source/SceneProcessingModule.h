/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Module/DynamicModuleHandle.h>

namespace AZ::SceneProcessing
{
    inline AZStd::unique_ptr<DynamicModuleHandle> s_sceneCoreModule;
    inline AZStd::unique_ptr<DynamicModuleHandle> s_sceneDataModule;
    inline AZStd::unique_ptr<DynamicModuleHandle> s_sceneBuilderModule;
} // namespace AZ::SceneProcessing
