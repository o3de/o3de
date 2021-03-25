--###############################################################################
	--// rollout with elements to control the locator
--###############################################################################
rollout poseRO "Pose Manager"
(
	groupbox gbCharacters " Characters " pos:[2,5] width:153 height:43
	editText edCharacter "" pos:[29,22] width:80 height:17
	dropdownlist ddCharacter "" pos:[32,20] width:94 height:10
	button btnCreateCharacter "" pos:[8,20] width:21 height:21 images:#((getDir #maxroot + "UI\\Icons\\ParameterCollector_i.bmp"), undefined, 28,6,6,20,20) width:29 height:29 toolTip:"Creates a new Character"
	button btnDeleteCharacter "" pos:[128,20] width:21 height:21 images:#((getDir #maxroot + "UI\\Icons\\ParameterCollector_i.bmp"), undefined, 28,8,8,17,17) width:29 height:29	toolTip:"Deletes selected Character"
	
	
	groupbox gbCollections " Collections " pos:[2,55] width:153 height:43
	editText edCollection "" pos:[29,72] width:80 height:17
	dropdownlist ddCollection "" pos:[32,70] width:94 height:10
	button btnCreateCollection "" pos:[8,70] width:21 height:21 images:#((getDir #maxroot + "UI\\Icons\\ParameterCollector_i.bmp"), undefined, 28,6,6,20,20) width:29 height:29 toolTip:"Creates a new Collection"
	button btnDeleteCollection "" pos:[128,70] width:21 height:21 images:#((getDir #maxroot + "UI\\Icons\\ParameterCollector_i.bmp"), undefined, 28,8,8,17,17) width:29 height:29	toolTip:"Deletes selected Collection"
	
	
	groupbox gbPoses " Poses " pos:[2,105] width:153 height:63
	editText edPose "" pos:[29,122] width:80 height:17
	dropdownlist ddPose "" pos:[32,120] width:94 height:20
	button btnCreatePose "" pos:[8,120] width:21 height:21 images:#((getDir #maxroot + "UI\\Icons\\ParameterCollector_i.bmp"), undefined, 28,6,6,20,20) width:29 height:29 toolTip:"Creates a new Pose"
	button btnDeletePose "" pos:[128,120] width:21 height:21 images:#((getDir #maxroot + "UI\\Icons\\ParameterCollector_i.bmp"), undefined, 28,8,8,17,17) width:29 height:29	toolTip:"Deletes selected Pose"
	button btnPastePose "Paste" pos:[38,145] width:80 height:20
	
	
	groupbox gbPaste " Paste " pos:[2,185] width:153 height:75
	checkbox chkPastePosition "Position" pos:[12,200] checked:true
	checkbox chkPasteRotation "Rotation" pos:[12,220] checked:true
	checkbox chkControllerValue "Controller Value" pos:[12,240] checked:true
	checkbox chkBoneOnly "Nodes Only" pos:[76,200] width:75
	checkbox chkByVelocity "By Velocity" pos:[76,220]
	
	
	groupbox gbPreview " Preview " pos:[2,273] width:153 height:150
	bitmap bmpPreview "" pos:[7,289] width:144 height:110
	checkbox chkNodesOnly "Only Nodes visible" pos:[12,403] checked:true
	
	groupbox gbNodes " Nodes " pos:[2,435] width:153 height:169
	multilistbox lbNodes "" pos:[7,455] width:143 height:9
	button btnAddNode "Add" pos:[7,578] width:72 height:20 toolText:"Adds new node to the list"
	button btnDeleteNode "Delete" pos:[79,578] width:71 height:20 toolText:"Deletes selected node/s"
	
	
	
	

	
	
	
	on poseRO open do
	(
		try
		(
			try ( if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then (cryTools.cryAnim.UI.main._f.getUI "Pose Manager" "").open = cryTools.cryAnim.base.iniFile #get #poseRO) catch()
			
			cryTools.cryAnim.UI.main.poseManager._f.inputINI()
			cryTools.cryAnim.UI.main.poseManager._f.updateList #character initial:true
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.open" )
	)
	
	
	on poseRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #poseRO) != value then
				cryTools.cryAnim.base.iniFile #set #poseRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.rolledUp" )
	)
	
	
	
	
	on ddCharacter selected value do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.updateList #character
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.ddCharacter.selected" )
	)
	
	
	on ddCollection selected value do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.updateList #collection
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.ddCollection.selected" )
	)
	
	
	on ddPose selected value do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.updateList #default
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.ddPose.selected" )
	)
	
	
	on btnCreateCharacter pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.createEntry "Character"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnCreateCharacter.pressed" )
	)
	
	
	on btnDeleteCharacter pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.deleteEntry "Character"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnDeleteCharacter.pressed" )
	)
	
	on btnCreateCollection pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.createEntry "Collection"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnCreateCollection.pressed" )
	)
	
	on btnDeleteCollection pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.deleteEntry "Collection"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnDeleteCollection.pressed" )
	)
	
	on btnCreatePose pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.createEntry "Pose"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnCreatePose.pressed" )
	)
	
	on btnDeletePose pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main.poseManager._f.deleteEntry "Pose"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnDeletePose.pressed" )
	)
	
	on edCharacter changed value do
	(
		try
		(
			if ddCharacter.items.count > 0 then
			(
				cryTools.cryAnim.UI.main.poseManager._f.renameEntry "Character"
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.edCharacter.changed" )
	)
	
	
	
	on edCollection changed value do
	(
		try
		(
			if ddCollection.items.count > 0 then
			(
				cryTools.cryAnim.UI.main.poseManager._f.renameEntry "Collection"
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.edCollection.changed" )
	)
	
	
	
	on edPose changed value do
	(
		try
		(
			if ddPose.items.count > 0 then
			(
				cryTools.cryAnim.UI.main.poseManager._f.renameEntry "Pose"
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.edPose.changed" )
	)
	
	
	
	on btnPastePose pressed do
	(
		try
		(
			if ddPose.items.count > 0 then
			(
				undo "Paste Pose" on
					cryTools.cryAnim.UI.main.poseManager._f.applyPose()
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnPastePose.pressed" )
	)
	
	
	
	on btnAddNode pressed do
	(
		try
		(
			if ddCharacter.items.count > 0 then
			(
				if selection.count == 0 then
					local tempVar = selectByName title:"Add Character Nodes" showHidden:true single:false --filter:filterNodes --// somehow crashes max...
				else
					local tempVar = getCurrentSelection()
				
				if classOf tempVar == Array and tempVar.count > 0 then
				(	
					local tempArray = cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].nodes
					for i = 1 to tempVar.count do
						if (findItem tempArray tempVar[i].name) == 0 then
							append tempArray tempVar[i].name
					
					tempArray = cryTools.sortRootChildren tempArray
					
					lbNodes.items = tempArray
					cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].nodes = tempArray
					
					cryTools.cryAnim.UI.main.poseManager._f.outputINI()
				)
				else
					messageBox "No nodes selected." title:"Error Adding Nodes"
				
			)
			else
				messageBox "No Character in List   " title:"Error adding Nodes"
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnAddNode.pressed" )
	)
	
	
	
	on btnDeleteNode pressed do
	(
		try
		(
			if lbNodes.items.count > 0 then
			(
				local poses = cryTools.cryAnim.UI.main.poseManager._v.poses
				local saveSelection = lbNodes.selection as Array
				local newSelection = (lbNodes.selection as Array)[1]
				
				lbNodes.selection = #{newSelection}
				if newSelection == lbNodes.items.count then lbNodes.selection = #{lbNodes.items.count - 1}
				if newSelection == 0 then lbNodes.selection = #{1}
				
				local newList = #()
				local tempArray = cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].nodes
				for i = 1 to tempArray.count do
					if (findItem saveSelection i) == 0 then
						append newList tempArray[i]
				
				if newList.count > 1 then
					newList = cryTools.sortRootChildren newList
				
				
				
				lbNodes.items = newList
				cryTools.cryAnim.UI.main.poseManager._v.poses[ddCharacter.selection].nodes = newList
				
				cryTools.cryAnim.UI.main.poseManager._f.outputINI()
			)
		)
		catch ( logOutput "!!> Error in cryAnim.UI.main.dialog.poseRO.btnDeleteNode.pressed" )
	)
)
logOutput "> Created poseRO rollout"


try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row3 poseRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 poseRO rolledUp:true
)
catch ( logOutput "!!> Error adding poseRO to main dialog" )
poseRO = undefined


logOutput ">> pose.ms loaded"