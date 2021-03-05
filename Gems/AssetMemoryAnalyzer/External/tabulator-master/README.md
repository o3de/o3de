![Tabulator Table](http://olifolkerd.github.io/tabulator/images/tabulator.png)

### Version 4.1 Out Now!

An easy to use interactive table generation JavaScript library

Full documentation & demos can be found at: [http://tabulator.info](http://tabulator.info)
***
![Tabulator Table](http://tabulator.info/images/tabulator_table.jpg)
***

NPM Package Changed
================================
jQuery was removed as a dependency in the 4.0 release, so Tabulator has moved in NPM from the old [jquery.tabulator](https://www.npmjs.com/package/jquery.tabulator) package to the new [tabulator-tables](https://www.npmjs.com/package/tabulator-tables) package.


Features
================================
Tabulator allows you to create interactive tables in seconds from any HTML Table, Javascript Array or JSON formatted data.

Simply include the library and the css in your project and you're away!

Tabulator is packed with useful features including:

![Tabulator Features](http://olifolkerd.github.io/tabulator/images/featurelist_share.png)


Frontend Framework Support
================================
Tabulator is built to work with all the major front end JavaScript frameworks including React, Angular and Vue.


Setup
================================
Setting up tabulator could not be simpler.

Include the library and the css
```html
<link href="dist/css/tabulator.min.css" rel="stylesheet">
<script type="text/javascript" src="dist/js/tabulator.min.js"></script>
```

Create an element to hold the table
```html
<div id="example-table"></div>
```

Turn the element into a tabulator with some simple javascript
```js
var table = new Tabulator("#example-table", {});
```


### Bower Installation
To get Tabulator via the Bower package manager, open a terminal in your project directory and run the following commmand:
```
bower install tabulator --save
```

### NPM Installation
To get Tabulator via the NPM package manager, open a terminal in your project directory and run the following commmand:
```
npm install tabulator-tables --save
```

### CDN - UNPKG
To access Tabulator directly from the UNPKG CDN servers, include the following two lines at the start of your project, instead of the localy hosted versions:
```html
<link href="https://unpkg.com/tabulator-tables@4.1.2/dist/css/tabulator.min.css" rel="stylesheet">
<script type="text/javascript" src="https://unpkg.com/tabulator-tables@4.1.2/dist/js/tabulator.min.js"></script>
```

Coming Soon
================================
Tabulator is actively under development and I plan to have even more useful features implemented soon, including:

- Data Reactivity
- Custom Row Templates
- Additional Editors and Formatters
- Print Styling
- Multi Cell Editing
- Cell Selection

Get in touch if there are any features you feel Tabulator needs.
