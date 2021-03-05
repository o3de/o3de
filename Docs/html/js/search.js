function searchFormSubmit(formElement)
{
    //#facet_doc_product=Amazon+CloudFront&amp;facet_doc_guide=Developer+Guide+(API+Version+2012-07-01)
    var so = $("#search-select").val();
    if (so.indexOf("documentation") === 0)
    {
        var this_doc_product = $("#this_doc_product").val();
        var this_doc_guide = $("#this_doc_guide").val();
        var action = "";
        var facet = "";
        if (so === "documentation-product" || so === "documentation-guide")
        {
            action += "?doc_product=" + encodeURIComponent(this_doc_product);
            facet += "#facet_doc_product=" + encodeURIComponent(this_doc_product);
            if (so === "documentation-guide")
            {
                action += "&doc_guide=" + encodeURIComponent(this_doc_guide);
                facet += "&facet_doc_guide=" + encodeURIComponent(this_doc_guide);
            }
        }

        var ua = window.navigator.userAgent;
        var msie = ua.indexOf('MSIE ');
        var trident = ua.indexOf('Trident/');
        var edge = ua.indexOf('Edge/');

        if (msie > 0 || trident > 0 || edge > 0)
        {
            var sq = $("#search-query").val();
            action += "&searchPath=" + encodeURIComponent(so);
            action += "&searchQuery=" + encodeURIComponent(sq);
            window.location.href = "/search/doc-search.html" + action + facet;
            return false;
        } else
        {
            formElement.action = "/search/doc-search.html" + facet;
        }
    } else
    {
        formElement.action = "http://aws.amazon.com/search";
    }
    return true;
}
