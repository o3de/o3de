--###############################################################################
	--// rollout with elements to control the pivot align _f
--###############################################################################
rollout pivotRO "Pivot"
(	
	button btnPivotSelect "Select Pivot" pos:[8,8] width:142 height:20 toolTip:"Creates pivot point selection tool"
	groupBox grpPivPos "Pivot Point" pos:[2,38] width:153 height:50
	button btnCreatePivPoint "Create" pos:[8,60] width:65 height:20 toolTip:"Creates pivot point on the selected node"
	button btnDeletePivPoint "Delete" pos:[83,60] width:65 height:20 toolTip:"Deletes pivot point of the selected node"
	
	groupBox gbSnapshot " Snapshot " pos:[2,100] width:153 height:50
	
	button btnSingleSnapshot "Single" pos:[8,123] width:66 height:20 toolTip:"Creates snapshot of the current selection"
	button btnChildrenSnapshot "+ Children" pos:[84,123] width:66 height:20 toolTip:"Creates grouped snapshot of the current selection with children"
	
	
	on pivotRO open do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then 
				(cryTools.cryAnim.UI.main._f.getUI "Pivot" "").open = cryTools.cryAnim.base.iniFile #get #pivotRO
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.pivotRO.open" )
	)

	
	
	on pivotRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #pivotRO) != value then
				cryTools.cryAnim.base.iniFile #set #pivotRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.pivotRO.rolledUp" )
	)
	
	
		
	on btnPivotSelect pressed do
	(
		try
		(
			if (local selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart() ) != undefined then
			(
				if selectedBipPart.object.isPivot == true then
				(
					undo off
						cryTools.cryAnim.align._f.callPivotSelect()
				)
				else
					print "Pivot Selection not available for this Object"
			)
			else
			(
				if selection.count > 0 then
					print "No Biped Object selected."
				else
					print "Nothing selected."
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.pivotRO.btnPivotSelect.pressed" )
	)
	
		
	on btnCreatePivPoint pressed do					
	(
		try
		(
			if (selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart() ) != undefined then
			(
				if selectedBipPart.pivotSel.index == undefined then
					messageBox "Select a Pivot first" title:"Pivot Point" beep:false
				else							
					if (cryTools.cryAnim.UI.main.pivot._f.pivotPoint #check) == true then
					(
						if (queryBox "Set new Pivot Point?" title:"Pivot Point" beep:false) == true then
							if (cryTools.cryAnim.UI.main.pivot._f.pivotPoint #delete) == true then
								cryTools.cryAnim.UI.main.pivot._f.pivotPoint #create
					)
					else
						cryTools.cryAnim.UI.main.pivot._f.pivotPoint #create							
			)
			else
			(
				if selection.count > 0 then
					print "No Biped Object selected."
				else
					print "Nothing selected"
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.pivotRO.btnCreatePivPoint.pressed" )
	)
		
	on btnDeletePivPoint pressed do
	(
		try
		(
				if (selectedBipPart = cryTools.cryAnim._f.getSelectedBipPart() ) != undefined then						
					if selectedBipPart.pivotSel.pivPoint.name != undefined then
						if (queryBox "Delete Pivot Point?" title:"Pivot Point" beep:false) == true then
							cryTools.cryAnim.UI.main.pivot._f.pivotPoint #delete
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.pivotRO.btnDeletePivPoint.pressed" )
	)
	
	on btnSingleSnapshot pressed do
	(
		try
		(
			with animate off
			(
				if cryTools.cryAnim._f.createSnapshot() == undefined then
					print "Nothing selected"
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.pivotRO.btnSingleSnapshot.pressed" )
	)
	
	on btnChildrenSnapshot pressed do
	(
		try
		(
			with animate off
			(
				local baseArray = cryTools.cryAnim._f.createSnapshot children:true
				
				if baseArray == undefined then
					print "Nothing selected"
				else
					group baseArray prefix:"Snapshot_"
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.pivotRO.btnChildrenSnapshot.pressed" )	
	)
)
logOutput "> Created pivotRO rollout"



try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 pivotRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 pivotRO
)
catch ( logOutput "!!> Error adding pivotRO to main dialog" )
pivotRO = undefined


logOutput ">> pivot.ms loaded"