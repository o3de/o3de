-- Basic camera movement that uses the default OpenXR ActionSets asset
-- that ships with the OpenXRVk Gem.
-- This script can be used as an alternative to XrCameraMovementComponent.
local xr_camera_move = {
    Properties = {
        cameraEntity = {default = EntityId(),
                        description = "The entity with a camera component"},
        cameraSpeed = { default = 1.0,
                        suffix= "ms-1",
                        description="Camera speed."},
        rotationStepSize = {
            default = 30.0,
            suffix = "deg",
            description = "Rotation step size in degrees around Up(Z) Axis. Each time the user moves the right hand thumbstick to the left or to the right the camera will Yaw rotate by this amount of degrees."
        }
    }
}

local function DumpActionHandle(name, actionHandle)
    if actionHandle:IsValid() then
        Debug.Log("Action [" ..name .. "] has index=[" .. actionHandle:GetIndex().. "]")
    else
        Debug.Log("Action [" ..name .. "] is invalid")
    end
end

local function GetActionHandle(actionSetName, actionName)
    local actionHandle = OpenXRActions.GetActionHandle(actionSetName, actionName)
    DumpActionHandle(actionName, actionHandle)
    assert(actionHandle:IsValid(), "Failed to get action handle [" .. actionName .. "] from action set[" .. actionSetName .. "]")
    return actionHandle
end

local function FilterDeadZone(value, deadzoneMagnitude)
    deadzoneMagnitude = deadzoneMagnitude or 0.05
    if math.abs(value) < deadzoneMagnitude then
        return 0.0
    end
    return value
end

function xr_camera_move:OnActivate()
    assert(EntityId.IsValid(self.Properties.cameraEntity), "xr_camera_move:OnActivate. Invalid camera entity.")

    -- Cache all action handles
    local actionSetName = "main_action_set"
    self._moveFrontwaysHandle = GetActionHandle(actionSetName, "move_frontways")
    self._moveSidewaysHandle = GetActionHandle(actionSetName, "move_sideways")
    self._yawRotateHandle = GetActionHandle(actionSetName, "shift_yaw_rotate")
    self._moveUpHandle = GetActionHandle(actionSetName, "move_up")
    self._moveDownHandle = GetActionHandle(actionSetName, "move_down")

    self._cameraMovementStates = Vector3(0.0, 0.0, 0.0)
    self._cameraYawRotationState = 0.0
    self._AbsRange = 0.9
    self.tickBusHandler = TickBus.Connect(self);

end

function xr_camera_move:OnDeactivate()
    if self.tickBusHandler ~= nil then
        self.tickBusHandler:Disconnect()
    end
end

function xr_camera_move:_DumpPoses(deltaTime, timePoint)
    local outcome = OpenXRReferenceSpaces.GetReferenceSpacePose(self._mySpaceName, "Local")
    if outcome:IsSuccess() then
        local tm = outcome:GetValue()
        Debug.Log("Current transform for <" .. self._mySpaceName .. "> == " .. tostring(tm))
    end

    local headTm = OpenXRReferenceSpaces.GetViewSpacePose()
    Debug.Log("View space pose=\n" .. tostring(headTm))

    local eyeCount = OpenXRReferenceSpaces.GetViewCount()
    Debug.Log("Eye count=\n" .. tostring(eyeCount))
    for idx=1, eyeCount do
        local eyeTm = OpenXRReferenceSpaces.GetViewPose(idx - 1)
        Debug.Log("Eye[" .. tostring(idx) .. "] transform=\n" .. tostring(eyeTm))
    end

    local viewPoses = OpenXRReferenceSpaces.GetViewPoses()
    local size = viewPoses:Size()
    Debug.Log("Got the following list with " .. tostring(size) .. " eye poses")
    for idx=1, size do
        local eyeTm = viewPoses:At(idx)
        Debug.Log("Eye pose[" .. tostring(idx) .. "] transform=\n" .. tostring(eyeTm))
    end
end

function xr_camera_move:_ReadActionStates(deltaTime, timePoint)
    self._cameraMovementStates = Vector3(0.0, 0.0, 0.0)

    local outcome = OpenXRActions.GetActionStateFloat(self._moveFrontwaysHandle)
    if outcome:IsSuccess() then
        self._cameraMovementStates.y = FilterDeadZone(outcome:GetValue())
    end

    outcome = OpenXRActions.GetActionStateFloat(self._moveSidewaysHandle)
    if outcome:IsSuccess() then
        self._cameraMovementStates.x = FilterDeadZone(outcome:GetValue())
    end

    outcome = OpenXRActions.GetActionStateBoolean(self._moveUpHandle)
    if outcome:IsSuccess() then
        if outcome:GetValue() then
            self._cameraMovementStates.z = 1.0
        end
    end

    outcome = OpenXRActions.GetActionStateBoolean(self._moveDownHandle)
    if outcome:IsSuccess() then
        if outcome:GetValue() then
            self._cameraMovementStates.z = -1.0
        end
    end

    -- Smooth rotation around the Up (Z) axis, also known as Yaw rotation,
    -- is prone to motion sickness for most users.
    -- So we'll do a rather snappy rotation changes in angular increments.
    outcome = OpenXRActions.GetActionStateFloat(self._yawRotateHandle)
    if not outcome:IsSuccess() then
        return
    end
    local newYawState = FilterDeadZone(outcome:GetValue())
    if newYawState >= self._AbsRange then
        if not self._canRotate then
            self._canRotate = true
            self._cameraYawRotationState = -1.0
        end
    elseif newYawState <= -self._AbsRange then
        if not self._canRotate then
            self._canRotate = true
            self._cameraYawRotationState = 1.0
        end
    else
        self._canRotate = false
    end

end

function xr_camera_move:_MoveCamera(deltaTime, timePoint)
    local distance = self.Properties.cameraSpeed * deltaTime
    local cameraTm = TransformBus.Event.GetWorldTM(self.Properties.cameraEntity)
    local upVector = cameraTm:GetBasisZ()
    local forwardVector = cameraTm:GetBasisY()
    local camVx = cameraTm:GetBasisX() * self._cameraMovementStates.x * distance
    local camVy = forwardVector * self._cameraMovementStates.y * distance
    local camVz = upVector * self._cameraMovementStates.z * distance
    local camDeltaTranslation = camVx + camVy + camVz

    if self._cameraYawRotationState == 0.0 then
        TransformBus.Event.SetWorldTranslation(self.Properties.cameraEntity, cameraTm:GetTranslation() + camDeltaTranslation)
        return
    end

    -- change the camera orientation
    local angleRads = math.rad(self.Properties.rotationStepSize) * self._cameraYawRotationState
    local rotQuat = Quaternion.CreateFromAxisAngle(upVector, angleRads)
    local newForward = rotQuat * forwardVector
    newForward:Normalize()
    local newBasisX = newForward:Cross(upVector)
    newBasisX:Normalize()

    local newLocation = cameraTm:GetTranslation() + camDeltaTranslation
    local mat33 = Matrix3x3.CreateFromColumns(newBasisX, newForward, upVector)
    local newTm = Transform.CreateFromMatrix3x3AndTranslation(mat33, newLocation)

    TransformBus.Event.SetWorldTM(self.Properties.cameraEntity, newTm)
    self._cameraYawRotationState = 0.0
end

function xr_camera_move:OnTick(deltaTime, timePoint)
    -- self:_DumpPoses(deltaTime, timePoint)
    self:_ReadActionStates(deltaTime, timePoint)
    self:_MoveCamera(deltaTime, timePoint)
end

return xr_camera_move