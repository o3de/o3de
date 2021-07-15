----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
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