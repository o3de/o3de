----------------------------------------------------------------------------------------------------
--
-- All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
-- its licensors.
--
-- For complete copyright and license terms please see the LICENSE at the root of this
-- distribution (the "License"). All use of this software is governed by the License,
-- or, if provided, by the license below or the license accompanying this file. Do not
-- remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
--
-- Original file Copyright Crytek GMBH or its affiliates, used under license.
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
