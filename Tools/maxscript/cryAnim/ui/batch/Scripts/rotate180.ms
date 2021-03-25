with undo off
(
	try
		cryTools.cryAnim._f.rotateAnim 180 range:true
	catch
		cryTools.cryAnim.UI.batchProcess._v.customScriptStatus = "Error: Rotating 180"
)
