//
//  BaseStationGameBridge.cpp
//  BaseStation
//
//  Created by Mark Pauley on 1/4/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "basestation/luaModules/baseStationGameBridge.h"
#include "util/lua/luaUtils.h"

#include "basestation/utils/timer.h"
#include "basestation/ui/messaging/messages/gameStateMessage.h"
#include "basestation/ui/messaging/messageQueue.h"
#include "basestation/gameControllers/gameTypes/gameWithLuaScript.h"

#include "basestation/vehicle/vehicle.h"
#include <lua/lua.hpp>

extern "C" {
  // Forward declarations of registration routines go here.
  static int luaopen_BaseStationGame(lua_State* state);
}


namespace BaseStation {
  static BaseStationGameBridge *_currentGame;
  
  BaseStationGameBridge::BaseStationGameBridge(GameWithLuaScript *game) {
    game_ = game;
    if(_currentGame == NULL) {
      // FIXME: this is super weak (pauley)
      //  I know there must be a way to sneak this object into the context somehow.
      //  This is actually a general problem for modules, we need to stash an object that gets
      //  Popped out on the other side (when lua calls us back).
      _currentGame = this;
    }
  }
  
  BaseStationGameBridge::~BaseStationGameBridge() {
    if(_currentGame == this) {
      _currentGame = NULL;
    }
  }
  
  const std::string& BaseStationGameBridge::GetModuleName() const {
    static std::string _moduleName = std::string("BaseStationGame");
    return _moduleName;
  }
  
  LuaBridgeModuleRegistrationFunction BaseStationGameBridge::GetRegistrationFunction() const {
    return &luaopen_BaseStationGame;
  }
  
}


#pragma mark Lua Module Interface
using namespace BaseStation;
extern "C" {
 /**********************************************************
*  Forward declarations of Lua library entrypoints go here. *
 ***********************************************************/
  
// void goalReached(void) sends goal reached message
static int goalReached(lua_State* state);

// float gameTime(void) -> returns game time in seconds
static int gameTime(lua_State* state);

///
// Vehicle stuff
///
// int[] vehicleIDs(void) -> returns list of vehicleIDs
static int vehicleIDs(lua_State* state);

// bool allVehiclesAreLocalized(void) -> returns true if all vehicles are localized
static int allVehiclesAreLocalized(lua_State* state);

// bool vehicleIsLocalized(int vehicleID) -> returns true if vehicle with ID vehicleID is localized
static int vehicleIsLocalized(lua_State* state);

// double vehicleSpeed(int vehicleID) -> returns speed for vehicle with ID vehicleID
static int vehicleSpeed(lua_State* state);

// double vehicleLane(int vehicleID) -> returns lane position for vehicle with ID vehicleID
static int vehicleLane(lua_State* state);

// int vehicleKills(int vehicleID) -> returns the number of kills for the vehicle with ID vehicleID
static int vehicleKills(lua_State* state);

// bool vehicleIsAI(int vehicleID) -> returns true if the vehicle with ID vehicleID is controlled by the AI
static int vehicleIsAI(lua_State* state);


///
// Formation stuff
///
// bool areVehiclesInFormation(void) -> returns true if vehicles are in formation
static int areVehiclesInFormation(lua_State* state);

// int timeInFormation(void) -> returns time that the vehicles have been in the current formation
static int timeInFormation(lua_State* state);

///
// Script stuff
///
// void setEqualSpacingScript(table t) -> sets the game's script based on the table (left over from json tutorial game)
int setEqualSpacingScript(lua_State* state);

// void setVehicleScript(table t) -> sets the game's vehicle script based on the table (left over from json tutorial game)
int setVehicleScript(lua_State* state);

}

// Library registration struct must be a C struct
//  All of these entries are key-value pairs.
//  The first value will be the function name,
//  The second value will be the entry point.
static const struct luaL_Reg _BaseStationGameBridgeLib[] = {
  {"goalReached", goalReached},
  {"gameTime", gameTime},
  
  {"vehicleIDs", vehicleIDs},
  {"allVehiclesAreLocalized", allVehiclesAreLocalized},
  {"vehicleIsLocalized", vehicleIsLocalized},
  {"vehicleSpeed", vehicleSpeed},
  {"vehicleLane", vehicleLane},
  {"vehicleKills", vehicleKills},
  {"vehicleIsAI", vehicleIsAI},
  //spawnScriptForVehicle?
  //numScriptsRunningForVehicle?
  //distanceBetween vehicles?
  
  {"areVehiclesInFormation", areVehiclesInFormation},
  {"timeInFormation", timeInFormation},
  
  {"setEqualSpacingScript", setEqualSpacingScript},
  {"setVehicleScript", setVehicleScript},
  
  {NULL, NULL}
};

#pragma mark Library Entry Point

int luaopen_BaseStationGame(lua_State *state) {
  luaL_newlib(state, _BaseStationGameBridgeLib);
  return 1;
}

#pragma mark Lua Module Implementation

/*  This is expected to be an example Lua module.
 *   We should definitely break this guy up into separate modules for clarity.
 *  NOTE: the values are passed in on the stack of the lua_State*
 *    The values are in-order (that is stack[1] == arg1, stack[2] == arg2, etc.)
 *    
 *
 *  Example: BaseStationGame.doFoo("one", "two", 3, four)
 *  lua_tointegerx(state, 3, &isInt) => returns 3, isInt is true.
 *  lua_tointegerx(state, 1, &isInt) => returns 0, isInt is false.
 *  lua_tostring(state, 1) => returns "one", does not need to be free'd (lua owns the string storage).
*/

int goalReached(lua_State* state) {
  assert(lua_gettop(state) == 0);
  _currentGame->GetGame()->GoalReached();
  //PRINT_NAMED_EVENT("Game.End", "GOALREACHED"); (How to log to DAS?)
  
  return 0;
}

int gameTime(lua_State* state) {
  double gameTime = BaseStationTimer::getInstance()->GetCurrentTimeInSeconds();
  lua_pushnumber(state, gameTime);

  return 1;
}

// This could maybe use some caching, so we don't have to build the vector more than once per tick..
int vehicleIDs(lua_State* state) {
  lua_newtable(state);
  int i = 1;
  vector<int> vehicleIDVector;
  _currentGame->GetGame()->GetVehicleIDs( vehicleIDVector );
  for (const int& vehicleID : vehicleIDVector)
  {
    lua_pushinteger(state, i);
    lua_pushinteger(state, vehicleID);
    lua_settable(state, -3);
    i++;
  }
  
  // the table containing all of the vehicleID's is now on top of the stack.
  return 1;
}

int allVehiclesAreLocalized(lua_State* state) {
  bool result = true;
  VehicleGameStatePtrMap gameStatePtrMap = _currentGame->GetGame()->vehicleStates();
  for (VehicleGameStatePtrMap::iterator iter = gameStatePtrMap.begin();
       iter != gameStatePtrMap.end();
       iter ++) {
    if (!iter->second->IsLocalized()) {
      result = false;
    }
  }

  lua_pushboolean(state, result);
  return 1;
}

int vehicleIsLocalized(lua_State* state) {
  luaL_checkint(state, 1);
  
  int vehicleID = (int)lua_tointegerx(state, 1, NULL);
  lua_pop(state, 1);
  bool isLocalized = _currentGame->GetGame()->vehicleStateForID(vehicleID)->GetParentVehicle()->vehicleState_.localized_;
  
  lua_pushboolean(state, isLocalized);
  return 1;
}

int vehicleSpeed(lua_State* state) {
  luaL_checkint(state, 1);
  
  int vehicleID = (int)lua_tointegerx(state, 1, NULL);
  lua_pop(state, 1);
  double speed = _currentGame->GetGame()->vehicleStateForID(vehicleID)->GetTrackerValue(TT_CURRENT_SPEED);
  lua_pushnumber(state, speed);
  return 1;
}

// FIXME: This returns between 0.0 and 1.0 this seems ok.  It seems like the lane position should be an int, minLane -> maxLane.
int vehicleLane(lua_State* state) {
  luaL_checkint(state, 1);
  
  int vehicleID = (int)lua_tointegerx(state, 1, NULL);
  lua_pop(state, 1);
  Vehicle* vehicle = _currentGame->GetGame()->vehicleStateForID(vehicleID)->GetParentVehicle();
  double lanePos = vehicle->vehicleState_.vehiclePosition_.GetCurrLane();
  
  lua_pushnumber(state, lanePos);
  return 1;
}

int vehicleKills(lua_State* state) {
  luaL_checkint(state, 1);
  
  int vehicleID = (int)lua_tointegerx(state, 1, NULL);
  lua_pop(state, 1);
  int result = _currentGame->GetGame()->vehicleStateForID(vehicleID)->GetTrackerValue(TT_KILLS);
  
  lua_pushinteger(state, result);
  return 1;
}

int vehicleIsAI(lua_State* state) {
  luaL_checkint(state, 1);
  
  int vehicleID = (int)lua_tointegerx(state, 1, NULL);
  lua_pop(state, 1);
  bool result = (_currentGame->GetGame()->vehicleStateForID(vehicleID)->GetParentVehicle()->operatingMode_ == VEHICLE_OPERATING_MODE_AI);
  
  lua_pushboolean(state, result);
  return 1;
}

int areVehiclesInFormation(lua_State* state) {
  bool result = _currentGame->GetGame()->vehiclesAreInFormation();
  lua_pushboolean(state, result);
  return 1;
}

int timeInFormation(lua_State* state) {
  double result = 0;
  result = _currentGame->GetGame()->timeInFormation();
  lua_pushnumber(state, result);
  return 1;
}

int setEqualSpacingScript(lua_State* state) {
  ptree scriptConf;
  Lua_ToPTree(state, scriptConf);
  _currentGame->GetGame()->updateEqualSpacingScript(scriptConf);
  lua_pop(state, 1);
  
  return 0;
}

int setVehicleScript(lua_State* state) {
  ptree scriptConf;
  Lua_ToPTree(state, scriptConf);
  _currentGame->GetGame()->updateVehicleScript(scriptConf);
  lua_pop(state, 1);
  
  return 0;
}

