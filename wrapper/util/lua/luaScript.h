/***************************************************************************************************
*  LuaScript
*  BaseStation
*
*  Created by Mark Pauley on 1/3/14.
*  Copyright (c) 2014 Anki. All rights reserved.
*
*  Description:
*  - A wrapper for a Lua Coroutine. Essentially represents a stack and a context.
*  - Must be created via a Lua Context. (LuaContext::CreateLuaScriptWith*)
*  - Wraps a Lua-thread (lua_State) and points at the parent context. 
*  - Can share state with sibling contexts through the parent context.
*    other sibling contexts.
*  - Runs the script using the function 'Resume' (currently returns no values)
*  - Stays alive while the script exits with 'coroutine.yield'
*  - Closes the thread (and releases resources) upon destruction.
*
*
***************************************************************************************************/

#ifndef UTIL_LUA_LUASCRIPT_H_
#define UTIL_LUA_LUASCRIPT_H_

#include "util/helpers/noncopyable.h"
#include "util/lua/luaDebugger.h"

struct lua_State;

namespace Anki{ namespace Util {
  class LuaContext;
  class LuaDebugger;
  class LuaScript : public Anki::Util::noncopyable {
    friend class LuaContext;
  protected:
    // Constructor may only be called by a LuaContext object (the parent)
    //   luaThread is assumed to have been created from parentContext
    LuaScript(lua_State* parentContext, lua_State* luaThread);
    
  public:
    // Will terminate and garbage collect the luaThread
    //  The thread itself will not have a chance to run.
    ~LuaScript();
    
    // Run the lua thread until either a coroutine.yield() is encountered
    //  or the top stack frame is returned from.
    void Resume();
    
    // Returns true if this script can be resumed.
    //  Will return NO if either the thread was fully returned from
    //  or if the parent context has been destroyed.
    bool IsAlive() const;
    
    LuaDebugger& Debugger();
    
    lua_State* GetLuaThread();
    
  private:
    lua_State* parentContext_;
    lua_State* luaThread_;
    int luaThreadRef_;
    LuaDebugger debugger_;
  };
  
}
} // namespace

#endif
