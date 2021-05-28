/*
*  All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
*  its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
var http = require("https");

var clientID = "{{clientID}}";
var clientSecret = "{{clientSecret}}";
var redirectUrl = "{{redirectUrl}}";

var getRefreshToken = function(code,  callback)
{
    var options = {
        hostname: '{{hostname}}',
        path: '{{path}}',
        port: {{port}},
        method : "POST",
        headers  : {'Content-Type':     'application/x-www-form-urlencoded;charset=UTF-8'}
        };

   var body = "grant_type=authorization_code&";
    body += "client_id=" + clientID + "&";
    body += "redirect_uri=" + redirectUrl + "&";
    body += "client_secret=" + clientSecret + "&";
    body += "code=" + code;
    console.log(body);

    var req = http.request(options, function(res) {

        res.setEncoding('utf8');
        var store = "";

        res.on('data', function (chunk) {
            console.log(chunk);
            store += chunk;
        });

        res.on('end', function() {
            var response = JSON.parse(store);
            var data = {
                access_token : response.access_token,
                expires_in : response.expires_in,
                refresh_token : response.refresh_token,
                id_token : response.id_token
                };
                console.log(data);
            callback(null, data);

        });

        req.on('error', function(e) {
            console.log('problem with request: ' + e.message);
            callback(e);
        });
    });

    req.write(body);
    req.end();

};

var refreshAccessToken = function(refreshToken,  callback)
{
    var options = {
        hostname: '{{hostname}}',
        path: '{{path}}',
        port: {{port}},
        method : "POST",
        headers  : {'Content-Type':     'application/x-www-form-urlencoded;charset=UTF-8'}
        };

    var body = "grant_type=refresh_token&";
    body += "refresh_token=" + refreshToken + "&";
    body += "client_id=" + clientID + "&";
    body += "client_secret=" + clientSecret;
    console.log(body);

    var req = http.request(options, function(res) {

        res.setEncoding('utf8');
        var store = "";

        res.on('data', function (chunk) {
            console.log(chunk);
            store += chunk;
        });

        res.on('end', function() {
            var response = JSON.parse(store);
            var data = {
                access_token : response.access_token,
                expires_in : response.expires_in
                };
                console.log(data);
            callback(null, data);

        });

        req.on('error', function(e) {
            console.log('problem with request: ' + e.message);
            callback(e);
        });
    });

    req.write(body);
    req.end();

};

exports.{{handlerName}} = function(event, context) {
    if(typeof event.code != 'undefined') {
        var code = event.code;
        console.log(code);

        getRefreshToken(code, function(err, data)
        {
            if(err)
            {
                context.fail(err);
            }
            else
            {

                context.succeed (data);

            }
        });
    }
    else if(typeof event.refresh_token != 'undefined') {
        var refresh_token = event.refresh_token;
        console.log(refresh_token);

        refreshAccessToken(refresh_token, function(err, data)
        {
            if(err)
            {
                context.fail(err);
            }
            else
            {
                context.succeed (data);
            }
        });
    }
};
