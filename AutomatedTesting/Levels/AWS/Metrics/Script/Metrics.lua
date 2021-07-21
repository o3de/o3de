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
local metrics = {
}

function metrics:OnActivate()
    self.tickTime = 0
    self.numSubmittedMetricsEvents = 0

    self.tickBusHandler = TickBus.Connect(self,self.entityId)
    self.metricsNotificationHandler = AWSMetricsNotificationBus.Connect(self, self.entityId)

    LyShineLua.ShowMouseCursor(true)
end

function metrics:OnSendMetricsSuccess(requestId)
    Debug.Log("Metrics is sent successfully.")
end

function metrics:OnSendMetricsFailure(requestId, errorMessage)
    Debug.Log("Failed to send metrics.")
end

function metrics:OnDeactivate()
    self.tickBusHandler:Disconnect()
    self.metricsNotificationHandler:Disconnect()
end

function metrics:OnTick(deltaTime, timePoint)
    self.tickTime = self.tickTime + deltaTime

    if self.tickTime > 5.0 then
        defaultAttribute = AWSMetrics_MetricsAttribute()
        defaultAttribute:SetName("event_name")
        defaultAttribute:SetStrValue("login")

        customAttribute = AWSMetrics_MetricsAttribute()
        customAttribute:SetName("custom_attribute")
        customAttribute:SetStrValue("value")

        attributeList = AWSMetrics_AttributesSubmissionList()
        attributeList.attributes:push_back(defaultAttribute)
        attributeList.attributes:push_back(customAttribute)


        if self.numSubmittedMetricsEvents % 2 == 0 then
            if AWSMetricsRequestBus.Broadcast.SubmitMetrics(attributeList.attributes, 0, "lua", false) then
                Debug.Log("Submitted metrics without buffer.")

                AWSMetricsRequestBus.Broadcast.FlushMetrics()
                Debug.Log("Flushed the buffered metrics.")
            else
                Debug.Log("Failed to Submit metrics without buffer.")
            end
        else
            if AWSMetricsRequestBus.Broadcast.SubmitMetrics(attributeList.attributes, 0, "lua", true) then
                Debug.Log("Submitted metrics with buffer.")
            else
                Debug.Log("Failed to Submit metrics with buffer.")
            end
        end

        self.numSubmittedMetricsEvents = self.numSubmittedMetricsEvents + 1
        self.tickTime = 0
    end
end

return metrics
