//
//  gameWithLuaScript.cpp
//  BaseStation
//
//  Created by Mark Pauley on 1/2/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "gameWithLuaScript.h"
#include "basestation/ui/platform/platform.h"
#include "basestation/utils/parameters.h"
#include "util/parsingConstants/parsingConstants.h"
#include "basestation/ui/messaging/messages/gameStateMessage.h"
#include "basestation/ui/messaging/messageQueue.h"

#include "util/lua/luaContext.h"
#include "util/lua/luaScript.h"
#include "util/lua/luaBridgeModule.h"
#include "basestation/luaModules/baseStationGameBridge.h"

#include "basestation/vehicle/script/vehicleScriptFactory.h"

#include "basestation/gameControllers/vehicleScriptController.h"
#include "basestation/vehicle/vehicle.h"

#include "metagame/gameConfig/gameSettings.h"

#include <boost/foreach.hpp>
#include <boost/property_tree/ptree.hpp>

namespace BaseStation {
  
  GameWithLuaScript::GameWithLuaScript(const MetaGame::GameSettings& settings, VehicleGameStatePtrMap &vehicleStates) : GameType(settings, vehicleStates),
  spacingScript_(NULL),
  vehicleScript_(NULL)
  {
    std::string basestationPath = SystemParameters::getInstance()->baseConfigurationPath_;
    std::string scriptsDir = basestationPath;
    scriptsDir += "basestation/config/scripts/";
    std::string myScript = settings.GetGameConfig()->get<string>(kP_LUA_SCRIPT);
    myScript = scriptsDir + myScript;
    luaContext_ = new Anki::Util::LuaContext();
    
    BaseStationGameBridge* gameBridge = new BaseStationGameBridge(this);
    luaContext_->RequireModule(*gameBridge);
    luaModules_.push_back(gameBridge); // TODO Consider moving the module into the luaScript to set context upon ::Resume
    
    luaScript_ = luaContext_->CreateLuaScriptWithFile(myScript);
    
    for (VehicleGameStatePtrMap::iterator iter = vehicleStates_.begin(); iter != vehicleStates_.end(); iter ++)
    {
      // For now, force all vehicles to drone mode.
      iter->second->GetParentVehicle()->operatingMode_ = VEHICLE_OPERATING_MODE_DRONE;
    }

  }
  
  GameWithLuaScript::~GameWithLuaScript()
  {
    Anki::Util::SafeDelete( luaScript_ );
    Anki::Util::SafeDelete( luaContext_ );
    
    for( auto& bridgeIt : luaModules_ )
    {
      Anki::Util::SafeDelete( bridgeIt );
    }
    luaModules_.clear();
  }
  
  // In-game logic goes here, we resume the script once per tick.
  void GameWithLuaScript::InGameUpdate()
  {
    for (VehicleGameStatePtrMap::iterator iter = vehicleStates_.begin(); iter != vehicleStates_.end(); iter ++)
    {
      // For now, force all vehicles to drone mode.
      iter->second->GetParentVehicle()->operatingMode_ = VEHICLE_OPERATING_MODE_DRONE;
    }
    
    // TODO make sure that script time is limited
    luaScript_->Resume();
    // not sure what should be on the stack here..
    // currently we return nothing

    if(!luaScript_->IsAlive()) {
      NormalGameEnd(false);
    }
    luaContext_->CollectGarbage();
  }
  
  // This could probably be an iterator..
  void GameWithLuaScript::GetVehicleIDs(vector<int>& outVector) const
  {
    outVector.clear();
    for (VehicleGameStatePtrMap::iterator iter = vehicleStates_.begin(); iter != vehicleStates_.end(); iter ++)
    {
      outVector.push_back(iter->first);
    }
  }
  
  VehicleGameState* GameWithLuaScript::vehicleStateForID(int vehicleID)
  {
    return vehicleStates_.at(vehicleID);
  }
  
  double GameWithLuaScript::timeInFormation()
  {
    if(spacingScript_ && VehicleScriptController::getInstance()->IsScriptRunning(spacingScript_)) {
      return spacingScript_->GetTimeInFormation();
    }
    else {
      return 0.0f;
    }
  }
  
  void GameWithLuaScript::GoalReached() {
    MessageQueue::getInstance()->AddMessageForUi(new GameStateMessage(GSMT_GAME_END_GOAL_REACHED, SEND_TO_ALL_ID, SEND_TO_ALL_ID));
    NormalGameEnd();
  }
  
  bool GameWithLuaScript::vehiclesAreInFormation()
  {
    if(spacingScript_ && VehicleScriptController::getInstance()->IsScriptRunning(spacingScript_)) {
      return spacingScript_->InFormation();
    }
    else {
      return false;
    }
  }
  
  // Game Script accessors
  void GameWithLuaScript::setVehicleFormation(VehicleFormation* spacingScript_)
  {
    
  }
  
  void GameWithLuaScript::updateEqualSpacingScript(boost::property_tree::ptree const &conf)
  {
    updateScriptWithConfig<VehicleFormation>(conf, spacingScript_, spacingScriptConf_);
  }
  
  void GameWithLuaScript::setVehicleScript(VehicleScript* vehicleScript) {
    
  }
  
  void GameWithLuaScript::updateVehicleScript(boost::property_tree::ptree const &conf)
  {
    updateScriptWithConfig<VehicleScript>(conf, vehicleScript_, vehicleScriptConf_);
  }
  
  template <typename T>
  void GameWithLuaScript::updateScriptWithConfig(boost::property_tree::ptree const &conf, T* &curScript, ptree &curScriptConf)
  {
    
    if(curScript && !VehicleScriptController::getInstance()->IsScriptRunning(curScript)) {
      // ignore the old script if it's not running.
      curScript = NULL;
    }
    
    // get script and start it if needed
    T *scriptToRemove = curScript;
    boost::optional<string> oldScriptTypeOptional = curScriptConf.get_optional<string>(kP_TYPE);
    curScriptConf.clear();
    curScriptConf = conf;
    
    bool deleteOldScript = curScriptConf.get(kP_DELETE_OLD_SCRIPT, false);
    boost::optional<string> newScriptTypeOptional = curScriptConf.get_optional<string>(kP_TYPE);
    
    if (oldScriptTypeOptional
        && newScriptTypeOptional
        && ((string)(*oldScriptTypeOptional)).compare(*newScriptTypeOptional) == 0
        && !deleteOldScript
        && curScript != NULL) {
      scriptToRemove = NULL;
      curScript->SetGameStateChanged(curScriptConf);
      /* 
       HELP! How do we log now?
      PRINT_NAMED_INFO("GameWithLuaScript.updateScriptWithConfig.update",
                       "Script type '%s' pointer class '%s' retained. Updated game state",
                       curScriptConf.get<string>(kP_TYPE).c_str(),
                       typeid(T).name());
       */
    } else {
      /*
       HELP! How do we log now?
      PRINT_NAMED_INFO("GameWithLuaScript.updateScriptWithConfig.create",
                       "Script type '%s' pointer class '%s' being created",
                       curScriptConf.get<string>(kP_TYPE).c_str(),
                       typeid(T).name());
       */
      curScript = static_cast<T*>(VehicleScriptFactory::getInstance()->SpawnScript(curScriptConf));
    }
    
    if (scriptToRemove != NULL){
      /*
       HELP! How do we log now?
      PRINT_NAMED_INFO("GameWithStates.updateScriptWithConfig.remove", "script type '%s' being deleted", typeid(*scriptToRemove).name());
       */
      VehicleScriptController::getInstance()->RemoveScript(scriptToRemove);
    }
  }
  
  
} // namespace Basestation

