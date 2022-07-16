LoadLevel("prefabs/default_level.spawnable")
-- Wait for level to load
IdleSeconds(5)

StartBudgetTotalCapture()
IdleFrames(100)
StopBudgetTotalCapture()
WritePerfDataToCsvFile("@user@/PerfData/metrics.csv")