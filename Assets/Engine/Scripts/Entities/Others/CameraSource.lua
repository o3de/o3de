----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
CameraSource = {
    Editor={
        Icon="Camera.bmp",
    },
};

------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
function CameraSource:OnInit()
    self:CreateCameraComponent();
    self:SetFlags(ENTITY_FLAG_CLIENT_ONLY,0);
end
