/***************************************************
*  LuaBridgeModule
*  BaseStation
*
*  Created by Mark Pauley on 1/7/14.
*  Copyright (c) 2014 Anki. All rights reserved.
*
*  Description:
*  Interface for Lua runtime interfaces.
*  This allows Lua code to call into c and c++ code!
*
****************************************************/

#ifndef UTIL_LUA_LUABRIDGEMODULE_H_
#define UTIL_LUA_LUABRIDGEMODULE_H_

#include "util/lua/luaContext.h"

typedef int (*LuaBridgeModuleRegistrationFunction) (lua_State *L);

namespace Anki{ namespace Util {
  class ILuaBridgeModule {
    friend class LuaContext;
    
  public:
    virtual ~ILuaBridgeModule() {}
    virtual const std::string& GetModuleName() const = 0;
    
  protected:
    virtual LuaBridgeModuleRegistrationFunction GetRegistrationFunction() const = 0;
    
  };
} }

#endif
