----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
--
--
--
----------------------------------------------------------------------------------------------------
MannequinObject =
{

    Properties =
    {
        objModel = "",
        fileActionController = "",
        fileAnimDatabase3P = "",
    },

    Editor = 
    {
        Icon = "user.bmp",
        IconOnTop = 1,
    },
}


function MannequinObject:OnPropertyChange()
    -- The OnPropertyChange callback is forwarded to script directly by the editor.
    -- As most of this entity is written in C++, we just want to send a notification
    -- that a property has changed, and deal with it there.
    self:ProcessBroadcastEvent( "OnPropertyChange" );
end
