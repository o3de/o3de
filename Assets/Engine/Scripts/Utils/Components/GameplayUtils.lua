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
--
----------------------------------------------------------------------------------------------------

-- require with:
-- local gameplayMultiHandler = require('scripts.utils.components.gameplayutils')

-- use like:
-- self.gameplayHandlers = gameplayMultiHandler.ConnectMultiHandlers{
--     [GameplayNotificationId("jump")] = {
--         OnEventBegin = function(floatValue) self:JumpStart(floatValue) end,
--         OnEventUpdating = function(floatValue) self:Jumping(floatValue) end,
--         OnEventEnd = function(floatValue) self:JumpEnded(floatValue) end,
--     },
-- }

-- disconnect from like this:
-- self.gameplayHandlers:Disconnect()

local multiHandlers = require('scripts.utils.components.MultiHandlers')
return {
    ConnectMultiHandlers = multiHandlers(GameplayNotificationBus, GameplayNotificationId),
}