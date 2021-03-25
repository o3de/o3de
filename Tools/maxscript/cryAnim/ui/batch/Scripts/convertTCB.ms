try
(
	if selection.count > 0 then
		local selArray = selection as Array
	else
		local selArray = Objects as Array
	
	for obj in selArray do
	(
		obj.controller.position.controller = TCB_Position()
		obj.controller.rotation.controller = TCB_Rotation()
		obj.controller.scale.controller = TCB_Scale()
	)
)
catch
	cryTools.cryAnim.UI.batchProcess._v.customScriptStatus = "Error: Setting TCB"