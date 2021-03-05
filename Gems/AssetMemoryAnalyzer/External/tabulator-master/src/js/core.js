'use strict';

/*=include polyfills.js */

/*=include column_manager.js */
/*=include column.js */
/*=include row_manager.js */
/*=include row.js */
/*=include cell.js */
/*=include footer_manager.js */

var Tabulator = function(element, options){

	this.options = {};

	this.columnManager = null; // hold Column Manager
	this.rowManager = null; //hold Row Manager
	this.footerManager = null; //holder Footer Manager
	this.browser = ""; //hold current browser type
	this.browserSlow = false; //handle reduced functionality for slower browsers

	this.modules = {}; //hold all modules bound to this table

	this.initializeElement(element);
	this.initializeOptions(options || {});
	this._create();

	Tabulator.prototype.comms.register(this); //register table for inderdevice communication
};

//default setup options
Tabulator.prototype.defaultOptions = {

	height:false, //height of tabulator

	layout:"fitData", ///layout type "fitColumns" | "fitData"
	layoutColumnsOnNewData:false, //update column widths on setData

	columnMinWidth:40, //minimum global width for a column
	columnVertAlign:"top", //vertical alignment of column headers

	resizableColumns:true, //resizable columns
	resizableRows:false, //resizable rows
	autoResize:true, //auto resize table

	columns:[],//store for colum header info

	data:[], //default starting data

	nestedFieldSeparator:".", //seperatpr for nested data

	tooltips: false, //Tool tip value
	tooltipsHeader: false, //Tool tip for headers
	tooltipGenerationMode:"load", //when to generate tooltips

	initialSort:false, //initial sorting criteria
	initialFilter:false, //initial filtering criteria

	columnHeaderSortMulti: true, //multiple or single column sorting

	sortOrderReverse:false, //reverse internal sort ordering

	footerElement:false, //hold footer element

	index:"id", //filed for row index

	keybindings:[], //array for keybindings

	clipboard:false, //enable clipboard
	clipboardCopyStyled:true, //formatted table data
	clipboardCopySelector:"active", //method of chosing which data is coppied to the clipboard
	clipboardCopyFormatter:"table", //convert data to a clipboard string
	clipboardPasteParser:"table", //convert pasted clipboard data to rows
	clipboardPasteAction:"insert", //how to insert pasted data into the table
	clipboardCopyConfig:false, //clipboard config

	clipboardCopied:function(){}, //data has been copied to the clipboard
	clipboardPasted:function(){}, //data has been pasted into the table
	clipboardPasteError:function(){}, //data has not successfully been pasted into the table

	downloadDataFormatter:false, //function to manipulate table data before it is downloaded
	downloadReady:function(data, blob){return blob;}, //function to manipulate download data
	downloadComplete:false, //function to manipulate download data
	downloadConfig:false,	//download config

	dataTree:false, //enable data tree
	dataTreeBranchElement: true, //show data tree branch element
	dataTreeChildIndent:9, //data tree child indent in px
	dataTreeChildField:"_children", //data tre column field to look for child rows
	dataTreeCollapseElement:false, //data tree row collapse element
	dataTreeExpandElement:false, //data tree row expand element
	dataTreeStartExpanded:false,
	dataTreeRowExpanded:function(){}, //row has been expanded
	dataTreeRowCollapsed:function(){}, //row has been collapsed


	addRowPos:"bottom", //position to insert blank rows, top|bottom

	selectable:"highlight", //highlight rows on hover
	selectableRangeMode: "drag", //highlight rows on hover
	selectableRollingSelection:true, //roll selection once maximum number of selectable rows is reached
	selectablePersistence:true, // maintain selection when table view is updated
	selectableCheck:function(data, row){return true;}, //check wheather row is selectable

	headerFilterPlaceholder: false, //placeholder text to display in header filters

	history:false, //enable edit history

	locale:false, //current system language
	langs:{},

	virtualDom:true, //enable DOM virtualization

	persistentLayout:false, //store column layout in memory
	persistentSort:false, //store sorting in memory
	persistentFilter:false, //store filters in memory
	persistenceID:"", //key for persistent storage
	persistenceMode:true, //mode for storing persistence information

	responsiveLayout:false, //responsive layout flags
	responsiveLayoutCollapseStartOpen:true, //start showing collapsed data
	responsiveLayoutCollapseUseFormatters:true, //responsive layout collapse formatter
	responsiveLayoutCollapseFormatter:false, //responsive layout collapse formatter

	pagination:false, //set pagination type
	paginationSize:false, //set number of rows to a page
	paginationButtonCount: 5, // set count of page button
	paginationElement:false, //element to hold pagination numbers
	paginationDataSent:{}, //pagination data sent to the server
	paginationDataReceived:{}, //pagination data received from the server
	paginationAddRow: "page", //add rows on table or page

	ajaxURL:false, //url for ajax loading
	ajaxURLGenerator:false,
	ajaxParams:{}, //params for ajax loading
	ajaxConfig:"get", //ajax request type
	ajaxContentType:"form", //ajax request type
	ajaxRequestFunc:false, //promise function
	ajaxLoader:true, //show loader
	ajaxLoaderLoading:false, //loader element
	ajaxLoaderError:false, //loader element
	ajaxFiltering:false,
	ajaxSorting:false,
	ajaxProgressiveLoad:false, //progressive loading
	ajaxProgressiveLoadDelay:0, //delay between requests
	ajaxProgressiveLoadScrollMargin:0, //margin before scroll begins

	groupBy:false, //enable table grouping and set field to group by
	groupStartOpen:true, //starting state of group
	groupValues:false,

	groupHeader:false, //header generation function

	movableColumns:false, //enable movable columns

	movableRows:false, //enable movable rows
	movableRowsConnectedTables:false, //tables for movable rows to be connected to
	movableRowsSender:false,
	movableRowsReceiver:"insert",
	movableRowsSendingStart:function(){},
	movableRowsSent:function(){},
	movableRowsSentFailed:function(){},
	movableRowsSendingStop:function(){},
	movableRowsReceivingStart:function(){},
	movableRowsReceived:function(){},
	movableRowsReceivedFailed:function(){},
	movableRowsReceivingStop:function(){},

	scrollToRowPosition:"top",
	scrollToRowIfVisible:true,

	scrollToColumnPosition:"left",
	scrollToColumnIfVisible:true,

	rowFormatter:false,

	placeholder:false,

	//table building callbacks
	tableBuilding:function(){},
	tableBuilt:function(){},

	//render callbacks
	renderStarted:function(){},
	renderComplete:function(){},

	//row callbacks
	rowClick:false,
	rowDblClick:false,
	rowContext:false,
	rowTap:false,
	rowDblTap:false,
	rowTapHold:false,
	rowAdded:function(){},
	rowDeleted:function(){},
	rowMoved:function(){},
	rowUpdated:function(){},
	rowSelectionChanged:function(){},
	rowSelected:function(){},
	rowDeselected:function(){},
	rowResized:function(){},

	//cell callbacks
	//row callbacks
	cellClick:false,
	cellDblClick:false,
	cellContext:false,
	cellTap:false,
	cellDblTap:false,
	cellTapHold:false,
	cellEditing:function(){},
	cellEdited:function(){},
	cellEditCancelled:function(){},

	//column callbacks
	columnMoved:false,
	columnResized:function(){},
	columnTitleChanged:function(){},
	columnVisibilityChanged:function(){},

	//HTML iport callbacks
	htmlImporting:function(){},
	htmlImported:function(){},

	//data callbacks
	dataLoading:function(){},
	dataLoaded:function(){},
	dataEdited:function(){},

	//ajax callbacks
	ajaxRequesting:function(){},
	ajaxResponse:false,
	ajaxError:function(){},

	//filtering callbacks
	dataFiltering:false,
	dataFiltered:false,

	//sorting callbacks
	dataSorting:function(){},
	dataSorted:function(){},

	//grouping callbacks
	groupToggleElement:"arrow",
	groupClosedShowCalcs:false,
	dataGrouping:function(){},
	dataGrouped:false,
	groupVisibilityChanged:function(){},
	groupClick:false,
	groupDblClick:false,
	groupContext:false,
	groupTap:false,
	groupDblTap:false,
	groupTapHold:false,

	columnCalcs:true,

	//pagination callbacks
	pageLoaded:function(){},

	//localization callbacks
	localized:function(){},

	//validation has failed
	validationFailed:function(){},

	//history callbacks
	historyUndo:function(){},
	historyRedo:function(){},

};

Tabulator.prototype.initializeOptions = function(options){
	for (var key in this.defaultOptions){
		if(key in options){
			this.options[key] = options[key];
		}else{
			if(Array.isArray(this.defaultOptions[key])){
				this.options[key] = [];
			}else if(typeof this.defaultOptions[key] === "object"){
				this.options[key] = {};
			}else{
				this.options[key] = this.defaultOptions[key];
			}
		}
	}
};

Tabulator.prototype.initializeElement = function(element){

	if(element instanceof HTMLElement){
		this.element = element;
		return true;
	}else if(typeof element === "string"){
		this.element = document.querySelector(element);

		if(this.element){
			return true;
		}else{
			console.error("Tabulator Creation Error - no element found matching selector: ", element);
			return false;
		}
	}else{
		console.error("Tabulator Creation Error - Invalid element provided:", element);
		return false;
	}

};


//convert depricated functionality to new functions
Tabulator.prototype._mapDepricatedFunctionality = function(){

};

//concreate table
Tabulator.prototype._create = function(){
	this._clearObjectPointers();

	this._mapDepricatedFunctionality();

	this.bindModules();

	if(this.element.tagName === "TABLE"){
		if(this.modExists("htmlTableImport", true)){
			this.modules.htmlTableImport.parseTable();
		}
	}

	this.columnManager = new ColumnManager(this);
	this.rowManager = new RowManager(this);
	this.footerManager = new FooterManager(this);

	this.columnManager.setRowManager(this.rowManager);
	this.rowManager.setColumnManager(this.columnManager);

	this._buildElement();

	this._loadInitialData();
};

//clear pointers to objects in default config object
Tabulator.prototype._clearObjectPointers = function(){
	this.options.columns = this.options.columns.slice(0);
	this.options.data = this.options.data.slice(0);
};


//build tabulator element
Tabulator.prototype._buildElement = function(){
	var element = this.element,
	mod = this.modules,
	options = this.options;

	options.tableBuilding.call(this);

	element.classList.add("tabulator");
	element.setAttribute("role", "grid");

	//empty element
	while(element.firstChild) element.removeChild(element.firstChild);

	//set table height
	if(options.height){
		options.height = isNaN(options.height) ? options.height : options.height + "px";
		element.style.height = options.height;
	}

	this.rowManager.initialize();

	this._detectBrowser();

	if(this.modExists("layout", true)){
		mod.layout.initialize(options.layout);
	}

	//set localization
	if(options.headerFilterPlaceholder !== false){
		mod.localize.setHeaderFilterPlaceholder(options.headerFilterPlaceholder);
	}

	for(let locale in options.langs){
		mod.localize.installLang(locale, options.langs[locale]);
	}

	mod.localize.setLocale(options.locale);

	//configure placeholder element
	if(typeof options.placeholder == "string"){

		var el = document.createElement("div");
		el.classList.add("tabulator-placeholder");

		var span = document.createElement("span");
		span.innerHTML = options.placeholder;

		el.appendChild(span);

		options.placeholder = el;
	}

	//build table elements
	element.appendChild(this.columnManager.getElement());
	element.appendChild(this.rowManager.getElement());


	if(options.footerElement){
		this.footerManager.activate();
	}

	if(options.dataTree && this.modExists("dataTree", true)){
		mod.dataTree.initialize();
	}

	if( (options.persistentLayout || options.persistentSort || options.persistentFilter) && this.modExists("persistence", true)){
		mod.persistence.initialize(options.persistenceMode, options.persistenceID);
	}

	if(options.persistentLayout && this.modExists("persistence", true)){
		options.columns = mod.persistence.load("columns", options.columns) ;
	}

	if(options.movableRows && this.modExists("moveRow")){
		mod.moveRow.initialize();
	}

	if(this.modExists("columnCalcs")){
		mod.columnCalcs.initialize();
	}

	this.columnManager.setColumns(options.columns);

	if(this.modExists("frozenRows")){
		this.modules.frozenRows.initialize();
	}

	if((options.persistentSort || options.initialSort) && this.modExists("sort", true)){
		var sorters = [];

		if(options.persistentSort && this.modExists("persistence", true)){
			sorters = mod.persistence.load("sort");

			if(sorters === false && options.initialSort){
				sorters = options.initialSort;
			}
		}else if(options.initialSort){
			sorters = options.initialSort;
		}

		mod.sort.setSort(sorters);
	}

	if((options.persistentFilter || options.initialFilter) && this.modExists("filter", true)){
		var filters = [];


		if(options.persistentFilter && this.modExists("persistence", true)){
			filters = mod.persistence.load("filter");

			if(filters === false && options.initialFilter){
				filters = options.initialFilter;
			}
		}else if(options.initialFilter){
			filters = options.initialFilter;
		}

		mod.filter.setFilter(filters);
		// this.setFilter(filters);
	}

	if(this.modExists("ajax")){
		mod.ajax.initialize();
	}

	if(options.pagination && this.modExists("page", true)){
		mod.page.initialize();
	}

	if(options.groupBy && this.modExists("groupRows", true)){
		mod.groupRows.initialize();
	}

	if(this.modExists("keybindings")){
		mod.keybindings.initialize();
	}

	if(this.modExists("selectRow")){
		mod.selectRow.clearSelectionData(true);
	}

	if(options.autoResize && this.modExists("resizeTable")){
		mod.resizeTable.initialize();
	}

	if(this.modExists("clipboard")){
		mod.clipboard.initialize();
	}

	options.tableBuilt.call(this);
};

Tabulator.prototype._loadInitialData = function(){
	var self = this;

	if(self.options.pagination && self.modExists("page")){
		self.modules.page.reset(true);

		if(self.options.pagination == "local"){
			if(self.options.data.length){
				self.rowManager.setData(self.options.data);
			}else{
				if((self.options.ajaxURL || self.options.ajaxURLGenerator) && self.modExists("ajax")){
					self.modules.ajax.loadData();
				}else{
					self.rowManager.setData(self.options.data);
				}
			}
		}else{
			self.modules.page.setPage(1);
		}
	}else{
		if(self.options.data.length){
			self.rowManager.setData(self.options.data);
		}else{
			if((self.options.ajaxURL || self.options.ajaxURLGenerator) && self.modExists("ajax")){
				self.modules.ajax.loadData();
			}else{
				self.rowManager.setData(self.options.data);
			}
		}
	}
};

//deconstructor
Tabulator.prototype.destroy = function(){
	var element = this.element;

	Tabulator.prototype.comms.deregister(this); //deregister table from inderdevice communication

	//clear row data
	this.rowManager.rows.forEach(function(row){
		row.wipe();
	});

	this.rowManager.rows = [];
	this.rowManager.activeRows = [];
	this.rowManager.displayRows = [];

	//clear event bindings
	if(this.options.autoResize && this.modExists("resizeTable")){
		this.modules.resizeTable.clearBindings();
	}

	if(this.modExists("keybindings")){
		this.modules.keybindings.clearBindings();
	}

	//clear DOM
	while(element.firstChild) element.removeChild(element.firstChild);
	element.classList.remove("tabulator");
};

Tabulator.prototype._detectBrowser = function(){
	var ua = navigator.userAgent;

	if(ua.indexOf("Trident") > -1){
		this.browser = "ie";
		this.browserSlow = true;
	}else if(ua.indexOf("Edge") > -1){
		this.browser = "edge";
		this.browserSlow = true;
	}else if(ua.indexOf("Firefox") > -1){
		this.browser = "firefox";
		this.browserSlow = false;
	}else{
		this.browser = "other";
		this.browserSlow = false;
	}
};

////////////////// Data Handling //////////////////


//load data
Tabulator.prototype.setData = function(data, params, config){
	if(this.modExists("ajax")){
		this.modules.ajax.blockActiveRequest();
	}

	return this._setData(data, params, config);
};

Tabulator.prototype._setData = function(data, params, config, inPosition){
	var self = this;

	if(typeof(data) === "string"){
		if (data.indexOf("{") == 0 || data.indexOf("[") == 0){
				//data is a json encoded string
				return self.rowManager.setData(JSON.parse(data), inPosition);
			}else{

				if(self.modExists("ajax", true)){
					if(params){
						self.modules.ajax.setParams(params);
					}

					if(config){
						self.modules.ajax.setConfig(config);
					}

					self.modules.ajax.setUrl(data);

					if(self.options.pagination == "remote" && self.modExists("page", true)){
						self.modules.page.reset(true);
						return self.modules.page.setPage(1);
					}else{
						//assume data is url, make ajax call to url to get data
						return self.modules.ajax.loadData(inPosition);
					}
				}
			}
		}else{
			if(data){
				//asume data is already an object
				return self.rowManager.setData(data, inPosition);
			}else{

				//no data provided, check if ajaxURL is present;
				if(self.modExists("ajax") && (self.modules.ajax.getUrl || self.options.ajaxURLGenerator)){

					if(self.options.pagination == "remote" && self.modExists("page", true)){
						self.modules.page.reset(true);
						return self.modules.page.setPage(1);
					}else{
						return self.modules.ajax.loadData(inPosition);
					}

				}else{
					//empty data
					return self.rowManager.setData([], inPosition);
				}
			}
		}
	};

//clear data
Tabulator.prototype.clearData = function(){
	if(this.modExists("ajax")){
		this.modules.ajax.blockActiveRequest();
	}

	this.rowManager.clearData();
};

//get table data array
Tabulator.prototype.getData = function(active){
	return this.rowManager.getData(active);
};

//get table data array count
Tabulator.prototype.getDataCount = function(active){
	return this.rowManager.getDataCount(active);
};

//search for specific row components
Tabulator.prototype.searchRows = function(field, type, value){
	if(this.modExists("filter", true)){
		return this.modules.filter.search("rows", field, type, value);
	}
};

//search for specific data
Tabulator.prototype.searchData = function(field, type, value){
	if(this.modExists("filter", true)){
		return this.modules.filter.search("data", field, type, value);
	}
};

//get table html
Tabulator.prototype.getHtml = function(active){
	return this.rowManager.getHtml(active);
};

//retrieve Ajax URL
Tabulator.prototype.getAjaxUrl = function(){
	if(this.modExists("ajax", true)){
		return this.modules.ajax.getUrl();
	}
};

//replace data, keeping table in position with same sort
Tabulator.prototype.replaceData = function(data, params, config){
	if(this.modExists("ajax")){
		this.modules.ajax.blockActiveRequest();
	}

	return this._setData(data, params, config, true);
};


//update table data
Tabulator.prototype.updateData = function(data){
	var self = this;
	var responses = 0;

	return new Promise((resolve, reject) => {
		if(this.modExists("ajax")){
			this.modules.ajax.blockActiveRequest();
		}

		if(typeof data === "string"){
			data = JSON.parse(data);
		}

		if(data){
			data.forEach(function(item){
				var row = self.rowManager.findRow(item[self.options.index]);

				if(row){
					responses++;

					row.updateData(item)
					.then(()=>{
						responses--;

						if(!responses){
							resolve();
						}
					});
				}
			});
		}else{
			console.warn("Update Error - No data provided");
			reject("Update Error - No data provided");
		}
	});

};

Tabulator.prototype.addData = function(data, pos, index){
	return new Promise((resolve, reject) => {
		if(this.modExists("ajax")){
			this.modules.ajax.blockActiveRequest();
		}

		if(typeof data === "string"){
			data = JSON.parse(data);
		}

		if(data){
			this.rowManager.addRows(data, pos, index)
			.then((rows) => {
				var output = [];

				rows.forEach(function(row){
					output.push(row.getComponent());
				});

				resolve(output);
			});
		}else{
			console.warn("Update Error - No data provided");
			reject("Update Error - No data provided");
		}
	});
};

//update table data
Tabulator.prototype.updateOrAddData = function(data){
	var self = this,
	rows = [],
	responses = 0;

	return new Promise((resolve, reject) => {
		if(this.modExists("ajax")){
			this.modules.ajax.blockActiveRequest();
		}

		if(typeof data === "string"){
			data = JSON.parse(data);
		}

		if(data){
			data.forEach(function(item){
				var row = self.rowManager.findRow(item[self.options.index]);

				responses++;

				if(row){
					row.updateData(item)
					.then(()=>{
						responses--;
						rows.push(row.getComponent());

						if(!responses){
							resolve(rows);
						}
					});
				}else{
					self.rowManager.addRows(item)
					.then((newRows)=>{
						responses--;
						rows.push(newRows[0].getComponent());

						if(!responses){
							resolve(rows);
						}
					});
				}
			});
		}else{
			console.warn("Update Error - No data provided");
			reject("Update Error - No data provided");
		}
	});
};

//get row object
Tabulator.prototype.getRow = function(index){
	var row = this.rowManager.findRow(index);

	if(row){
		return row.getComponent();
	}else{
		console.warn("Find Error - No matching row found:", index);
		return false;
	}
};

//get row object
Tabulator.prototype.getRowFromPosition = function(position, active){
	var row = this.rowManager.getRowFromPosition(position, active);

	if(row){
		return row.getComponent();
	}else{
		console.warn("Find Error - No matching row found:", position);
		return false;
	}
};

//delete row from table
Tabulator.prototype.deleteRow = function(index){
	return new Promise((resolve, reject) => {
		var row = this.rowManager.findRow(index);

		if(row){
			row.delete()
			.then(() => {
				resolve();
			})
			.catch((err) => {
				reject(err);
			});

		}else{
			console.warn("Delete Error - No matching row found:", index);
			reject("Delete Error - No matching row found")
		}
	});
};

//add row to table
Tabulator.prototype.addRow = function(data, pos, index){
	return new Promise((resolve, reject) => {
		if(typeof data === "string"){
			data = JSON.parse(data);
		}

		this.rowManager.addRows(data, pos, index)
		.then((rows)=>{
			//recalc column calculations if present
			if(this.modExists("columnCalcs")){
				this.modules.columnCalcs.recalc(this.rowManager.activeRows);
			}

			resolve(rows[0].getComponent());
		});
	});
};

//update a row if it exitsts otherwise create it
Tabulator.prototype.updateOrAddRow = function(index, data){
	return new Promise((resolve, reject) => {
		var row = this.rowManager.findRow(index);

		if(typeof data === "string"){
			data = JSON.parse(data);
		}

		if(row){
			row.updateData(data)
			.then(()=>{
				//recalc column calculations if present
				if(this.modExists("columnCalcs")){
					this.modules.columnCalcs.recalc(this.rowManager.activeRows);
				}

				resolve(row.getComponent());
			})
			.catch((err)=>{
				reject(err);
			});
		}else{
			row = this.rowManager.addRows(data)
			.then((rows)=>{
				//recalc column calculations if present
				if(this.modExists("columnCalcs")){
					this.modules.columnCalcs.recalc(this.rowManager.activeRows);
				}

				resolve(rows[0].getComponent());
			})
			.catch((err)=>{
				reject(err);
			});
		}
	});
};

//update row data
Tabulator.prototype.updateRow = function(index, data){
	return new Promise((resolve, reject) => {
		var row = this.rowManager.findRow(index);

		if(typeof data === "string"){
			data = JSON.parse(data);
		}

		if(row){
			row.updateData(data).then(()=>{
				resolve(row.getComponent());
			})
			.catch((err)=>{
				reject(err);
			});
		}else{
			console.warn("Update Error - No matching row found:", index);
			reject("Update Error - No matching row found");
		}
	});
};

//scroll to row in DOM
Tabulator.prototype.scrollToRow = function(index, position, ifVisible){
	return new Promise((resolve, reject) => {
		var row = this.rowManager.findRow(index);

		if(row){
			this.rowManager.scrollToRow(row, position, ifVisible)
			.then(()=>{
				resolve();
			})
			.catch((err)=>{
				reject(err);
			});
		}else{
			console.warn("Scroll Error - No matching row found:", index);
			reject("Scroll Error - No matching row found");
		}
	});
};

Tabulator.prototype.getRows = function(active){
	return this.rowManager.getComponents(active);
};

//get position of row in table
Tabulator.prototype.getRowPosition = function(index, active){
	var row = this.rowManager.findRow(index);

	if(row){
		return this.rowManager.getRowPosition(row, active);
	}else{
		console.warn("Position Error - No matching row found:", index);
		return false;
	}
};

//copy table data to clipboard
Tabulator.prototype.copyToClipboard = function(selector, selectorParams, formatter, formatterParams){
	if(this.modExists("clipboard", true)){
		this.modules.clipboard.copy(selector, selectorParams, formatter, formatterParams);
	}
};

/////////////// Column Functions  ///////////////

Tabulator.prototype.setColumns = function(definition){
	this.columnManager.setColumns(definition);
};

Tabulator.prototype.getColumns = function(structured){
	return this.columnManager.getComponents(structured);
};

Tabulator.prototype.getColumn = function(field){
	var col = this.columnManager.findColumn(field);

	if(col){
		return col.getComponent();
	}else{
		console.warn("Find Error - No matching column found:", field);
		return false;
	}
};

Tabulator.prototype.getColumnDefinitions = function(){
	return this.columnManager.getDefinitionTree();
};

Tabulator.prototype.getColumnLayout = function(){
	if(this.modExists("persistence", true)){
		return this.modules.persistence.parseColumns(this.columnManager.getColumns());
	}
};

Tabulator.prototype.setColumnLayout = function(layout){
	if(this.modExists("persistence", true)){
		this.columnManager.setColumns(this.modules.persistence.mergeDefinition(this.options.columns, layout))
		return true;
	}
	return false;
};

Tabulator.prototype.showColumn = function(field){
	var column = this.columnManager.findColumn(field);

	if(column){
		column.show();

		if(this.options.responsiveLayout && this.modExists("responsiveLayout", true)){
			this.modules.responsiveLayout.update();
		}
	}else{
		console.warn("Column Show Error - No matching column found:", field);
		return false;
	}
};

Tabulator.prototype.hideColumn = function(field){
	var column = this.columnManager.findColumn(field);

	if(column){
		column.hide();

		if(this.options.responsiveLayout && this.modExists("responsiveLayout", true)){
			this.modules.responsiveLayout.update();
		}
	}else{
		console.warn("Column Hide Error - No matching column found:", field);
		return false;
	}
};


Tabulator.prototype.toggleColumn = function(field){
	var column = this.columnManager.findColumn(field);

	if(column){
		if(column.visible){
			column.hide();
		}else{
			column.show();
		}
	}else{
		console.warn("Column Visibility Toggle Error - No matching column found:", field);
		return false;
	}
};

Tabulator.prototype.addColumn = function(definition, before, field){
	var column = this.columnManager.findColumn(field);

	this.columnManager.addColumn(definition, before, column)
};

Tabulator.prototype.deleteColumn = function(field){
	var column = this.columnManager.findColumn(field);

	if(column){
		column.delete();
	}else{
		console.warn("Column Delete Error - No matching column found:", field);
		return false;
	}
};

//scroll to column in DOM
Tabulator.prototype.scrollToColumn = function(field, position, ifVisible){

	return new Promise((resolve, reject) => {
		var column = this.columnManager.findColumn(field);

		if(column){
			this.columnManager.scrollToColumn(column, position, ifVisible)
			.then(()=>{
				resolve();
			})
			.catch((err)=>{
				reject(err);
			});
		}else{
			console.warn("Scroll Error - No matching column found:", field);
			reject("Scroll Error - No matching column found");
		}
	});

};


//////////// Localization Functions  ////////////
Tabulator.prototype.setLocale = function(locale){
	this.modules.localize.setLocale(locale);
};

Tabulator.prototype.getLocale = function(){
	return this.modules.localize.getLocale();
};

Tabulator.prototype.getLang = function(locale){
	return this.modules.localize.getLang(locale);
};

//////////// General Public Functions ////////////

//redraw list without updating data
Tabulator.prototype.redraw = function(force){
	this.columnManager.redraw(force);
	this.rowManager.redraw(force);
};

Tabulator.prototype.setHeight = function(height){
	this.options.height = isNaN(height) ? height : height + "px";
	this.element.style.height = this.options.height;
	this.rowManager.redraw();
};

///////////////////// Sorting ////////////////////

//trigger sort
Tabulator.prototype.setSort = function(sortList, dir){
	if(this.modExists("sort", true)){
		this.modules.sort.setSort(sortList, dir);
		this.rowManager.sorterRefresh();
	}
};

Tabulator.prototype.getSorters = function(){
	if(this.modExists("sort", true)){
		return this.modules.sort.getSort();
	}
};

Tabulator.prototype.clearSort = function(){
	if(this.modExists("sort", true)){
		this.modules.sort.clear();
		this.rowManager.sorterRefresh();
	}
};


///////////////////// Filtering ////////////////////

//set standard filters
Tabulator.prototype.setFilter = function(field, type, value){
	if(this.modExists("filter", true)){
		this.modules.filter.setFilter(field, type, value);
		this.rowManager.filterRefresh();
	}
};

//add filter to array
Tabulator.prototype.addFilter = function(field, type, value){
	if(this.modExists("filter", true)){
		this.modules.filter.addFilter(field, type, value);
		this.rowManager.filterRefresh();
	}
};

//get all filters
Tabulator.prototype.getFilters = function(all){
	if(this.modExists("filter", true)){
		return this.modules.filter.getFilters(all);
	}
};

Tabulator.prototype.setHeaderFilterFocus = function(field){
	if(this.modExists("filter", true)){
		var column = this.columnManager.findColumn(field);

		if(column){
			this.modules.filter.setHeaderFilterFocus(column);
		}else{
			console.warn("Column Filter Focus Error - No matching column found:", field);
			return false;
		}
	}
};


Tabulator.prototype.setHeaderFilterValue = function(field, value){
	if(this.modExists("filter", true)){
		var column = this.columnManager.findColumn(field);

		if(column){
			this.modules.filter.setHeaderFilterValue(column, value);
		}else{
			console.warn("Column Filter Error - No matching column found:", field);
			return false;
		}
	}
};

Tabulator.prototype.getHeaderFilters = function(){
	if(this.modExists("filter", true)){
		return this.modules.filter.getHeaderFilters();
	}
};


//remove filter from array
Tabulator.prototype.removeFilter = function(field, type, value){
	if(this.modExists("filter", true)){
		this.modules.filter.removeFilter(field, type, value);
		this.rowManager.filterRefresh();
	}
};

//clear filters
Tabulator.prototype.clearFilter = function(all){
	if(this.modExists("filter", true)){
		this.modules.filter.clearFilter(all);
		this.rowManager.filterRefresh();
	}
};

//clear header filters
Tabulator.prototype.clearHeaderFilter = function(){
	if(this.modExists("filter", true)){
		this.modules.filter.clearHeaderFilter();
		this.rowManager.filterRefresh();
	}
};

///////////////////// Filtering ////////////////////
Tabulator.prototype.selectRow = function(rows){
	if(this.modExists("selectRow", true)){
		this.modules.selectRow.selectRows(rows);
	}
};

Tabulator.prototype.deselectRow = function(rows){
	if(this.modExists("selectRow", true)){
		this.modules.selectRow.deselectRows(rows);
	}
};

Tabulator.prototype.toggleSelectRow = function(row){
	if(this.modExists("selectRow", true)){
		this.modules.selectRow.toggleRow(row);
	}
};

Tabulator.prototype.getSelectedRows = function(){
	if(this.modExists("selectRow", true)){
		return this.modules.selectRow.getSelectedRows();
	}
};

Tabulator.prototype.getSelectedData = function(){
	if(this.modExists("selectRow", true)){
		return this.modules.selectRow.getSelectedData();
	}
};

//////////// Pagination Functions  ////////////

Tabulator.prototype.setMaxPage = function(max){
	if(this.options.pagination && this.modExists("page")){
		this.modules.page.setMaxPage(max);
	}else{
		return false;
	}
};

Tabulator.prototype.setPage = function(page){
	if(this.options.pagination && this.modExists("page")){
		this.modules.page.setPage(page);
	}else{
		return false;
	}
};

Tabulator.prototype.setPageSize = function(size){
	if(this.options.pagination && this.modExists("page")){
		this.modules.page.setPageSize(size);
		this.modules.page.setPage(1);
	}else{
		return false;
	}
};

Tabulator.prototype.getPageSize = function(){
	if(this.options.pagination && this.modExists("page", true)){
		return this.modules.page.getPageSize();
	}
};

Tabulator.prototype.previousPage = function(){
	if(this.options.pagination && this.modExists("page")){
		this.modules.page.previousPage();
	}else{
		return false;
	}
};

Tabulator.prototype.nextPage = function(){
	if(this.options.pagination && this.modExists("page")){
		this.modules.page.nextPage();
	}else{
		return false;
	}
};

Tabulator.prototype.getPage = function(){
	if(this.options.pagination && this.modExists("page")){
		return this.modules.page.getPage();
	}else{
		return false;
	}
};

Tabulator.prototype.getPageMax = function(){
	if(this.options.pagination && this.modExists("page")){
		return this.modules.page.getPageMax();
	}else{
		return false;
	}
};

///////////////// Grouping Functions ///////////////

Tabulator.prototype.setGroupBy = function(groups){
	if(this.modExists("groupRows", true)){
		this.options.groupBy = groups;
		this.modules.groupRows.initialize();
		this.rowManager.refreshActiveData("display");
	}else{
		return false;
	}
};

Tabulator.prototype.setGroupStartOpen = function(values){
	if(this.modExists("groupRows", true)){
		this.options.groupStartOpen = values;
		this.modules.groupRows.initialize();
		if(this.options.groupBy){
			this.rowManager.refreshActiveData("group");
		}else{
			console.warn("Grouping Update - cant refresh view, no groups have been set");
		}
	}else{
		return false;
	}
};

Tabulator.prototype.setGroupHeader = function(values){
	if(this.modExists("groupRows", true)){
		this.options.groupHeader = values;
		this.modules.groupRows.initialize();
		if(this.options.groupBy){
			this.rowManager.refreshActiveData("group");
		}else{
			console.warn("Grouping Update - cant refresh view, no groups have been set");
		}
	}else{
		return false;
	}
};

Tabulator.prototype.getGroups = function(values){
	if(this.modExists("groupRows", true)){
		return this.modules.groupRows.getGroups(true);
	}else{
		return false;
	}
};

// get grouped table data in the same format as getData()
Tabulator.prototype.getGroupedData = function(){
	if (this.modExists("groupRows", true)){
		return this.options.groupBy ?
				this.modules.groupRows.getGroupedData() : this.getData()
	}
}

///////////////// Column Calculation Functions ///////////////
Tabulator.prototype.getCalcResults = function(){
	if(this.modExists("columnCalcs", true)){
		return this.modules.columnCalcs.getResults();
	}else{
		return false;
	}
};

/////////////// Navigation Management //////////////

Tabulator.prototype.navigatePrev = function(){
	var cell = false;

	if(this.modExists("edit", true)){
		cell = this.modules.edit.currentCell;

		if(cell){
			e.preventDefault();
			return cell.nav().prev();
		}
	}

	return false;
};

Tabulator.prototype.navigateNext = function(){
	var cell = false;

	if(this.modExists("edit", true)){
		cell = this.modules.edit.currentCell;

		if(cell){
			e.preventDefault();
			return cell.nav().next();
		}
	}

	return false;
};

Tabulator.prototype.navigateLeft = function(){
	var cell = false;

	if(this.modExists("edit", true)){
		cell = this.modules.edit.currentCell;

		if(cell){
			e.preventDefault();
			return cell.nav().left();
		}
	}

	return false;
};

Tabulator.prototype.navigateRight = function(){
	var cell = false;

	if(this.modExists("edit", true)){
		cell = this.modules.edit.currentCell;

		if(cell){
			e.preventDefault();
			return cell.nav().right();
		}
	}

	return false;
};

Tabulator.prototype.navigateUp = function(){
	var cell = false;

	if(this.modExists("edit", true)){
		cell = this.modules.edit.currentCell;

		if(cell){
			e.preventDefault();
			return cell.nav().up();
		}
	}

	return false;
};

Tabulator.prototype.navigateDown = function(){
	var cell = false;

	if(this.modExists("edit", true)){
		cell = this.modules.edit.currentCell;

		if(cell){
			e.preventDefault();
			return cell.nav().dpwn();
		}
	}

	return false;
};


/////////////// History Management //////////////
Tabulator.prototype.undo = function(){
	if(this.options.history && this.modExists("history", true)){
		return this.modules.history.undo();
	}else{
		return false;
	}
};

Tabulator.prototype.redo = function(){
	if(this.options.history && this.modExists("history", true)){
		return this.modules.history.redo();
	}else{
		return false;
	}
};

Tabulator.prototype.getHistoryUndoSize = function(){
	if(this.options.history && this.modExists("history", true)){
		return this.modules.history.getHistoryUndoSize();
	}else{
		return false;
	}
};

Tabulator.prototype.getHistoryRedoSize = function(){
	if(this.options.history && this.modExists("history", true)){
		return this.modules.history.getHistoryRedoSize();
	}else{
		return false;
	}
};

/////////////// Download Management //////////////

Tabulator.prototype.download = function(type, filename, options){
	if(this.modExists("download", true)){
		this.modules.download.download(type, filename, options);
	}
};

/////////// Inter Table Communications ///////////

Tabulator.prototype.tableComms = function(table, module, action, data){
	this.modules.comms.receive(table, module, action, data);
};

////////////// Extension Management //////////////

//object to hold module
Tabulator.prototype.moduleBindings = {};

//extend module
Tabulator.prototype.extendModule = function(name, property, values){

	if(Tabulator.prototype.moduleBindings[name]){
		var source = Tabulator.prototype.moduleBindings[name].prototype[property];

		if(source){
			if(typeof values == "object"){
				for(let key in values){
					source[key] = values[key];
				}
			}else{
				console.warn("Module Error - Invalid value type, it must be an object");
			}
		}else{
			console.warn("Module Error - property does not exist:", property);
		}
	}else{
		console.warn("Module Error - module does not exist:", name);
	}

};

//add module to tabulator
Tabulator.prototype.registerModule = function(name, module){
	var self = this;
	Tabulator.prototype.moduleBindings[name] = module;
};

//ensure that module are bound to instantiated function
Tabulator.prototype.bindModules = function(){
	this.modules = {};

	for(var name in Tabulator.prototype.moduleBindings){
		this.modules[name] = new Tabulator.prototype.moduleBindings[name](this);
	}
};

//Check for module
Tabulator.prototype.modExists = function(plugin, required){
	if(this.modules[plugin]){
		return true;
	}else{
		if(required){
			console.error("Tabulator Module Not Installed: " + plugin);
		}
		return false;
	}
};


Tabulator.prototype.helpers = {

	elVisible: function(el){
		return !(el.offsetWidth <= 0 && el.offsetHeight <= 0);
	},

	elOffset: function(el){
		var box = el.getBoundingClientRect();

		return {
			top: box.top + window.pageYOffset - document.documentElement.clientTop,
			left: box.left + window.pageXOffset - document.documentElement.clientLeft
		};
	},

	deepClone: function(obj){
		var clone = Array.isArray(obj) ? [] : {};

		for(var i in obj) {
			if(obj[i] != null && typeof(obj[i])  === "object"){
				if (obj[i] instanceof Date) {
					clone[i] = new Date(obj[i]);
				} else {
					clone[i] = this.deepClone(obj[i]);
				}
			}
			else{
				clone[i] = obj[i];
			}
		}
		return clone;
	}
};

Tabulator.prototype.comms = {
	tables:[],
	register:function(table){
		Tabulator.prototype.comms.tables.push(table);
	},
	deregister:function(table){
		var index = Tabulator.prototype.comms.tables.indexOf(table);

		if(index > -1){
			Tabulator.prototype.comms.tables.splice(index, 1);
		}
	},
	lookupTable:function(query){
		var results = [],
		matches, match;

		if(typeof query === "string"){
			matches = document.querySelectorAll(query);

			if(matches.length){
				for(var i = 0; i < matches.length; i++){
					match = Tabulator.prototype.comms.matchElement(matches[i]);

					if(match){
						results.push(match);
					}
				}
			}

		}else if(query instanceof HTMLElement || query instanceof Tabulator){
			match = Tabulator.prototype.comms.matchElement(query);

			if(match){
				results.push(match);
			}
		}else if(Array.isArray(query)){
			query.forEach(function(item){
				results = results.concat(Tabulator.prototype.comms.lookupTable(item));
			});
		}else{
			console.warn("Table Connection Error - Invalid Selector", query);
		}

		return results;
	},
	matchElement:function(element){
		return Tabulator.prototype.comms.tables.find(function(table){
			return element instanceof Tabulator ? table === element : table.element === element;
		});
	}
};

/*=include modules/layout.js */
/*=include modules/localize.js */
/*=include modules/comms.js */
