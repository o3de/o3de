----------------------------------------------------------------------------------------------------
--
-- Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
-- 
-- SPDX-License-Identifier: Apache-2.0 OR MIT
--
--
--
--
--
--    Description: Containers for Lua
--  Docs:
--
----------------------------------------------------------------------------------------------------
local countTable = Set and Set.countTable or {};

-- Create factory and management table
Set = {};
Set.countTable = countTable;

-- All tables are registered in Set.countTable, therefore won't GCed!
-- Unless we make their entries weak references. See Lua docs.
-- The effect is that these references are ignored by the garbage collector.
setmetatable(Set.countTable, { __mode="kv" });


-- Create and return a new Set table
Set.New = function()
    local nt = {}
    Set.countTable[nt] = 0
    return nt
end

Set.SerializeValues = function(tbl)
    local result = {}
    for k,v in pairs(tbl) do
      local item = {}
      table.insert( item, 1, k );
      table.insert( item, 2, v );
      table.insert( result, item );
    end
    return result
end


Set.DeserializeValues = function(tbl)
    local result = Set.New()
    for i,item in ipairs(tbl) do
      Set.Add( result, item[1], item[2] );
    end
    return result;
end



Set.DeserializeEntities = function(tbl)
    local result = Set.New()
    for i,entityID in ipairs(tbl) do
        Set.Add(result, entityID)
    end
    return result
end


Set.SerializeEntities = function(tbl)
    local result = {};
    for entityID,v in pairs(tbl) do
        table.insert(result, entityID)
    end
    return result;
end


Set.DeserializeItems = function(tbl)
    local result = Set.New();
    for i,item in ipairs(tbl) do
        if (item) then
            Set.Add(result, item);
        end
    end
    return result;
end


Set.SerializeItems = function(tbl)
    local result = {};
    local index = 1;
    for item,v in pairs(tbl) do
        result[index] = item;
        index = index + 1;
    end
    return result;
end


-- Is this table registered as a Set?
-- When performance and best-effort execution are required, this function
-- can easily be reduced to a stub or even all calls to it removed with regexp
Set.Check = function(table)
    -- Run-time checking
    if (not Set.countTable[table]) then
        -- Throwing an error here would be preferable
        if (print) then 
            print(tostring(table).."is not registered as a Set"); 
        else 
            System.Log(tostring(table).."is not registered as a Set");
        end
        System.ShowDebugger()
        Set.throwAnError.throwAnError = true;
    end
end


-- Add an entry into a Set iff it does not already exist
-- if v == nil, we set it to true - leaving it nil would break things
-- Returns true on success, false if entry already exists
Set.Add = function(table, k, v)
    v = v or true;
    Set.Check(table);
    if (not table[k]) then
        table[k] = v;
        Set.countTable[table] = Set.countTable[table] + 1;
        return true;
    end
    return false;
end
     
-- Removes an entry from a Set
-- if k == nil, does nothing, which is debatable semantics
-- Returns true on success, false if entry did not exist
Set.Remove = function(table, k)
    Set.Check(table);
    if (table[k]) then
        table[k] = nil;
        Set.countTable[table] = Set.countTable[table] - 1;
        return true;
    end
    return false;
end

     
-- Get a value from a Set
-- Return the value or nil if none
Set.Get = function(table, k)
    Set.Check(table);
    return table[k]
end


-- Set the value associated with a key, even if it already exists
-- if v == nil, we set it to true - leaving it nil would break things
-- Returns true if it did already exist, false if not
Set.Set = function(table, k, v)
    v = v or true;
    Set.Check(table);
    if (not table[k]) then
        table[k] = v;
        Set.countTable[table] = Set.countTable[table] + 1;
        return false;
    else
        -- Replace, no need to change count
        table[k] = v;
        return true;
    end
end


-- Get the number of entries in the Set
Set.Size = function(table)
    Set.Check(table);
    return Set.countTable[table];
end


-- Remove all entries
-- No return value
Set.RemoveAll = function(table)
    Set.Check(table);
    for k,v in pairs(table) do
        table[k] = nil;
    end
    Set.countTable[table] = 0;
end
    
    
-- Add entries of one Set into another
-- Keys shared by both will not be overwritten
-- Returns true iff keys were disjoint, none shared
Set.Merge = function(dest, source)
    local disjoint = true;
    Set.Check(dest);
    Set.Check(source);
    for k,v in pairs(source) do
        if (not Set.Add(dest, k, v)) then
            disjoint = false;
        end
    end
    return disjoint;
end
    

-- Iterating:
-- For now, treat a Set as any table.
-- This should change.
    

-- Utility/debugging function:
-- Sanity check a suspicious Set
-- Tries to find evidence of tampering
-- Returns true iff it passes
Set.SanityCheck = function(table) 
    -- Check this was created as a Set
    Set.Check(table);  
    -- Find as many things as you can in the Set
    local count = 0;
    for i,v in pairs(table) do
        count = count + 1;
    end
    local size = Set.Size(table);
    if (count ~= size) then
        System.Log("[Set] Sanity check failed - Set size is "..tostring(size)..", counted "..count);
        return false;
    end
    -- Check that all entries have the same type
    -- This is just good practice, rather than an error
    local indexType = nil;
    local valueType = nil;
    for i,v in pairs(table) do
        -- Check index
        if (indexType) then
            if (indexType ~= type(i)) then
                System.Log("[Set] Sanity check failed - Found indices of both types "..indexType.." and "..type(i));
                return false;
            end
        else
            indexType = type(i);
        end
        -- Check value
        if (valueType) then
            if (valueType ~= type(v)) then
                System.Log("[Set] Sanity check failed - Found values of both types "..valueType.." and "..type(v));
                return false;
            end
        else
            valueType = type(v);
        end
    end
    return true;
end



-- Unit test
-- Return true for success or false on failure
Set.Test = function(fullTest)

    local A = Set.New();
    local B = Set.New();

    if ( Set.Add(A, "key1", 1) == false ) then return false; end
    if ( Set.Add(A, "key2") == false ) then return false; end
    if ( Set.Add(A, "key3", 3) == false ) then return false; end
    if ( Set.Add(A, "key1", 1) == true  ) then return false; end
    if ( Set.Remove(A, "key1") == false ) then return false; end
    if ( Set.Get(A, "key2")== false )            then return false; end
    if ( Set.Get(A, "key1") ~= nil )            then return false; end
    if ( Set.Get(A, "key3") ~= 3 )                then return false; end
    if ( Set.Size(A) ~= 2 )                             then return false; end
    Set.RemoveAll(A);
    if ( Set.Size(A) ~= 0 )                                then return false; end  
    if ( Set.Add(A, "key1", 1) == false ) then return false; end
    
    if ( Set.Set(A, "key1", 9) == false )         then return false; end
    if ( Set.Set(A, "key0", 9) == true  ) then return false; end
    if ( Set.Add(A, "keyF", 3) == false ) then return false; end
    if ( Set.Size(A) ~= 3 )                             then return false; end
    
    
    if ( Set.Add(B, "key3", 3) == false ) then return false; end
    if ( Set.Add(B, "key4", 4) == false ) then return false; end
    if ( Set.Add(B, "key5", 5) == false ) then return false; end
    if ( Set.Add(B, "key3", 3) == true  ) then return false; end
    
    if ( Set.Set(B, "key9") == true  ) then return false; end
    if ( Set.Get(B, "key9") == nil  ) then return false; end

    if (not Set.Merge(A,B)) then return false; end
    if (Set.Size(A) ~= 7) then return false; end
    
    if (Set.Merge(A,B)) then return false; end
    if (Set.Size(A) ~= 7) then return false; end
    
    if (fullTest) then
        -- Check countTable and GC (slow)
    
        -- Shouldn't assume there are no Sets when testing
    
        collectgarbage();
    
        local baseCount = 0;
        for i,v in pairs(Set.countTable) do
                baseCount = baseCount + 1;
        end
    
        B = nil;
        collectgarbage();
    
        local count = 0;
        for i,v in pairs(Set.countTable) do
                count = count + 1;
        end
        if (baseCount - count ~= 1) then return false; end
    end

    -- Move this line around for testing
    do return true; end 
    
    return true;
end

-- Perform quick version of unit test
if (not Set.Test()) then
    System.Log("Containers: ... Error - Failed Unit Test");
else
    --System.Log("Containers: ... End loading!");
end

-- Useful testing lines in Editor:
-- #Script.ReloadScript("Scripts/Utils/Containers.lua");
-- #System.Log(tostring(Set.Test(true)))