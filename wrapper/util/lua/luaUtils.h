//
//  LuaUtils.h
//  BaseStation
//
//  Created by Mark Pauley on 1/9/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//
//  Description:
//  Utility functions that might be useful in getting data into or out of Lua.
//

#ifndef UTIL_LUA_LUAUTILS_H_
#define UTIL_LUA_LUAUTILS_H_

struct lua_State;
#include <boost/property_tree/ptree_fwd.hpp>

namespace Anki{ namespace Util
{
  
// Converts the top table on the given lua stack to a boost ptree.  Does not pop stack.
void Lua_ToPTree(lua_State* state, boost::property_tree::ptree& outTree);

// Converts the ptree to a lua table, which is then pushed to the top of the given lua stack.
void Lua_PushPTreeAsTable(lua_State* state, boost::property_tree::ptree const &tree);

}
} // namespace

#endif