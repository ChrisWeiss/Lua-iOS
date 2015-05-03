/************************************************************************
*  LuaContext.h
*  BaseStation
*
*  Created by Mark Pauley on 1/3/14.
*  Copyright (c) 2014 Anki. All rights reserved.
*
*  Description:
*  - An object wrapper for a Lua Context.
*  - Contains loaded libraries (so that they can be shared accross scripts)
*  - Can be used to load bridge modules via RequireLib (see ILuaBridgeModule)
*  - Can be used to manually do garbage collection on the Lua context.
*  - Spawns new scripts with the CreateLuaScriptWith* methods
*  - Can be used to set global values (visible from all scripts spawned by this context)
*  - Will close the Lua Context and notify all spawned scripts of termination upon destruction.
*
*
************************************************************************/

#ifndef UTIL_LUA_LUACONTEXT_H_
#define UTIL_LUA_LUACONTEXT_H_


#include <string>
#include "util/helpers/noncopyable.h"

struct lua_State;
namespace Anki{ namespace Util {
  class LuaScript;
  class ILuaBridgeModule;
  
  class LuaContext : public Anki::Util::noncopyable {
    
  public:
    LuaContext();
    ~LuaContext();
    
    LuaScript* CreateLuaScriptWithFile(const std::string& fileName);
    
    void RequireModule(const ILuaBridgeModule& module);
    void CollectGarbage();

    void SetGlobal(const std::string& globalName, void* value);
    void ClearGlobal(const std::string& globalName);
    
  private:
    
    lua_State *luaState_;
  };

} }

#endif /* defined(__BaseStation__LuaContext__) */
