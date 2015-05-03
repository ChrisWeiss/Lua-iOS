//
//  testGameWithLuaScript.cpp
//  BaseStation
//
//  Created by Mark Pauley on 1/2/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//
//  --gtest_filter=TestGameWithLuaScript*

#define private public
#define protected public


#include "basestation/tests/basestationFullGoogleTest.h"
#include "util/parsingConstants/parsingConstants.h"
#include "basestation/ui/messaging/messageQueue.h"
#include "basestation/ui/messaging/messages/inputControllerMessage.h"
#include "basestation/ui/messaging/messages/itemMessage.h"
#include "basestation/ui/messaging/messages/gameStateMessage.h"

namespace BaseStation {

/**
 * test for lua-script based game
 *
 * @author pauley
 */
class TestGameWithLuaScript : public BasestationFullGoogleTest
{
public:

};
  
  TEST_F(TestGameWithLuaScript, TestSimpleGameWithLuaScript)
  {
    
    SetCustomGameToPlay("simple_script");
    SetTestRunTime(1000);
    InitializeGame();
    AdvanceToGameStart();
    
    _saveUiMessages = true;
    // I think this will fail if there was an error starting the game..
    for (unsigned int i = 0; i < 60; ++i)
      TickBasestation();
    
    bool goalReached = false;
    for (unsigned int i = 0; i < _savedUiMessages.size(); ++i)
    {
      UiMessage * uiMessage = _savedUiMessages[i];
      switch (uiMessage->type_)
      {
        case UMCT_GAME_STATE_MESSAGE:
        {
          GameStateMessage * gameStateMessage = (GameStateMessage*)uiMessage;
          if (gameStateMessage->gameStateMessageType_ == GSMT_GAME_END_GOAL_REACHED)
            goalReached = true;
        }
          break;
          
        default:
          break;
      }
    }
    
    EXPECT_TRUE(goalReached);
}

TEST_F(TestGameWithLuaScript, TestTimedGameWithLuaScript)
{
  string configRoot = "./";
  char* configRootChars = getenv(CONFIGROOT);
  if (configRootChars != NULL)
    configRoot = configRootChars;
  
  SetCustomGameToPlay("timed_script");

  InitializeGame();
  SetTestRunTime(1000);
  AdvanceToGameStart();
  
  _saveUiMessages = true;
  // I think this will fail if there was an error starting the game..
  for (unsigned int i = 0; i < 60; ++i)
    TickBasestation();
  
  bool goalReached = false;
  for (unsigned int i = 0; i < _savedUiMessages.size(); ++i)
  {
    UiMessage * uiMessage = _savedUiMessages[i];
    switch (uiMessage->type_)
    {
      case UMCT_GAME_STATE_MESSAGE:
      {
        GameStateMessage * gameStateMessage = (GameStateMessage*)uiMessage;
        if (gameStateMessage->gameStateMessageType_ == GSMT_GAME_END_GOAL_REACHED)
          goalReached = true;
      }
        break;
        
      default:
        break;
    }
  }
  
  EXPECT_FALSE(goalReached);
  
  for (unsigned int i = 0; i < 200; ++i)
    TickBasestation();
  
  for (unsigned int i = 0; i < _savedUiMessages.size(); ++i)
  {
    UiMessage * uiMessage = _savedUiMessages[i];
    switch (uiMessage->type_)
    {
      case UMCT_GAME_STATE_MESSAGE:
      {
        GameStateMessage * gameStateMessage = (GameStateMessage*)uiMessage;
        if (gameStateMessage->gameStateMessageType_ == GSMT_GAME_END_GOAL_REACHED)
          goalReached = true;
      }
        break;
        
      default:
        break;
    }
  }
  
  EXPECT_TRUE(goalReached);
}
  
}  // namespace BaseStation
