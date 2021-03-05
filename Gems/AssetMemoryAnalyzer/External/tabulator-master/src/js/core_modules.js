;(function (global, factory) {
	if(typeof exports === 'object' && typeof module !== 'undefined'){
		module.exports = factory();
	}else if(typeof define === 'function' && define.amd){
		define(factory);
	}else{
		global.Tabulator = factory();
	}
}(this, (function () {

	/*=include core.js */
	/*=include modules_enabled.js */

	return Tabulator;

})));