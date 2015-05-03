//
//  testLua.cpp
//  BaseStation
//
//  Created by Mark Pauley on 1/10/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//


#include "util/helpers/includeGTest.h"

#include <lua/lua.hpp>
#include "util/lua/luaUtils.h"
#include "util/lua/luaContext.h"
#include "util/lua/luaScript.h"
#include "util/lua/luaDebugger.h"
#include "basestation/utils/parameters.h"
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
#include "util/ptree/ptreeTools.h"
#include "basestation/tests/bTestDefines.h"
#include "basestation/ui/platform/standalonePlatform.h"

using namespace std;
using namespace BaseStation;
using boost::property_tree::ptree;

namespace BaseStation {
class TestLua : public ::testing::Test {
public:
  TestLua()
  : ::testing::Test()
  , state_(nullptr)
  , platform_(nullptr)
  {};
  
  void SetUp()
  {
    state_ = luaL_newstate();
    luaL_openlibs(state_);
    expectedPTree_ = ptree();
    string configRoot = "./";
    char* configRootChars = getenv(CONFIGROOT);
    if (configRootChars != NULL)
      configRoot = configRootChars;
    platform_ = new StandalonePlatform(configRoot + "basestation/config");
    
  }
  void TearDown()
  {
    lua_close(state_);
    state_ = NULL;
    delete platform_;
    platform_ = nullptr;
  }
  void LoadJSON(string const& jsonStr, ptree& outTree);
  void ExpectJSON(string const& jsonStr);
  ptree RecursiveSort(ptree &tree);
  void CheckExpected(ptree &test);
  
  void ExpectTable(string const& table);
  
protected:
  lua_State* state_;
  ptree expectedPTree_;
  Platform* platform_;
};
  

  
  
#pragma mark JSON conversion tests.
  
void TestLua::LoadJSON(string const& jsonStr, ptree& outTree)
{
  std::stringstream stream(jsonStr);
  boost::property_tree::json_parser::read_json(stream, outTree);
}
  
void TestLua::ExpectJSON(string const &jsonStr)
{
  LoadJSON(jsonStr, expectedPTree_);
}
  
ptree TestLua::RecursiveSort(ptree &tree)
{
  for(ptree::value_type &v : tree)
  {
    if(v.first != "")
    {
      v.second.sort();
      tree.put_child(v.first, RecursiveSort(v.second));
    }
  }
  return tree;
}
  
void TestLua::CheckExpected(ptree &test)
{
  TestLua::RecursiveSort(expectedPTree_);
  TestLua::RecursiveSort(test);
  
  if(! (expectedPTree_ == test) )
  {
    stringstream expectedStream, actualStream;
    boost::property_tree::json_parser::write_json(expectedStream,
                                                  expectedPTree_);
    boost::property_tree::json_parser::write_json(actualStream, test);
    cout << "Expected: " << expectedStream.str() << endl;
    cout << "Actual: " << actualStream.str() << endl;
    EXPECT_TRUE(expectedPTree_ == test);
  }
  
}

TEST_F(TestLua, TestJSONConversionSimpleList)
{
  ExpectJSON("{ \"result\": [ 1, true, 3, \"foo\", 5 ] }");

  luaL_loadstring(state_, "result = {  1, true, 3, \"foo\", 5 }");
  EXPECT_EQ(LUA_OK, lua_pcall(state_, 0, 0, 0));
  lua_getglobal(state_, "result");
  ptree list;
  ptree children;

  Anki::Util::Lua_ToPTree(state_, children);
  list.add_child("result", children);
  CheckExpected(list);
}

TEST_F(TestLua, TestJSONConversionSimpleTable)
{
  ExpectJSON("{ \"result\": { \"foo\": \"bar\", \"bar\": false, \"weeble\": 2 } }" );
  
  luaL_loadstring(state_, "result = { foo=\"bar\", bar=false, weeble=2 }");
  EXPECT_EQ(LUA_OK, lua_pcall(state_, 0, 0, 0));
  lua_getglobal(state_, "result");
  ptree list;
  ptree children;
  
  Anki::Util::Lua_ToPTree(state_, children);
  list.add_child("result", children);
  CheckExpected(list);
}
  
TEST_F(TestLua, TestJSONConversionSimpleRecursiveList)
{
  ExpectJSON("{ \"result\": [ [1, 2, 3], [2, 3, 4], [3, 4, 5], [4, 5, 6], [5, 6, 7] ] }");
  
  luaL_loadstring(state_, "result = {{1, 2, 3}, {2, 3, 4}, {3, 4, 5}, {4, 5, 6}, {5, 6, 7}}");
  EXPECT_EQ(LUA_OK, lua_pcall(state_, 0, 0, 0));
  lua_getglobal(state_, "result");
  ptree list;
  ptree children;
  
  Anki::Util::Lua_ToPTree(state_, children);
  list.add_child("result", children);
  CheckExpected(list);
}
  
TEST_F(TestLua, TestJSONConversionSimpleRecursiveTable)
{
  ExpectJSON("{ \"result\": { \"foo\": { \"bar\": \"baz\", \"foo\": \"bar\", \"weeble\": 2 } }}");
  
  luaL_loadstring(state_, "result = { foo={ foo=\"bar\", bar=\"baz\", weeble=2 }}");
  EXPECT_EQ(LUA_OK, lua_pcall(state_, 0, 0, 0));
  lua_getglobal(state_, "result");
  ptree list;
  ptree children;
  
  Anki::Util::Lua_ToPTree(state_, children);
  list.add_child("result", children);
  CheckExpected(list);
}
  
void TestLua::ExpectTable(const string &table)
{
  string returnString = string("result = ") + table;
  luaL_loadstring(state_, returnString.c_str());
  EXPECT_EQ(LUA_OK, lua_pcall(state_, 0, 0, 0));
  lua_getglobal(state_, "result");
  EXPECT_EQ(1, lua_gettop(state_));
  EXPECT_TRUE(lua_istable(state_, 1));
  Anki::Util::Lua_ToPTree(state_, expectedPTree_);
  lua_pop(state_, 1);
}
  
TEST_F(TestLua, TestLuaConversionSimpleJSON)
{
  ExpectTable("{ a = \"foo\", b = false, c = \"baz\" }");
  ptree inTree;
  LoadJSON("{ \"a\": \"foo\", \"b\": false, \"c\": \"baz\" }", inTree);
  Anki::Util::Lua_PushPTreeAsTable(state_, inTree);
  ptree outTree;
  Anki::Util::Lua_ToPTree(state_, outTree);
  CheckExpected(outTree);
}
  
TEST_F(TestLua, TestLuaConversionArrayJSON)
{
  ExpectTable("{ 1, 1, 2, 3, 5, 8, 13 }");
  ptree inTree;
  LoadJSON("[ 1, 1, 2, 3, 5, 8, 13 ]", inTree);
  Anki::Util::Lua_PushPTreeAsTable(state_, inTree);
  ptree outTree;
  Anki::Util::Lua_ToPTree(state_, outTree);
  CheckExpected(outTree);
}
  
TEST_F(TestLua, TestLuaConversionNestedJSON)
{
  ExpectTable("{ a = { a=\"foo\", b={ a=\"bar\", b=\"baz\" } } }");
  ptree inTree;
  LoadJSON("{ \"a\": { \"a\": \"foo\", \"b\": { \"a\": \"bar\", \"b\": \"baz\" } } }", inTree);
  Anki::Util::Lua_PushPTreeAsTable(state_, inTree);
  ptree outTree;
  Anki::Util::Lua_ToPTree(state_, outTree);
  CheckExpected(outTree);
}
  
  
TEST_F(TestLua, TestLuaCreateContext)
{
  Anki::Util::LuaContext testContext;
  // Just make sure we don't crash for now..
}
  
TEST_F(TestLua, TestLuaCreateScript)
{
  Anki::Util::LuaContext testContext;
  Anki::Util::LuaScript* testScript = nullptr;
  const string scriptPath = platform_->pathToResource(BaseStation::Platform::Resources, "/scripts/test/simpleScript.lua");
  testScript = testContext.CreateLuaScriptWithFile(scriptPath);
  EXPECT_TRUE(testScript != nullptr);
  delete testScript;
}
  
TEST_F(TestLua, TestLuaRunSimpleScript)
{
  Anki::Util::LuaContext testContext;
  Anki::Util::LuaScript* testScript = nullptr;
  const string scriptPath = platform_->pathToResource(BaseStation::Platform::Resources, "/scripts/test/simpleScript.lua");
  testScript = testContext.CreateLuaScriptWithFile(scriptPath);
  
  EXPECT_TRUE(testScript != nullptr);
  EXPECT_TRUE(testScript->IsAlive());
  testScript->Resume();
  EXPECT_FALSE(testScript->IsAlive());
  delete testScript;
}

TEST_F(TestLua, TestLuaSetBreakpoints)
{
  Anki::Util::LuaContext testContext;
  Anki::Util::LuaScript* testScript = nullptr;
  std::stringstream inStream;
  std::stringstream outStream;
  
  const string scriptPath = platform_->pathToResource(BaseStation::Platform::Resources, "/scripts/test/simpleScript.lua");
  testScript = testContext.CreateLuaScriptWithFile(scriptPath);
  testScript->Debugger().SetInStream(inStream);
  testScript->Debugger().SetOutStream(outStream);
  
  testScript->Debugger().SetFunctionBreakpoint("simpleFunctionOne");
  inStream << "c " << std::endl;
  testScript->Debugger().SetLineBreakpoint("./basestation/config/scripts/test/simpleScript.lua", 14);
  
  inStream << "c " << std::endl;
  testScript->Debugger().SetLineBreakpoint("./basestation/config/scripts/test/simpleScript.lua", 15);
  inStream << "c " << std::endl;
  
  EXPECT_TRUE(testScript != nullptr);
  EXPECT_TRUE(testScript->IsAlive());
  testScript->Resume();
  
  std::string inString = inStream.str();
  std::string outString = outStream.str();
  EXPECT_FALSE(testScript->IsAlive());
  // Should have hit some breakpoints.
  // TODO: verify we printed what we thought we were supposed to print.
  EXPECT_FALSE(outString.empty());
  delete testScript;
}


} //namespace BaseStation
