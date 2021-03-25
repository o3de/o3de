--###############################################################################
	--// rollout to control several operations
--###############################################################################
rollout operationRO "Operation"
(
	
	button btnStart "Start" pos:[8,8] width:45 height:20 toolTip:"Sets the start time of process"
	button btnStop "Stop" pos:[60,8] width:45 height:20 toolTip:"Sets the end time of process"
	spinner spnBegin "" pos:[6,35] range:[0,9999,0] type:#integer fieldWidth:35 scale:1
	spinner spnEnd "" pos:[58,35] range:[0,9999,0] type:#integer fieldWidth:35 scale:1
	spinner spnSteps "" pos:[112,35] range:[0,50,1] type:#integer fieldWidth:25 scale:1
	label labSteps "Steps" pos:[115,12]
	
	--checkbox chkOnlyExistingKeys "Only existing Keys" pos:[8,60] enabled:false
	label labSetRange "Set Range:" pos:[6,60]
	button btnSetRangeSelection "Selection" pos:[70,57] height:20 width:50
	button btnSetRangeAll "All" pos:[123,57] height:20 width:30
					
	groupBox gbOperation "Operation" pos:[2,80] width:153 height:77
	dropDownList ddOperation "" pos:[8,99] width:142 height:21
	label labOperation "To :" pos:[15,124] visible:false
	label labDistanceDirection "" pos:[15,124] visible:false align:#left
	dropDownList ddOperationTo "" pos:[8,140] width:142 height:21 visible:false
	spinner spnDistance "" pos:[8,140] width:142 height:21 range:[-1000,1000,0] visible:false
	spinner spnDirection "" pos:[8,140] width:142 height:21 range:[-360,360,180] visible:false
	
	button btnApply "Apply" pos:[8,129] width:33 height:20 toolTip:"Applies the Operation once"
	button btnApplyBeginEnd "Start/Stop" pos:[46,129] width:59 height:20 toolTip:"Applies the Operation for the selected range"
	button btnApplyRange "Range" pos:[110,129] width:39 height:20 toolTip:"Applies the Operation for the animation range"
	
	
	




	
	on operationRO open do
	(
		try
		(
			try ( if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then (cryTools.cryAnim.UI.main._f.getUI "Operation" "").open = cryTools.cryAnim.base.iniFile #get #operationRO) catch()


			operationRO.height = 161

			ddOperation.items = cryTools.cryAnim.UI.main.operation._f.updateDialog output:#ddOp
			ddOperationTo.items = cryTools.cryAnim.UI.main.operation._f.updateDialog output:#ddOpTo
			spnBegin.value = animationRange.start.frame
			spnEnd.value = animationRange.end.frame
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.open" )
	)
	
	
	on operationRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #operationRO) != value then
				cryTools.cryAnim.base.iniFile #set #operationRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.rolledUp" )
	)
	
	
	on chkOnlyExistingKeys changed value do
	(
		try
		(
			spnSteps.enabled = not value
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.chkOnlyExistingKeys.changed" )
	)
	
	
	
	on btnStart pressed do
	(
		try
		(
			spnBegin.value = sliderTime
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.btnStart.pressed" )
	)
	
	on btnStop pressed do
	(
		try
		(
			spnEnd.value = sliderTime
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.btnStop.pressed" )
	)


	
	on spnSteps changed value do
	(
		try
		(
			local diffStartEnd = spnEnd.value - spnBegin.value
			if diffStartEnd < 0 then
				diffStartEnd -= (diffStartEnd * 2)
				
			if value > diffStartEnd then
				spnSteps.value = diffStartEnd
			if spnSteps.value == 0 then spnSteps.value = 1
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.spnSteps.changed" )
	)
	
	
	on btnSetRangeSelection pressed do
	(
		if selection.count > 0 then
		(
			local firstKey = 0
			local lastKey = 0
			for obj in selection do
			(
				if obj.classID[1] == 37157 then
				(
					try
					(
						local tempFirstKey = obj.transform.controller.keys[1].time.frame
						if tempFirstKey < firstKey then
							firstKey = tempFirstKey
					)catch()
					
					try
					(
						local tempLastkey = obj.transform.controller.keys[obj.transform.controller.keys.count].time.frame
						if tempLastKey > lastKey then
							lastKey = tempLastKey
					)catch()
					
				)
				else
				(
					
					for i = 1 to 3 do
					(
						try
						(
							
							local tempFirstKey = obj.controller[i].keys[1].time.frame
							if tempFirstKey < firstKey then
								firstKey = tempFirstKey
						)
						catch()
						
						
						try
						(
							local tempLastkey = obj.controller[i].keys[obj.controller[i].keys.count].time.frame
							if tempLastKey > lastKey then
								lastKey = tempLastKey
						)
						catch()
					)
				)
			)
			
			if firstKey != lastKey then
				animationRange = interval firstKey lastKey
			else
				print "Error setting new Range"
		)
		else
			print "Nothing selected."
		
	)
	
	
	
	on btnSetRangeAll pressed do
	(
		local firstKey = 0
		local lastKey = 0
		for obj in Objects do
		(
			if obj.classID[1] == 37157 then
			(
				try
				(
					local tempFirstKey = obj.transform.controller.keys[1].time.frame
					if tempFirstKey < firstKey then
						firstKey = tempFirstKey
				)catch()
				
				try
				(
					local tempLastkey = obj.transform.controller.keys[obj.transform.controller.keys.count].time.frame
					if tempLastKey > lastKey then
						lastKey = tempLastKey
				)catch()
				
			)
			else
			(
				
				for i = 1 to 3 do
				(
					try
					(
						
						local tempFirstKey = obj.controller[i].keys[1].time.frame
						if tempFirstKey < firstKey then
							firstKey = tempFirstKey
					)
					catch()
					
					
					try
					(
						local tempLastkey = obj.controller[i].keys[obj.controller[i].keys.count].time.frame
						if tempLastKey > lastKey then
							lastKey = tempLastKey
					)
					catch()
				)
			)
		)
			
		if firstKey != lastKey then
			animationRange = interval firstKey lastKey
		else
				print "Error setting new Range"
		
	)
	
	
	
	on ddOperation selected value do
	(
		try
		(
			case value of
			(
				4: ddOperation.selection -= 1
				7: ddOperation.selection -= 1
				11: ddOperation.selection -= 1
			)
			cryTools.cryAnim.UI.main.operation._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.ddOperation.selected" )
	)
	
	
					
		
	on btnApply pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main.operation._f.applyOperation ddOperation.selection ddOperationTo.selection sliderTime.frame sliderTime.frame 1	
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.btnApply.pressed" )
	)
	
	on btnApplyBeginEnd pressed do
	(
		try
		(
			if spnBegin.value == spnEnd.value then
			(
				messageBox ("'Start' must be unequal to 'End'") title:"Begin/End Operation"
				return false
			)
			
			if spnBegin.value > spnEnd.value then
			(
				tempValue = spnBegin.value
				spnBegin.value = spnEnd.value
				spnEnd.value = tempValue
			)
			
			local diffBeginEnd = spnEnd.value - spnBegin.value
			if diffBeginEnd < 0 then
				diffBeginEnd -= (diffBeginEnd * 2)
			
			if spnSteps.value > diffBeginEnd then
				spnSteps.value = diffBeginEnd
			
			undo "BeginEndOperation" on
			(
				local tempAnimationRange = animationRange
				animationRange = interval spnBegin.value spnEnd.value
				
				local tempBool = cryTools.cryAnim.UI.main.operation._f.applyOperation ddOperation.selection ddOperationTo.selection spnBegin.value spnEnd.value spnSteps.value
				if tempBool != false then
					cryTools.cryAnim.UI.main.operation._f.applyOperation ddOperation.selection ddOperationTo.selection spnEnd.value spnEnd.value 1								

				animationRange = tempAnimationRange
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.btnApplyBeginEnd.pressed" )
	)
	
	
	on btnApplyRange pressed do
	(
		try
		(
			undo "RangeOperation" on
			(
				tempBool = cryTools.cryAnim.UI.main.operation._f.applyOperation ddOperation.selection ddOperationTo.selection animationRange.start.frame animationRange.end.frame spnSteps.value
				if tempBool != false then
					cryTools.cryAnim.UI.main.operation._f.applyOperation ddOperation.selection ddOperationTo.selection animationRange.end.frame animationRange.end.frame 1
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.operationRO.btnApplyRange.pressed" )
	)				
)
logOutput "> Created operationRO rollout"


try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 operationRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 operationRO
)
catch ( logOutput "!!> Error adding operationRO to main dialog" )
operationRO = undefined


logOutput ">> operation.ms loaded"