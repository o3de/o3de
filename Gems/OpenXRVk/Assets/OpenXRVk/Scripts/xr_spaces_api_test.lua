-- this LUA script shows how to use the OpenXRReferenceSpaces API.
-- It creates a custom reference space called 'MySpace', and prints
-- its pose, along with the pose of the `View` Reference Space pose,
-- and each Eye pose during OnTick().
local xr_spaces_api_test = {
    Properties = {
    }
}

function xr_spaces_api_test:OnActivate()
    local spaces = OpenXRReferenceSpaces.GetReferenceSpaceNames()
    local size = spaces:Size()
    Debug.Log("xr_spaces_api_test:OnActivate Got the following list with " .. tostring(size) .. " spaces")
    for idx=1, size do
        local name = spaces:At(idx)
        Debug.Log("space[" .. tostring(idx) .. "]=" .. name)
    end

    local newSpaceName = "MySpace"
    local tm = Transform.CreateTranslation(Vector3(0.0, 1.0, 0.0))
    local outcome = OpenXRReferenceSpaces.AddReferenceSpace(OpenXRReferenceSpaces.ReferenceSpaceIdView, newSpaceName, tm)
    if outcome:IsSuccess() then
        Debug.Log("Sucessfully created space named " .. newSpaceName)
        self._mySpaceName = newSpaceName
        self.tickBusHandler = TickBus.Connect(self);
    else
        Debug.Log("Failed to create space named " .. newSpaceName .. ". Reason: " .. outcome:GetError())
    end

    local baseSpaceForViewSpaceName = OpenXRReferenceSpaces.GetBaseSpaceForViewSpacePose()
    Debug.Log("FYI: [" .. OpenXRReferenceSpaces.ReferenceSpaceNameView .. "] space will be located with base space [" .. baseSpaceForViewSpaceName .. "]")

    local leftEyeIndex = OpenXRReferenceSpaces.LeftEyeViewId
    local rightEyeIndex = OpenXRReferenceSpaces.RightEyeViewId
    Debug.Log("FYI: LeftEyeIndex=[" .. tostring(leftEyeIndex) .. "], RightEyeIndex=[" .. tostring(rightEyeIndex) .. "]")

end

function xr_spaces_api_test:OnDeactivate()
    if self._mySpaceName ~= nil then
        local outcome = OpenXRReferenceSpaces.RemoveReferenceSpace(self._mySpaceName)
        if outcome:IsSuccess() then
            Debug.Log("Sucessfully removed space named " .. self._mySpaceName)
        else
            Debug.Log("Failed to remove space named " .. self._mySpaceName .. ". Reason: " .. outcome:GetError())
        end
        self._mySpaceName = nil
    end

    if self.tickBusHandler ~= nil then
        self.tickBusHandler:Disconnect()
    end
end

function xr_spaces_api_test:_DumpPoses(deltaTime, timePoint)
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

function xr_spaces_api_test:OnTick(deltaTime, timePoint)
    self:_DumpPoses(deltaTime, timePoint)
end

return xr_spaces_api_test