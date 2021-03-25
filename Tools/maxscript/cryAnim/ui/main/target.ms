--###############################################################################
	--// rollout with the selection list and what axes to use or if there is an offset
--###############################################################################
rollout targetRO "Target"
(
	dropDownList ddSelection "" pos:[8,8] width:144 height:21
	
	groupBox grpAxis "Used Axis" pos:[2,37] enabled:false width:153 height:85 --85 -- 107 -- 125
	label labX "X" pos:[38,57] enabled:false
	label labY "Y" pos:[65,57] enabled:false
	label labZ "Z" pos:[92,57] enabled:false
	label labOffset "Offset" pos:[117,57] enabled:false
	label labPos "Pos" pos:[10,75] enabled:false
	checkbox chkX pos:[36,75] height:16 enabled:false
	checkbox chkY pos:[63,75] height:16 enabled:false
	checkbox chkZ pos:[90,75] height:16 enabled:false
	checkbox chkPosOffset pos:[117,75] height:16 width:15 enabled:false
	
	label labRot "Rot" pos:[10,98] enabled:false
	checkbox chkXRot pos:[36,98] height:16 enabled:false
	checkbox chkYRot pos:[63,98] height:16 enabled:false
	checkbox chkZRot pos:[90,98] height:16 enabled:false
	checkbox chkRotOffset pos:[117,98] height:16 width:15 enabled:false

	button btnSetOffset "S" pos:[137,76] height:37 width:15 enabled:false toolTip:"Retrieve the offset of selected node and selection in the list"
	
	label labEditOffset1 "1" pos:[8,122] height:20 width:40 visible:false
	label labEditOffset2 "2" pos:[8,140] height:20 width:40 visible:false
	edittext edOffset1 "" pos:[30,120] fieldWidth:118 visible:false
	edittext edOffset2 "" pos:[30,138] fieldWidth:118 visible:false

	timer clock interval:10000 active:true

	label labChange "" pos:[0,0] height:1 enabled:false
	
	
	
	on targetRO close do
	(
		try
		(
			if cryTools.cryAnim._v.various[10] == undefined then
				cryTools.cryAnim.base.iniFile #set #visible value:"0"
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.close" )
	)
	
	on targetRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #targetRO) != value then
				cryTools.cryAnim.base.iniFile #set #targetRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.rolledUp" )
	)

	
	on targetRO open do
	(
		try
		(
			try ( if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then (cryTools.cryAnim.UI.main._f.getUI "Target" "").open = cryTools.cryAnim.base.iniFile #get #targetRO) catch()
			(cryTools.cryAnim.UI.main._f.getUI "Target" "").height =128
			
			cryTools.cryAnim.UI.main.target._f.updateDDSelection()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.open" )
	)				
	
	on ddSelection selected value do
	(
		try
		(
			groupArray = #(\
				grpAxis, labX, labY, labZ, labOffset, \
				labPos, chkX, chkY, chkZ, chkPosOffset, \
				labRot, chkXRot, chkYRot, chkZRot, chkRotOffset )
		
				
			if ddSelection.selection <= 3 then
			(
				cryTools.cryAnim.UI.main.target._f.updateDDSelection op:false
				ddSelection.selection = 1
				
				if (findString labChange.text "1") == undefined then
					labChange.text = "1:Changed"
				else
					labChange.text = "1:Unchanged"


			)
			else
			(
				cryTools.cryAnim.UI.main.target._f.updateDDSelection op:true
				
				if (findString labChange.text "2") == undefined then
					labChange.text = "2:Changed"
				else
					labChange.text = "2:Unchanged"
			)
			
			cryTools.cryAnim.UI.main.operation._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.ddSelection.selected" )
	)
	
	
	
	

	on btnSetOffset pressed do
	(
		try
		(
			if $selection.count > 0 then
			(	
				cryTools.cryAnim._v.various[35] = #()
				
				local tempSnap = cryTools.cryAnim._f.createSnapshot()
				for obj in tempSnap do
				(
					in coordsys (getNodeByName (cryTools.cryAnim.UI.main._f.getUI "Target" "").ddSelection.selected)
						append cryTools.cryAnim._v.various[35] (rotPosStruct rotation:obj.rotation position:obj.pos)					
				)
				for obj in tempSnap do
					delete obj
				
				cryTools.cryAnim.UI.main.target._f.updateOffset()
				
			)
			else
				messageBox "No Node selected." title:"Set Offset"
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.btnSetOffset.pressed" )
	)
	
	on chkPosOffset changed value do
	(
		try			
			cryTools.cryAnim.UI.main.target._f.updateOffset()
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.chkPosOffset.changed" )
	)
	
	on chkRotOffset changed value do
	(
		try
			cryTools.cryAnim.UI.main.target._f.updateOffset()
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.chkRotOffset.changed" )
	)
	
	
	
	
	
	on edOffset1 entered value do
	(
		try
			setManualOffset labEditOffset1 edOffset1
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.edOffset1.entered" )
	)
	
	on edOffset2 entered value do
	(
		try
			setManualOffset labEditOffset2 edOffset2
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.targetRO.edOffset2.entered" )
	)
)
logOutput "> Created targetRO rollout"


try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 targetRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 targetRO
)
catch ( logOutput "!!> Error adding targetRO to main dialog" )
targetRO = undefined


logOutput ">> target.ms loaded"