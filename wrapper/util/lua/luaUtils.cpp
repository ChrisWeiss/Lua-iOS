//
//  LuaUtils.cpp
//  BaseStation
//
//  Created by Mark Pauley on 1/9/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "util/lua/luaUtils.h"
#include "util/logging/logging.h"

#include "util/ptree/includePtree.h"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <lua/lua.hpp>

namespace {
static void Lua_PushNextToPTree(lua_State* state,
                                boost::property_tree::ptree &oTree);
static void Lua_PushNextValueToPTree(lua_State* state,
                                     boost::property_tree::ptree &oTree,
                                     const std::string &key);
static void Lua_PushNextValueInArrayToPTree(lua_State* state,
                                            boost::property_tree::ptree &oTree);
static void Lua_PushKeyValueToPTree(lua_State* state,
                                    boost::property_tree::ptree &oTree,
                                    const std::string &key,
                                    const boost::property_tree::ptree &value);
template <typename V> static void Lua_PushKeyValueToPTree(lua_State* state,
                                                          boost::property_tree::ptree &oTree,
                                                          const std::string &key,
                                                          const V &value);
static void Lua_PushPTreeAsValue(lua_State* state,
                                 const boost::property_tree::ptree&);
} // anonymous namespace, file-local

namespace Anki{ namespace Util {
using namespace boost::property_tree;

#pragma mark Public Entry Points
// Converts the top table on the given lua stack to a boost ptree.  Does not pop stack.
void Lua_ToPTree(lua_State* state, ptree& outTree)
{
  // Make sure we actually have something on the lua stack.
  if(lua_gettop(state) < 1) {
    PRINT_NAMED_ERROR("Lua_ToPTree", "Should have at least one argument on the lua stack (was %d)!",
                 lua_gettop(state));
    return;
  }
  
  // Make sure the thing on the stack is actually a table.
  int type;
  if((type = lua_type(state, -1)) == LUA_TTABLE) {
    // Loop through the hash entries (if any)
    lua_pushnil(state); // lua_next requires nil at the top to iterate a table.
    while(lua_next(state, -2) != 0) { // recall that the table is two from the top since we pushed nil.
      Lua_PushNextToPTree(state, outTree);
      lua_pop(state, 1); // pop the value, lua will find the key after this key.
    }
    // Keep the children in sorted order for sanity's sake.
    outTree.sort();
    
    // Loop through the indexed entries (if any)
    size_t len = luaL_len(state, -1);
    for(int i = 1; i <= len; i++) {
      lua_rawgeti(state, -1, i);
      Lua_PushNextValueInArrayToPTree(state, outTree);
      lua_pop(state, 1);
    }
  }
  else {
    PRINT_NAMED_ERROR("Lua_ToPTree", "Should be a table on the lua stack, was %s (%d)",
                 lua_typename(state, type), type);
  }
}
  
// Converts the ptree to a lua table, which is then pushed to the top of the given lua stack.
void Lua_PushPTreeAsTable(lua_State* state, boost::property_tree::ptree const& tree)
{
  // First, we create a new table
  //  (split into the array part and the hash part)
  size_t arrayLength = 0;
  size_t childrenLength = 0;
  
  // Arrays are children with an empty key
  arrayLength = tree.count("");
  
  // The size counts the array part as well, so ignore them.
  childrenLength = tree.size() - arrayLength;
  
  // Make the table (will be on top of the stack)
  assert(arrayLength < INT_MAX);
  assert(childrenLength < INT_MAX);
  lua_createtable(state, (int)arrayLength, (int)childrenLength);
  
  // Lua tables can have hash values as well as arrays
  // So can ptree, so handle both.
  for(const ptree::value_type& kv : tree)
  {
    if(kv.first == "")
    {
      // push as array
      Lua_PushPTreeAsValue(state, kv.second);
      size_t curArrayLength = lua_rawlen(state, -2);
      assert(childrenLength < (INT_MAX - 1));
      lua_rawseti(state, -2, (int)curArrayLength + 1);
    }
    else
    {
      // push as hash
      lua_pushstring(state, kv.first.c_str());
      Lua_PushPTreeAsValue(state, kv.second);
      lua_settable(state, -3);
    }
  }
}

}
} // namespace

// Anonymous namespace for the helper functions
namespace {

#pragma mark - Helper Functions

#pragma mark Lua to ptree helpers
// Using value on the stack as the next key
void Lua_PushNextToPTree(lua_State* state, boost::property_tree::ptree& oTree)
{
  ASSERT_NAMED(lua_gettop(state) >= 2, "Lua_PushNextToPTree");
  // get type of key
  switch (lua_type(state, -2))
  {
    case LUA_TSTRING:
    {
      std::string strValue(lua_tostring(state, -2));
      Lua_PushNextValueToPTree(state, oTree, strValue);
    }
      break;
    case LUA_TNUMBER:
      // We just ignore these, because we'll pull them out when we go through the indexed entries.
      break;
    default:
      PRINT_NAMED_WARNING("Lua_PushNextToPTree", "Unhandled lua type: %d", lua_type(state, -2));
      break;
  }
}

// Using value on the stack as the next value
void Lua_PushNextValueToPTree(lua_State* state, boost::property_tree::ptree& oTree, const std::string &key)
{
  boost::property_tree::ptree subTree;
  double numValue;
  bool   boolValue;
  std::string strValue;
  
  switch (lua_type(state, -1))
  {
    case LUA_TBOOLEAN:
      boolValue = lua_toboolean(state, -1);
      Lua_PushKeyValueToPTree<bool>(state, oTree, key, boolValue);
      break;
    case LUA_TNUMBER:
      numValue = lua_tonumber(state, -1);
      Lua_PushKeyValueToPTree<double>(state, oTree, key, numValue);
      break;
    case LUA_TSTRING:
      strValue = std::string(lua_tostring(state, -1));
      Lua_PushKeyValueToPTree<std::string>(state, oTree, key, strValue);
      break;
    case LUA_TTABLE:
      Anki::Util::Lua_ToPTree(state, subTree);
      Lua_PushKeyValueToPTree(state, oTree, key, subTree);
      break;
    default:
      PRINT_NAMED_WARNING("Lua_PushNextValueToPTree", "Unhandled lua type: %d", lua_type(state, -1));
      break;
  }
}

void Lua_PushKeyValueToPTree(lua_State* state,
                             boost::property_tree::ptree &oTree,
                             const std::string &key,
                             const boost::property_tree::ptree &value)
{
  oTree.put_child(key, value);
}

template <typename V> static void Lua_PushKeyValueToPTree(lua_State* state,
                                                          boost::property_tree::ptree &oTree,
                                                          const std::string &key,
                                                          const V &value)
{
  oTree.put(key, value);
}

void Lua_PushNextValueInArrayToPTree(lua_State* state, boost::property_tree::ptree &oTree)
{
  bool   boolVal;
  lua_Number numVal;
  std::string strVal;
  
  boost::property_tree::ptree child;
  
  switch (lua_type(state, -1)) {
    case LUA_TBOOLEAN:
      boolVal = lua_toboolean(state, -1);
      child.put("", boolVal);
      oTree.push_back(make_pair("", child));
      break;
    case LUA_TNUMBER:
      numVal = lua_tonumber(state, -1);
      child.put("", numVal);
      oTree.push_back(make_pair("", child));
      break;
    case LUA_TSTRING:
      strVal = std::string(lua_tostring(state, -1));
      child.put("", strVal);
      oTree.push_back(make_pair("", child));
      break;
    case LUA_TTABLE:
      Anki::Util::Lua_ToPTree(state, child);
      oTree.push_back(make_pair("", child));
      break;
    default:
      PRINT_NAMED_WARNING("Lua_PushNextValueInArrayToPTree", "Unhandled lua type: %d", lua_type(state, -1));
      break;
  }
}

#pragma mark ptree to Lua table helpers

// Custom translator for bool (only supports std::string)
struct BoolTranslator
{
  // Converts a string to bool
  boost::optional<bool> get_value(const std::string& str)
  {
    if (!str.empty())
    {
      using boost::algorithm::iequals;
      
      if (iequals(str, "true"))
        return boost::optional<bool>(true);
      else if(iequals(str, "false"))
        return boost::optional<bool>(false);
    }
    return boost::optional<bool>(boost::none);
  }
  
  // Converts a bool to string
  boost::optional<std::string> put_value(const bool& b)
  {
    return boost::optional<std::string>(b ? "true" : "false");
  }
};
  
// tree could contain just a single value, or it could be a sub-tree
void Lua_PushPTreeAsValue(lua_State* state,
                          const boost::property_tree::ptree& tree)
{
  // Check for a sub-tree
  if(!tree.empty())
  {
    // We don't support ptrees that both have data and children.
    //  Personally, I feel this is an abomination. (pauley)
    boost::optional<const boost::property_tree::ptree&> treeOpt = tree.get_child_optional("");
    if(treeOpt)
    {
      Anki::Util::Lua_PushPTreeAsTable(state, treeOpt.get());
      return;
    }
  }
  
  {
    // Check for a boolean
    BoolTranslator boolDetector;
    boost::optional<bool>boolOpt = tree.get_optional<bool>("", boolDetector);
    if(boolOpt)
    {
      lua_pushboolean(state, boolOpt.get());
      return;
    }
  }
  
  // Check for a number
  boost::optional<lua_Number> doubleOpt = tree.get_optional<lua_Number>("");
  if(doubleOpt)
  {
    lua_pushnumber(state, doubleOpt.get());
    return;
  }
  
  // Finally, check for a string
  // Theoretically, this should always work.
  boost::optional<std::string> stringOpt = tree.get_optional<std::string>("");
  if(stringOpt)
  {
    lua_pushstring(state, stringOpt.get().c_str());
    return;
  }
  // Should never get here.
  ASSERT_NAMED(!"Unable to parse this ptree!", "Lua_PushPTreeAsValue");
  lua_pushnil(state);
}
  
} // anonymous namespace
