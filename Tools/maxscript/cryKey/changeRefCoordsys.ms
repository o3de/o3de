case getRefCoordSys() of
	(
	#local: setRefCoordSys #hybrid
	#hybrid: setRefCoordSys #screen
	#screen: setRefCoordSys #world
	#world: setRefCoordSys #parent
	#parent: setRefCoordSys #local
	)