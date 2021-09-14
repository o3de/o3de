/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/View/Windows/Tools/UpgradeTool/Modifier.h>

namespace ModifierCpp
{

}

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        Modifier::Modifier
            ( const ModifyConfiguration& modification
            , AZStd::vector<AZ::Data::AssetInfo>&& assets
            , AZStd::function<void()> onComplete)
        {

            ModelNotificationsBus::Broadcast(&ModelNotificationsTraits::OnUpgradeAllBegin);
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void Modifier::OnSystemTick()
        {

        }
    }
}
