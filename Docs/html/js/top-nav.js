var AWSDocs = AWSDocs || {};

AWSDocs.Flyout = (function () {

    var slideOpen = function ($trigger, $el) {
        $trigger.addClass("active");
        $el.show("slide", 300);
    };

    var keepOpen = function ($trigger, $el) {
        $trigger.addClass("active");
        $el.show();
    };

    var close = function ($trigger, $el) {
        $trigger.removeClass("active");
        $el.hide();
    };

    return {
        slideOpen: slideOpen,
        keepOpen: keepOpen,
        close: close
    };

})();

AWSDocs.TemplateCompiler = (function() {

    var compileFlyout = function($flyoutTemplate, $flyout, flyoutData) {
        
        if (typeof Handlebars !== "undefined"){
        var rawHtml = $flyoutTemplate.html();
        var template = Handlebars.compile(rawHtml);      // 3) prepare the HTML template for execution ...
        var context = flyoutData;
        var renderedHtml = template(context);            // 4) execute the template against the content ...

        $flyoutTemplate.replaceWith(renderedHtml);       // 5) inject the transformed content into the page ...

        if ($flyout) {
            $flyout.menu();                              // 6) activate styling for flyout, AFTER we've built it
        }

        var $homeLink = $( "#ui-id-1" );                 // 7) a hack to make the home flyout link work
        if ($homeLink) {
            $homeLink.click(function() {
                window.location = flyoutData.flyoutList[0].topTarget;
            });
        }
        }
    };

    var getFlyoutJson = function($flyoutTemplate, $flyout) {
	
	var locale = $("meta[name=guide-locale]").attr("content");
    var requestUrl = "/flyout.json";  // server request from server-root/js/awsdocs.min.js to server-root/flyout.js
	
	if (typeof locale !== "undefined")
	{
	if(locale.length==5)
	{
	    requestUrl = "/menu/" + locale + requestUrl;
	}
	else
	{
	    // use default if we don't have a valid locale
	    requestUrl = "/menu" + requestUrl;
	}
	}

        $.getJSON(requestUrl, function( flyoutData ) {               // 2) retrieve the flyout content from the server
            compileFlyout($flyoutTemplate, $flyout, flyoutData);     //    and pass on the template, flyout el, and JSON ...
        })
    };

    var createFlyout = function($flyoutTemplate, $flyout) {

        if ($flyoutTemplate) {                              // 1) if the template exists on the page ...

            getFlyoutJson($flyoutTemplate, $flyout);
        };

    };

    return {
        createFlyout: createFlyout
    };

})();

AWSDocs.Cookies = (function ()
{

    var setCookie = function ($name, $value)
    {
        var days = 14;
        var d = new Date();

        // set expire date to milliseconds
        d.setTime(d.getTime() + (days * 24 * 60 * 60 * 1000));

        document.cookie = $name + "=" + $value + ";expires=" + d.toUTCString() + ";path=/";
    };


    return {
        setCookie: setCookie
    };

})();

$(document).ready(function () {

    var $hamburger = $("#aws-nav-flyout-trigger");
    var $flyoutContainer = $("#topnav-flyout-menu-container");
    var $flyout = $("#topnav-flyout-menu");
    var $langSelector = $("#languageSelection");
    var $flyoutTemplate = $( "#flyout-item-template" );

    AWSDocs.TemplateCompiler.createFlyout($flyoutTemplate, $flyout);

    if ($langSelector) {
        // activate styling for language selector
        $langSelector.selectmenu({
            select: function (event, ui)
            {
                var languageCookieName = "aws-doc-lang";
                var targetUrl = ui.item.value;
                var selectedLanguage = targetUrl.substring(1, 6);
                var d = new Date();
                d.setTime(d.getTime() + (14 * 24 * 60 * 60 * 1000));

                AWSDocs.Cookies.setCookie(languageCookieName, selectedLanguage);

                if (location.href.indexOf(targetUrl) < 0)
                {
                    location.href = targetUrl;
                }

            }
        });

        $( window ).resize(function() {
            $langSelector.selectmenu( "close" );
        });

    }

    if ($hamburger) {
        $hamburger.mouseenter(

            function ( event ) {

                if (event.relatedTarget !== null && (event.relatedTarget.id == "ui-id-1"                ||
                                                     event.relatedTarget.id == "awsdocs-flyout-first-a" ||
                                                     event.relatedTarget.id == "topnav-flyout-menu"     )) { // see if we're entering from the flyout menu
                    AWSDocs.Flyout.keepOpen($hamburger, $flyoutContainer);                                   // if we ARE entering from the flyout, don't slide open again
                } else {
                    AWSDocs.Flyout.slideOpen($hamburger, $flyoutContainer);
                }
            }
        );

        $hamburger.mouseleave( // when the user mouses out of the $hamburger, collapse
            function () {
                AWSDocs.Flyout.close($hamburger, $flyoutContainer);
            }
        );
    }

    if ($flyoutContainer) {
        $flyoutContainer.hover(
            function () {     // on mouseenter
                AWSDocs.Flyout.keepOpen($hamburger, $flyoutContainer);
            }, function () {     // on mouseleave
                AWSDocs.Flyout.close($hamburger, $flyoutContainer);
                $flyout.menu("collapseAll");
            }
        );
    }
});



