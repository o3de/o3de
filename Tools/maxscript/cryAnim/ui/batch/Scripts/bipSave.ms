try
(
	tempVar = (cryTools.cryAnim.UI.main._f.getUI "Settings" "radSavePrompt").state
	(cryTools.cryAnim.UI.main._f.getUI "Settings" "radSavePrompt").state = 2
	(cryTools.cryAnim.UI.main._f.getUI "Load / Save / Export" "Save").pressed()

	(cryTools.cryAnim.UI.main._f.getUI "Settings" "radSavePrompt").state = tempVar
	tempVar = undefined
)
catch ( cryTools.cryAnim.UI.batchProcess._v.customScriptStatus = "Error: Failed to Save" )