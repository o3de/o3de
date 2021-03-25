try
(
	tempVar = (cryTools.cryAnim.UI.main._f.getUI "Settings" "radExportPrompt").state
	(cryTools.cryAnim.UI.main._f.getUI "Settings" "radExportPrompt").state = 2
	(cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "Export").pressed()

	(cryTools.cryAnim.UI.main._f.getUI "Settings" "radSaveExportPrompt").state = tempVar
	tempVar = undefined
)
catch ( cryTools.cryAnim.UI.batchProcess._v.customScriptStatus = "Error: Failed to Export" )