-------------------------------------------------------------------------------
--  cryInfoFloater.ms
--	Version 1.2
--	by Christopher Evans
-------------------------------------------------------------------------------

if cryInfoFloater != undefined do ( destroydialog cryInfoFloater )

rollout cryInfoFloater "cryInfo 1.4"
	(
		edittext cryInfo_txt text:"cryInfo" fieldWidth:390 height:260 pos:[1,3]
		button saveinfo_txt "Save Updated Info" pos:[3,269]
		button runEmbed "Run Embedded Scripts" pos:[115,269] enabled:false
		
		--	on cryinfo open
		on cryInfoFloater open do
		(
		
		if $cryInfo != undefined then
			(
			cryInfo_txt.text = (getUserPropBuffer $cryInfo)
			)
			
		if $cryInfo == undefined then
			(
			cryInfo_txt.text = "There is no cryInfo node present.\nType info in here and press \"Save Updated Info\" to save cryInfo in this file."
			)
		if $cryEmbed == undefined then
			(
			runEmbed.enabled = false
			)
		else
			(
			runEmbed.enabled = true
			)
		)
		
		--	on save info pressed
		on saveinfo_txt pressed do
		(
		
			if $cryInfo == undefined then
			(
			dummy name:"cryInfo" pos:[0,0,0] boxsize:[1,1,1]
			)
		
		setUserPropBuffer $cryInfo cryInfo_txt.text
		
		)
		
		--	on runembed pressed
		on runEmbed pressed do
		(
		runme = (getUserPropBuffer $cryEmbed)
		execute runme
		)
		
		--	on resized do
		on cryInfoFloater resized size do
		(
			size1 = size as string
			size2 = filterstring size1 "[],"
			cryInfo_txt.width = ((size2[1] as float) - 10)
			cryInfo_txt.height = ((size2[2] as float) - 35)
			saveinfo_txt.pos = [4, (cryInfoFloater.height - 26)]
		)
	)
	createDialog cryInfoFloater 400 295 bgcolor:black fgcolor:white style:#(#style_resizing, #style_titlebar, #style_border, #style_sysmenu)