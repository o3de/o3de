max utility mode
UtilityPanel.OpenUtility CryEngine_2_Exporter
for obj in selection do
(
	ExprtArr = csexport.export.get_node_list()
	for x = 1 to ExprtArr.count do
	(
		if ExprtArr[x] == obj then deleteItem ExprtArr x
	)
	append ExprtArr obj
	csexport.export.set_node_list ExprtArr
)
max modify mode