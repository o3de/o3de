----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project.
-- For complete copyright and license terms please see the LICENSE at the root of this distribution.
--
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
----------------------------------------------------------------------------------------------------
require("test0")
require("folder.test1")
require("folder.separated.test2")
require("folder/separated/test3")
require'folder.test4'
require'test5'
require"folder.test6"
require"test7"
require "folder.test8"
require("folder.test9")
require( "folder.test10")
require( "folder.test11" )
require ("folder.test12") 
require ( "folder.test13")
require ( "folder.test14" )
require 'folder.test15'
require "folder.test16"
require("test17") -- tests greedy operation in regex...
if _G.value ~= true then  end 
local result0 = require("local_test0")
local result1 = require("folder.local_test1")
local result2 = require("folder.separated.local_test2")
local result3 = require("folder/separated/local_test3")
local result4 = require'folder.local_test4'
local result5 = require'local_test5'
local result6 = require"folder.local_test6"
local result7 = require"local_test7"
local result8 = require "folder.local_test8"
local result9 = require("folder.local_test9")
local result10 = require( "folder.local_test10")
local result11 = require( "folder.local_test11" )
local result12 = require ("folder.local_test12") 
local result13 = require ( "folder.local_test13")
local result14 = require ( "folder.local_test14" )
local result15 = require 'folder.local_test15'
local result16 = require "folder.local_test16"
local result17 = require("local_test17") -- tests greedy operation in regex
if _G.value ~= true then  end 
