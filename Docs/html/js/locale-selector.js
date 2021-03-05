$(document).ready(function ()
{

    // check language cookie early on in case we need to reload the correct page
    CheckLanguageCookie();
    CheckRegistrationCookie();

});



function SetCookie(name, value, session)
{
    var days = 14;
    var d = new Date();

    // set expire date to milliseconds
    d.setTime(d.getTime() + (days * 24 * 60 * 60 * 1000));
    
    if (session == 1){
        document.cookie = name + "=" + value + ";path=/";
    }
    else {
        document.cookie = name + "=" + value + ";expires=" + d.toUTCString() + ";path=/";        
    }
}

function CheckRegistrationCookie()
{
        var regCookieName = "regStatus";
        var regCookieValue = getCookie(regCookieName);

	if(regCookieValue=="pre-register")
	{
		$('#span-conosole-signin').css('display','none');
		$('#span-conosole-signup').css('display','inline');
	}
	else
	{
		$('#span-conosole-signin').css('display','inline');
		$('#span-conosole-signup').css('display','none');
	}
}

function CheckLanguageCookie()
{
    try
    {
        var languageCookieName = "aws-doc-lang";
        var languageCookieValue = getCookie(languageCookieName);
        var currentSelectorUrl = $("#languageSelection").val();
        var pageLocale = currentSelectorUrl.substring(1, 6);
        var pageBasePath = currentSelectorUrl.substring(6);
        var cookieUrl = "/" + languageCookieValue + pageBasePath;

        var currentUrl = location.href;
        var pathIndex = currentUrl.indexOf(pageBasePath);

        if (pathIndex >= 6)
        {
            var startIndex = pathIndex - 5;
            var urlLocale = currentUrl.substring(startIndex, pathIndex);
            var urlLocaleStart = currentUrl.substring(startIndex - 1, startIndex);

            // Check if this page is in locale folder...
            if ((urlLocaleStart == "/") && (urlLocale.substring(2, 3) == "_"))
            {
                // Locale Folder Handling Section
                // Check if the locale of the content and the locale folder agreee
                if (pageLocale != urlLocale)
                {
                    //var currentSelection = $('option:selected', 'select[id="languageSelection"]');
                    //var displayText = " Translation Unavailable (Displaying: " + currentSelection.text() + ")";

                    // remove selected attribute from the currently selected option
                    //currentSelection.removeAttr('selected');

                    // add option that captures the preferred language and display language
                    //$("#languageSelection").append($("<option selected=\"selected\" enabled=\"false\">" + displayText + "</option>"));
                }

            }
            else
            {
                // Root Folder Handling Section
                // Normally taken care of by the server, but checking in case of a cached version is being displayed.
                if ((languageCookieValue != "") && (languageCookieValue != pageLocale))
                {
                    location.href = cookieUrl;
                }
            }
        }

        // check if cookie is set
        if (languageCookieValue.length == 5)
        {
            var cookieOption = $('select[id="languageSelection"]').find('option[value="' + cookieUrl + '"]');

            // refresh cookie
            SetCookie(languageCookieName, languageCookieValue, 0);

            // Add checkmark to the language specified by the cookie...
            cookieOption.html(cookieOption.text() + "&nbsp;&#10003;");

        }
    }
    catch (error)
    {
        // content does not support the language selector.
    }
}

function getCookie(cname)
{
    var name = cname + "=";
    var ca = document.cookie.split(';');
    for (var i = 0; i < ca.length; i++)
    {
        var c = ca[i];
        while (c.charAt(0) == ' ') c = c.substring(1);
        if (c.indexOf(name) != -1) return c.substring(name.length, c.length);
    }
    return "";
}


