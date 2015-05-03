/********************************************************
*  BaseStationGameBridge
*
*  Created by Mark Pauley on 1/4/14.
*  Copyright (c) 2014 Anki. All rights reserved.
*
*  Description:
*   Bridge module from a BaseStation game object to a lua script.
*   Not currently expected to be thread-safe.
*
*   Probably needs documentation once finished,
*   For now, please look at _BaseStationGameBridgeLib[]
*   to see the full list of lua entry points.
********************************************************/
 
#ifndef UTIL_LUA_LUAGAMEBRIDGE_H_
#define UTIL_LUA_LUAGAMEBRIDGE_H_

#include "util/lua/luaBridgeModule.h"


namespace BaseStation {
  class GameWithLuaScript;
  
  class BaseStationGameBridge : public Anki::Util::ILuaBridgeModule {
  public:
    BaseStationGameBridge(GameWithLuaScript *game);
    virtual ~BaseStationGameBridge() override;
    virtual const std::string& GetModuleName() const;
    inline GameWithLuaScript* GetGame() { return game_; };
    
  protected:
    virtual LuaBridgeModuleRegistrationFunction GetRegistrationFunction() const;
    
  private:
    GameWithLuaScript *game_;
  };
  
} // namespace BaseStation

#endif
