//
//  LuaScript.cpp
//  BaseStation
//
//  Created by Mark Pauley on 1/3/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "util/lua/luaScript.h"
#include "util/logging/logging.h"
#include <lua/lua.hpp>
#include <cassert>

namespace Anki{ namespace Util {

  LuaScript::LuaScript(lua_State* parentContext, lua_State* luaThread)
  : parentContext_(parentContext)
  , luaThread_(luaThread)
  , debugger_(*this)
  {
    lua_pushthread(luaThread);
    luaThreadRef_ = luaL_ref(luaThread, LUA_REGISTRYINDEX);
    assert(lua_gettop(luaThread_) == 1);
    assert(lua_isfunction(luaThread_, -1));
  }
  
  LuaScript::~LuaScript() {
    luaL_unref(parentContext_, LUA_REGISTRYINDEX, luaThreadRef_);
  }
  
  
  void LuaScript::Resume() {
    lua_Debug debugInfo;
    int result;
    assert(luaThread_);
    result = lua_resume(luaThread_, parentContext_, 0);
    if(result > LUA_YIELD)
    {
      lua_getstack(luaThread_, 1, &debugInfo);
      lua_getinfo(luaThread_, "nSl", &debugInfo);
      PRINT_NAMED_ERROR("LuaScript.Resume.error", "Error resuming lua script %s:%d : %s ", debugInfo.source, debugInfo.currentline, lua_tolstring(luaThread_, -1, NULL));
    }
    else if(result == LUA_YIELD)
    {
#ifdef DEBUG
      lua_getstack(luaThread_, 1, &debugInfo);
      lua_getinfo(luaThread_, "nSl", &debugInfo);
      PRINT_NAMED_DEBUG("LuaScript.Resume.yield", "where = %s:%d", debugInfo.source, debugInfo.currentline);
#endif
    }
    else
    {
      PRINT_NAMED_INFO("LuaScript.Resume.finished", "");
    }
  }
  
  
  bool LuaScript::IsAlive() const {
    // We can resume if either the thread is in the yield state
    //  OR if the top thing on the thread's stack is a function.
    // Note that we know nothing about the arguments said function expects.
    bool ret = (lua_status(luaThread_) == LUA_YIELD);
    if(!ret) {
      ret = lua_isfunction(luaThread_, -1);
    }
    if(!ret) {
      ret = lua_iscfunction(luaThread_, -1);
    }
    return ret;
  }
  
  LuaDebugger& LuaScript::Debugger() {
    return debugger_;
  }
  
  lua_State* LuaScript::GetLuaThread() {
    return luaThread_;
  }
  
}
} // namespace