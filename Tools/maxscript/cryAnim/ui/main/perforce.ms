--###############################################################################
	--// rollout with all the file control _f like Load Biped File, Save, Export etc.
--###############################################################################
rollout perforceRO "Perforce"
(
	timer clock interval:0 active:false

	checkbox chkAutoUpdate "AutoUpdate" pos:[8,8] checked:true
	spinner spnAutoUpdateTime "Time " pos:[104,8] range:[0,999,0] width:50 scale:1 type:#integer
	
	groupBox grpOpen " Open For Edit " pos:[2,33] width:153 height:120
	
	label labOpenY "Yes" pos:[82,53]
	label labOpenN "No" pos:[107,53]
	label labOpenA "Ask" pos:[129,53]
	
	label labOpenLoad "Load" pos:[14,68]
	label labOpenSave "Save" pos:[14,88]
	label labOpenExport "Export" pos:[14,108]
	label labOpenSaveExport "Save/Export" pos:[14,128]
	
	radiobuttons radLoadOpen pos:[85,68] labels:#("","","") columns:3 default:3
	radiobuttons radSaveOpen pos:[85,88] labels:#("","","") columns:3 default:3
	radiobuttons radExportOpen pos:[85,108] labels:#("","","") columns:3 default:3
	radiobuttons radSaveExportOpen pos:[85,128] labels:#("","","") columns:3 default:3

	groupBox grpAdd " Add to Source Control " pos:[2,163] width:153 height:100
	
	label labAddY "Yes" pos:[82,183]
	label labAddN "No" pos:[107,183]
	label labAddA "Ask" pos:[129,183]

	label labAddSave "Save" pos:[14,198]
	label labAddExport "Export" pos:[14,218]
	label labAddSaveExport "Save/Export" pos:[14,238]

	radiobuttons radSaveAdd pos:[85,198] labels:#("","","") columns:3 default:3
	radiobuttons radExportAdd pos:[85,218] labels:#("","","") columns:3 default:3
	radiobuttons radSaveExportAdd pos:[85,238] labels:#("","","") columns:3 default:3
	
	

	on perforceRO open do
	(
		try
		(
			try ( if (cryTools.cryAnim.base.iniFile #get #rolloutStates) == true then (cryTools.cryAnim.UI.main._f.getUI "Perforce" "").open = cryTools.cryAnim.base.iniFile #get #perforceRO) catch()
			
			try chkAutoUpdate.checked = cryTools.cryAnim.base.iniFile #get #autoUpdate catch()
			spnAutoUpdateTime.enabled = chkAutoUpdate.checked
			try spnAutoUpdateTime.value = cryTools.cryAnim.base.iniFile #get #autoUpdateTime catch()
			clock.interval = spnAutoUpdateTime.value * 60000
		
			try radLoadOpen.state = cryTools.cryAnim.base.iniFile #get #loadOpen catch()
			try radSaveOpen.state = cryTools.cryAnim.base.iniFile #get #saveOpen catch()
			try radExportOpen.state = cryTools.cryAnim.base.iniFile #get #exportOpen catch()
			try radSaveExportOpen.state = cryTools.cryAnim.base.iniFile #get #saveExportOpen catch()

			try radSaveAdd.state = cryTools.cryAnim.base.iniFile #get #saveAdd catch()
			try radExportAdd.state = cryTools.cryAnim.base.iniFile #get #exportAdd catch()
			try radSaveExportAdd.state = cryTools.cryAnim.base.iniFile #get #saveExportAdd catch()
			
			clock.active = chkAutoUpdate.checked
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.perforceRO.open" )
	)
	
	
	on perforceRO rolledUp value do
	(
		try
		(
			if (cryTools.cryAnim.base.iniFile #get #perforceRO) != value then
				cryTools.cryAnim.base.iniFile #set #perforceRO
			
			cryTools.cryAnim.UI.main._f.updateDialog()
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.perforceRO.rolledUp" )
	)
	
	
	on chkAutoUpdate changed value do
	(
		try
		(
			clock.active = value
			spnAutoUpdateTime.enabled = value
			cryTools.cryAnim.base.iniFile #set #autoUpdate
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.perforceRO.chkAutoUpdate.changed" )
	)
	
	on clock tick do
	(
		try
		(
			if spnAutoUpdateTime.value > 0 then
				if (local tempVar = cryTools.cryAnim.base.perforce "" #checkLoading) != false then
					if cryTools.suppressWarnings == false then
						cryTools.cryAnim.base.perforce tempVar #messageOnUpdate
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.perforceRO.clock" )
	)
	

	
	on spnAutoUpdateTime changed value do
	(
		try
		(
			clock.interval = value * 60000
			cryTools.cryAnim.base.iniFile #set #autoUpdateTime
		)
		catch ( logOutput "!!> Error in cryTools.cryAnim.UI.main.dialog.perforceRO.spnAutoUpdateTime.changed" )
	)
	


	on radLoadOpen changed value do
		cryTools.cryAnim.base.iniFile #set #loadOpen
		
	on radSaveOpen changed value do
		cryTools.cryAnim.base.iniFile #set #saveOpen
		
	on radExportOpen changed value do
		cryTools.cryAnim.base.iniFile #set #exportOpen
		
	on radSaveExportOpen changed value do
		cryTools.cryAnim.base.iniFile #set #saveExportOpen
	
	on radSaveAdd changed value do
		cryTools.cryAnim.base.iniFile #set #saveAdd
		
	on radExportAdd changed value do
		cryTools.cryAnim.base.iniFile #set #exportAdd
		
	on radSaveExportAdd changed value do
		cryTools.cryAnim.base.iniFile #set #saveExportAdd	
)
logOutput "> Created perforceRO rollout"


try
(
	if cryTools.cryAnim.base.iniFile #get #multiRow == true then
		addSubRollout cryTools.cryAnim.UI.main.dialog.row4 perforceRO
	else
		addSubRollout cryTools.cryAnim.UI.main.dialog.row1 perforceRO rolledUp:true
)
catch ( logOutput "!!> Error adding perforceRO to main dialog" )
perforceRO = undefined


logOutput ">> perforce.ms loaded"