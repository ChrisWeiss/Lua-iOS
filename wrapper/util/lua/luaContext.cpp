//
//  LuaContext.cpp
//  BaseStation
//
//  Created by Mark Pauley on 1/3/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "util/lua/luaContext.h"
#include "util/lua/luaScript.h"
#include "util/lua/luaBridgeModule.h"
#include "util/lua/luaUtils.h"
#include <lua/lua.hpp>

#include "util/logging/logging.h"
#include "util/parsingConstants/parsingConstants.h"
#include <assert.h>



namespace Anki{ namespace Util {
  
  LuaContext::LuaContext() {
    luaState_ = luaL_newstate();
    luaL_openlibs(luaState_);
  }
  
  LuaContext::~LuaContext() {
    lua_close(luaState_);
  }
  
  LuaScript* LuaContext::CreateLuaScriptWithFile(const std::string& fileName)
  {
    // Load script from file.
    if(luaL_loadfile(luaState_, fileName.c_str())) {
      __attribute__((unused)) const char* errorString = lua_tolstring(luaState_, 1, NULL);
      PRINT_NAMED_ERROR("LuaContext.CreateLuaScriptWithFile.loadFile", "%s", errorString);
      assert(!errorString);
      CollectGarbage();
      return nullptr;
    }
    // Script is now on the top of the stack, call it.
    if(lua_pcall(luaState_, 0, 1, 0)) {
      __attribute__((unused)) const char* errorString = lua_tolstring(luaState_, 1, NULL);
      PRINT_NAMED_ERROR("LuaContext.CreateLuaScriptWithFile.pcall", "%s", errorString);
      assert(!errorString);
      CollectGarbage();
      return nullptr;
    }
    // At this point, the script was successfully run.
    // We need to do a bit of work to make sure it did so correctly.
    
    if(lua_gettop(luaState_) != 1) {
      // DEBUGGER ENTRY HERE
      PRINT_NAMED_ERROR("LuaContext.CreateLuaScriptWithFile", "Expected at least one return item from script %s.", fileName.c_str());
      lua_settop(luaState_, 0);
      CollectGarbage();
      return nullptr;
    }
    
    lua_State* luaThreadState = NULL;

    // We expect the loaded program to return an entry point, either a thread or a function
    // Either way, we'll make sure the result is a thread (a resumable object).
    if(lua_isthread(luaState_, -1)) {
      luaThreadState = lua_tothread(luaState_, 1);
    }
    else if(lua_isfunction(luaState_, -1)) {
      // If we got a function, we just make a new thread and pop the function ref over onto
      // that new thread's stack (using xmove).
      luaThreadState = lua_newthread(luaState_);
      // We push a copy of the function to the top of the stack and then pop it over to the new thread.
      lua_pushvalue(luaState_, -2);
      lua_xmove(luaState_, luaThreadState, 1);
    }
    else {
      // DEBUG ENTRY HERE.
      lua_settop(luaState_, 0);
      PRINT_NAMED_ERROR("LuaContext.CreateLuaScriptWithFile", "Script didn't return a thread or function!");
      CollectGarbage();
      return nullptr;
    }
    
    // create script and discard any pending data from stack
    LuaScript* newScript = new LuaScript(luaState_, luaThreadState);
    lua_settop(luaState_, 0);

    CollectGarbage();
    
    return newScript;
  }
  
  
  void LuaContext::CollectGarbage() {
    lua_gc(luaState_, LUA_GCCOLLECT, 0);
  }
  
  void LuaContext::RequireModule(const ILuaBridgeModule& module) {
    luaL_requiref(luaState_, module.GetModuleName().c_str(), module.GetRegistrationFunction(), 1);
    lua_settop(luaState_, 0);
  }
  
  void LuaContext::SetGlobal(const std::string& globalName, void* value) {
    lua_pushlightuserdata(luaState_, value);
    lua_setglobal(luaState_, globalName.c_str());
  }
  
  void LuaContext::ClearGlobal(const std::string& globalName) {
    lua_pushnil(luaState_);
    lua_setglobal(luaState_, globalName.c_str());
  }
  
} }
