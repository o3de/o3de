
try
(
	if cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile].subRanges.count == 0 then
		cryTools.cryAnim.UI.batchProcess.dialog.rollouts[2].btnImportANM.pressed()
	
	
	local listEntries = cryTools.cryAnim.UI.batchProcess._v.exportFiles[cryTools.cryAnim.UI.batchProcess._v.selectedFile]
	local tempStatus = ""
	
	local tempCheckBeforeExport = cryTools.checkbeforeexport
	local tempSuppressWarnings = cryTools.suppresswarnings
	
	cryTools.checkbeforeexport = false
	cryTools.suppresswarnings = true

	UtilityPanel.OpenUtility CryEngine2_Exporter
	
	
	local tempRange = animationRange
	
	if listEntries.subRanges.count == 0 then
		tempStatus = "Error: No Sub-Ranges found"
	else
	(
	
		for i = 1 to listEntries.subRanges.count do
		(
			if listEntries.subRanges[i].range.start.frame == listEntries.subRanges[i].range.end.frame then
			(
				tempStatus = listEntries.subRanges[i].export + " has wrong animation range"
				continue
			)
			else
			(
				if listEntries.subRanges[i].range.start.frame > listEntries.subRanges[i].range.end.frame then
				(
					local tempTime = listEntries.subRanges[i].range.start
					listEntries.subRanges[i].range.start = listEntries.subRanges[i].range.end
					listEntries.subRanges[i].range.end = tempTime
					
					cryTools.cryAnim.UI.batchProcess._f.subRangeUpdateList()
					tempStatus += "Switched Start and Stop of " + listEntries.subRanges[i].export
				)
			)
			
			
			animationRange = listEntries.subRanges[i].range
			
			saveMaxFile (maxFilePath + listEntries.subRangePrefix + "_" + listEntries.subRanges[i].export + ".max") quiet:true
			
			local newObjects = #()
			for f = 1 to listEntries.subRanges[i].objects.count do
			(
				if (local tempNode = getNodeByName listEntries.subRanges[i].objects[f]) != undefined then
					newObjects[f] = tempNode
				else
					tempStatus += "; Can't find " + listEntries.subRanges[i].objects[f]
			)
			
			
			if newObjects.count > 0 then
			(
				csexport.set_node_list newObjects
				csexport.export_nodes()
			)
			else
				tempStatus += "; No Node Found"
			
			deleteFile (maxFilePath + maxFileName)
			
		)
	)
	animationRange = tempRange
	
	cryTools.checkbeforeexport = tempCheckBeforeExport
	cryTools.suppresswarnings = tempSuppressWarnings
	
	cryTools.cryAnim.UI.batchProcess._v.customScriptStatus = tempStatus
)
catch()
