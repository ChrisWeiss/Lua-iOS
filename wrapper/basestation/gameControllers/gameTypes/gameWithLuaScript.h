//
//  gameWithLuaScript.h
//  BaseStation
//
//  Created by Mark Pauley on 1/2/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef BASESTATION_GAMECONTROLLERS_GAMETYPES_GAMEWITHLUASCRIPT_H_
#define BASESTATION_GAMECONTROLLERS_GAMETYPES_GAMEWITHLUASCRIPT_H_


#include <boost/property_tree/ptree_fwd.hpp>
#include "basestation/vehicle/script/formation/vehicleFormation.h"
#include "basestation/gameControllers/gameTypes/gameType.h"

namespace Metagame {
  class GameSettings;
}

namespace Anki{ namespace Util {
  class LuaContext;
  class LuaScript;
  class ILuaBridgeModule;
} }

namespace BaseStation {

  class GameWithLuaScript : public GameType
  {
  public:
    
    static GameType *Create(const MetaGame::GameSettings& settings, VehicleGameStatePtrMap &vehicleStates) {
      return new GameWithLuaScript(settings, vehicleStates);
    };
    
    virtual ~GameWithLuaScript();
    
    // In game logic goes here
    virtual void InGameUpdate();
    
    void GetVehicleIDs(vector<int>& outVector) const;
    VehicleGameState* vehicleStateForID(int vehicleID);
    VehicleGameStatePtrMap vehicleStates() { return vehicleStates_; };
    bool   vehiclesAreInFormation();
    double timeInFormation();
    void GoalReached();
    
    void setVehicleFormation(VehicleFormation* formation);
    void updateEqualSpacingScript(boost::property_tree::ptree const &conf);
    
    void setVehicleScript(VehicleScript* vehicleScript);
    void updateVehicleScript(boost::property_tree::ptree const &conf);
    
    
  private:
  
    GameWithLuaScript(const MetaGame::GameSettings& settings, VehicleGameStatePtrMap &vehicleStates);

    Anki::Util::LuaContext *luaContext_;
    Anki::Util::LuaScript *luaScript_;
    vector<Anki::Util::ILuaBridgeModule*> luaModules_;
    
    template <typename T> void updateScriptWithConfig(boost::property_tree::ptree const &conf, T* &oldScript, ptree &oldScriptConf);
    
    VehicleFormation *spacingScript_;
    ptree spacingScriptConf_;
    
    VehicleScript *vehicleScript_;
    ptree vehicleScriptConf_;
  };
  
}


#endif
