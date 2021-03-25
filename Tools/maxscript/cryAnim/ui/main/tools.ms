--###############################################################################
	--// rollout with internal control _f like Promt for File when saving, exporting, save+export, or reloading the whole script as well as the perforce control
--###############################################################################
rollout settingsRO "Settings"
(
	button btnReload "Reload CryAnim" pos:[8,8] width:142 height:20 toolTip:"Reloads the whole cryAnim script and dialog"
	
	groupBox grpLoadingDialog " Promt for File " pos:[2,40] width:153 height:100
	
	label labPromptN "Yes" pos:[105,60]
	label labPromptA "No" pos:[129,60]

	label labPromptSave "Save" pos:[14,75]
	label labPromptExport "Export" pos:[14,95]
	label labPromptSaveExport "Save/Export" pos:[14,115]

	radiobuttons radSavePrompt pos:[108,75] labels:#("","") columns:2 default:1
	radiobuttons radExportPrompt pos:[108,95] labels:#("","") columns:2 default:1
	radiobuttons radSaveExportPrompt pos:[108,115] labels:#("","") columns:2 default:1


	label labWorkingDir "Biped Working Directory :" pos:[10,150]
	edittext edWorkingDir "" pos:[4,170] fieldWidth:140
	button btnPickWorkingDir "Pick" pos:[8,190] height:20 width:65 toolTip:"Get the path for the biped working directory"
	button btnSetWorkingDir "Set" pos:[85,190] height:20 width:65 toolTip:"Set the path as biped working directory"
	
	checkbox chkMultiRow "Multi Row Dialog" pos:[10,230] checked:true
	checkbox chkRolloutStates "Use Rollout States" pos:[10,250] checked:false
	checkbox chkReadOnly "Notify File Attribute" pos:[10,270] checked:true
	
	--button btnCustomizeRollouts "Customize Rollouts" pos:[8,290] width:142 height:20
	
	
	
	on settingsRO open do
	(
		try
		(
			try radLoadOpen.state = cryTools.cryAnim.base.iniFile #get #loadOpen catch()
			try radSaveOpen.state = cryTools.cryAnim.base.iniFile #get #saveOpen catch()
			try radExportOpen.state = cryTools.cryAnim.base.iniFile #get #exportOpen catch()
			try radSaveExportOpen.state = cryTools.cryAnim.base.iniFile #get #saveExportOpen catch()

			try radSaveAdd.state = cryTools.cryAnim.base.iniFile #get #saveAdd catch()
			try radExportAdd.state = cryTools.cryAnim.base.iniFile #get #exportAdd catch()
			try radSaveExportAdd.state = cryTools.cryAnim.base.iniFile #get #saveExportAdd catch()
			
			try radSavePrompt.state = cryTools.cryAnim.base.iniFile #get #savePrompt catch()
			try radExportPrompt.state = cryTools.cryAnim.base.iniFile #get #exportPrompt catch()
			try radSaveExportPrompt.state = cryTools.cryAnim.base.iniFile #get #saveExportPrompt catch()
			try edWorkingDir.text = cryTools.cryAnim.base.iniFile #get #workingDir catch()
			if edWorkingDir.text == "" then
				edWorkingDir.text = cryTools.buildPathFull + "Game\\Animations\\"
				
			cryTools.cryAnim.UI.main._v.bipWorkingDir = edWorkingDir.text
			cryTools.cryAnim.UI.main._v.cafWorkingDir = subString crytools.cbapath 1 (crytools.cbapath.count - 14)
			
			if cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath == "" then
				cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath = cryTools.cryAnim.UI.main._v.bipWorkingDir + "*.bip"
			
			cryTools.cryAnim.UI.main.loadSave._v.bipSavePath = cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath
			cryTools.cryAnim.UI.main.loadSave._v.cafSavePath = cryTools.cryAnim.UI.main._v.cafWorkingDir + "*.caf"
			
			try local multiRow = cryTools.cryAnim.base.iniFile #get #multiRow catch()
			if multiRow != "" then chkMultiRow.checked = multiRow
				
			try chkRolloutStates.checked = cryTools.cryAnim.base.iniFile #get #rolloutStates catch()
			try chkReadOnly.checked = cryTools.cryAnim.base.iniFile #get #readOnly catch()

			
			if chkRolloutStates.checked == true then
				try ( (cryTools.cryAnim.UI.main._f.getUI "Tools" "").open = cryTools.cryAnim.base.iniFile #get #settingsRO ) catch()

			
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.open" )
	)
	
	
	on settingsRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #settingsRO) != value then
				cryTools.cryAnim.base.iniFile #set #settingsRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.rolledUp" )
	)
	
	
	on btnReload pressed do
	(
		try
			cryTools.cryAnim.base.reloadScript()
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.btnReload.pressed" )
	)				
	
	on btnPickWorkingDir pressed do
	(
		try
		(
			if cryTools.cryAnim.UI.main._v.bipWorkingDir == "" then				
				local tempVar = (getSavePath caption:"Project Directory" initialDir:crytools.BuildPathFull)
			else
				local tempVar = (getSavePath caption:"Project Directory" initialDir:cryTools.cryAnim.UI.main._v.bipWorkingDir)

			if tempVar != undefined then
			(
				if tempVar[tempVar.count] != "\\" then
					append tempVar "\\"
		
				edWorkingDir.text = tempVar			
			)
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.btnPickWorkingDir.pressed" )
	)
	
	on btnSetWorkingDir pressed do
	(
		try
		(
			cryTools.cryAnim.UI.main._v.bipWorkingDir = edWorkingDir.text
			cryTools.cryAnim.UI.main.loadSave._v.bipOpenPath = edWorkingDir.text + ".bip"
			cryTools.cryAnim.base.iniFile #set #workingDir
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.btnSetWorkingDir.pressed" )
	)

	on chkMultiRow changed value do
	(
		try
		(
			cryTools.cryAnim.base.iniFile #set #multiRow
			cryTools.cryAnim.UI.main._f.callDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.chkMultiRow.changed" )
	)
	
	on chkRolloutStates changed value do
	(
		try
			cryTools.cryAnim.base.iniFile #set #rolloutStates
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.chkRolloutStates.changed" )
	)
	
	on chkReadOnly changed value do
	(
		try			
			cryTools.cryAnim.base.iniFile #set #readOnly
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.chkReadOnly.changed" )
	)

	
	
	

	on radSavePrompt changed value do
	(
		try
			cryTools.cryAnim.base.iniFile #set #savePrompt
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.radSavePromt.changed" )
	)
		
	on radExportPrompt changed value do
	(
		try
			cryTools.cryAnim.base.iniFile #set #exportPrompt
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.radExportPrompt.changed" )
	)
		
	on radSaveExportPrompt changed value do
	(
		try
			cryTools.cryAnim.base.iniFile #set #saveExportPrompt
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.settingsRO.radSaveExportPrompt.changed" )
	)
	
	
	on btnCustomizeRollouts pressed do
	(
		cryTools.cryAnim.UI.main.settings._f.callCustomizeDialog()
	)

	
)
logOutput "> Created settingsRO rollout"


try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row4 settingsRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 settingsRO rolledUp:true
)
catch ( logOutput "!!> Error adding settingsRO to main dialog" )
settingsRO = undefined


logOutput ">> tools.ms loaded"