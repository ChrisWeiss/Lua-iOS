//
//  luaDebugger.cpp
//  BaseStation
//
//  Created by Mark Pauley on 6/21/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#include "util/lua/luaDebugger.h"
#include "util/lua/luaScript.h"
#include "util/helpers/templateHelpers.h"
#include <lua/lua.hpp>
#include <functional>
#include <unordered_map>
#include "util/helpers/includeIostream.h"
#include <fstream>
#include <sstream>
#include <assert.h>

namespace Anki{ namespace Util {
  
  // Maps from lua_State* to the responsible LuaDebugger thunks
  static std::unordered_map<const void*,std::function<void(lua_Debug*)>> sDebuggerMap;

  LuaDebugger::LuaDebugger(LuaScript& scriptToDebug, std::istream& inStream, std::ostream& outStream)
  : _script(scriptToDebug)
  , _inStream(&inStream)
  , _outStream(&outStream)
  , _stepBreakpoint(nullptr)
  , _debuggerState(DebuggerState::Idle)
  {
    lua_State* thread = _script.GetLuaThread();
    
    // Use std::bind to stash the target before saving to the debuggerMap
    auto hook = std::bind(&LuaDebugger::DebugHook, this, std::placeholders::_1);
    sDebuggerMap[thread] = hook;
  }
  
  LuaDebugger::~LuaDebugger()
  {
    lua_State* thread = _script.GetLuaThread();
    lua_sethook(thread, nullptr, 0, 0);
    sDebuggerMap.erase(thread);
    SafeDelete(_stepBreakpoint);
  }
  
#pragma mark - Interactive Debugger Commands
  // Set up the command-map for the interactive debugger
  typedef std::function<bool(LuaDebugger& debugger, std::ostream& ostream, const char*)> DebuggerCommand;
  typedef std::unordered_map<std::string, DebuggerCommand> DebuggerCommandMap;
  static DebuggerCommandMap GenerateCommands()
  {
    DebuggerCommandMap result;
    // 'continue'
    // 'c'
    auto continueLambda = [](LuaDebugger& debugger, std::ostream& ostream, const char* _) -> bool
    {
      ostream << "continue" << std::endl;
      debugger.Continue();
      return true;
    };
    result["continue"] = continueLambda;
    result["c"] = continueLambda;
    
    // 'step'
    // 's'
    auto stepLambda = [](LuaDebugger& debugger, std::ostream& ostream, const char* _) -> bool
    {
      debugger.Step();
      return true;
    };
    result["step"] = stepLambda;
    result["s"] = stepLambda;
    
    // 'next'
    // 'n'
    auto nextLambda = [](LuaDebugger& debugger, std::ostream& ostream, const char* _) -> bool
    {
      debugger.Next();
      return true;
    };
    result["next"] = nextLambda;
    result["n"] = nextLambda;
    
    // 'return'
    // 'r'
    auto returnLambda = [](LuaDebugger& debugger, std::ostream& ostream, const char* _) -> bool
    {
      ostream << "step to return" << std::endl;
      debugger.StepToReturn();
      return true;
    };
    result["return"] = returnLambda;
    result["r"] = returnLambda;
    
    // 'list'
    // 'l'
    auto listLambda = [](LuaDebugger& debugger, std::ostream& ostream, const char* args) -> bool
    {
      debugger.PrintSource(ostream);
      return false;
    };
    result["list"] = listLambda;
    result["l"] = listLambda;
    
    // 'where'
    // 'bt'
    auto whereLambda = [](LuaDebugger& debugger, std::ostream& ostream, const char* _) -> bool
    {
      debugger.PrintBacktrace(ostream);
      return false;
    };
    result["where"] = whereLambda;
    result["bt"] = whereLambda;
    
    // 'expression'
    // 'p'
    auto expressionLambda = [](LuaDebugger& debugger, std::ostream& ostream, const char* args) -> bool
    {
      // The first character is always a space (because of getline).
      debugger.EvaluateExpression(ostream, &args[1]);
      return false;
    };
    result["expression"] = expressionLambda;
    result["p"] = expressionLambda;
    
    // TODO: print the value of local variables (hint: use lua_getlocal, pass (2) for the stack frame).
    
    return result;
  }
  static DebuggerCommandMap sDebuggerCommands = GenerateCommands();
  
#pragma mark - Main Debugger Entry Point
  //
  // Main Debugger entry point.
  void LuaDebugger::EnterDebugger(std::istream &inStream, std::ostream &outStream)
  {
    SafeDelete(_stepBreakpoint);
    _debuggerState = DebuggerState::Debugging;
    assert(inStream.good());
    assert(outStream.good());
    std::string command;
    char argBuf[512];
    while (_debuggerState == DebuggerState::Debugging && inStream.good() && outStream.good()) {
      // Print the prompt.
      outStream << "> ";
      // Parse the command. (apparently this splits on whitespace already)
      // TODO: use readline? (may not work over the network..)
      inStream >> command;
      inStream.getline(argBuf, sizeof argBuf);
      // Execute the command.
      auto iter = sDebuggerCommands.find(command);
      if(iter != sDebuggerCommands.end()) {
        const DebuggerCommand& cmd = iter->second;
        bool shouldBreak = cmd(*this, outStream, argBuf);
        if (shouldBreak) {
          UpdateHooks();
          break; // Break out of the debugger if the command says so.
        }
      }
      else {
        outStream << "unknown command: " << command << std::endl;
      }
    }
  }
  
#pragma mark Control interfaces
  void LuaDebugger::Step()
  {
    lua_Debug debugInfo{0};
    _debuggerState = DebuggerState::Stepping;
    GetDebugStack(&debugInfo);
    GetDebugInfo(&debugInfo);
    SafeDelete(_stepBreakpoint);
    _stepBreakpoint = new LuaBreakpoint_Line(debugInfo.short_src, debugInfo.currentline + 1);
  }
  
  
  void LuaDebugger::Next()
  {
    _debuggerState = DebuggerState::Stepping;
    SafeDelete(_stepBreakpoint);
    _stepBreakpoint = new LuaBreakpoint_Line(LuaBreakpoint_Line::AnyLine());
  }
  
  void LuaDebugger::StepToReturn()
  {
    lua_Debug debug{0};
    _debuggerState = DebuggerState::Stepping;
    GetDebugStack(&debug);
    GetDebugInfo(&debug);
    SafeDelete(_stepBreakpoint);
    if(debug.name != nullptr) {
      // HACK: this is completely broken when you take recursion into account.
      _stepBreakpoint = new LuaBreakpoint_Function(debug.name);
    }
    else {
      // TODO: implement step to return
      //  via counting calls - returns (when we hit -1, we should break)
      Step();
    }
  }
  
  void LuaDebugger::Continue()
  {
    _debuggerState = DebuggerState::Continuing;
  }
  
  
#pragma mark - Display Interfaces
  void LuaDebugger::PrintCurrentStackFrame(std::ostream& stream) const
  {
    PrintStackFrame(stream, 1);
  }
  
  void LuaDebugger::PrintStackFrame(std::ostream& stream, const int frameNumber) const
  {
    lua_Debug debugInfo{0};
    GetDebugStack(&debugInfo, frameNumber);
    GetDebugInfo(&debugInfo);
    const char* name = "(anon)";
    if (debugInfo.name != nullptr) {
      name = debugInfo.name;
    }
    stream << name << " at " << debugInfo.short_src << ":" << debugInfo.currentline << std::endl;
  }
  
  void LuaDebugger::PrintBacktrace(std::ostream& stream) const
  {
    lua_Debug debugInfo{0};
    int stackLevel = 0 ;
    while (GetDebugStack(&debugInfo, stackLevel)) {
      stream << "(" << stackLevel << ") ";
      PrintStackFrame(stream, stackLevel);
      stackLevel++;
    }
  }
  
  void LuaDebugger::PrintBreakpoints(std::ostream& stream) const
  {
    // <Walk through the list of breakpoints and print them>
    int i = 0;
    for(const auto* bp : _breakpoints) {
      stream << "[" << i << "] " << bp << std::endl;
    }
  }
  
  static inline std::ostream& PrintLineNumber(std::ostream &stream, const unsigned int lineNumber, const bool printArrow = false)
  {
    static const char* preSpacer = "   ";
    static const char* spacer = "     ";
    std::stringstream lineNumberStr;
    lineNumberStr << lineNumber;
    const std::string& str = lineNumberStr.str();
    assert(str.length() < sizeof spacer);
    if(!printArrow) {
      stream << preSpacer;
    }
    else {
      stream << "-> ";
    }
    return stream << str << &(spacer[str.length()]);
  }
  
  // Currently, PrintSource requires that the un-compiled file be on the disk.
  //   We use a really piggy algorithm to read that file and dump it's contents.
  //   To do any better, we would probably need to cache the location of the line-breaks on a per-file basis. (pauley)
  void LuaDebugger::PrintSource(std::ostream &stream, const unsigned int context, const unsigned int stackFrame) const
  {
    lua_Debug debugInfo{0};
    if(GetDebugStack(&debugInfo)
       && GetDebugInfo(&debugInfo)
       && debugInfo.source != nullptr) {
      char lineBuf[512];
      
      // The source file name always starts with an '@' character, skip this.
      std::ifstream fileStream(&debugInfo.source[1]);
      if(!fileStream.good()) {
        stream << "< No Source (" << debugInfo.source << ")>" << std::endl;
        return;
      }

      int linesToSkip = (int)debugInfo.currentline - context - 1;
      while (linesToSkip > 0 && fileStream.getline(lineBuf, sizeof lineBuf)) {
        --linesToSkip;
      }
      
      // Print context (before)
      //  If linesToSkip is negative, it means that the pre-context was shortened
      //  because we are too close to the beginning of the file.
      int curLineNumber = (debugInfo.currentline - context) - linesToSkip;
      int linesToPrint = context + linesToSkip;
      while (linesToPrint > 0 && fileStream.getline(lineBuf, sizeof lineBuf)) {
        PrintLineNumber(stream, curLineNumber) << lineBuf << std::endl;
        --linesToPrint;
        ++curLineNumber;
      }
      // Print current line
      fileStream.getline(lineBuf, sizeof lineBuf);
      PrintLineNumber(stream, curLineNumber, true) << lineBuf << std::endl;
      ++curLineNumber;
      
      // Print context (after)
      //  getline will evaluate to false when we have hit the end of file.
      linesToPrint = context;
      while (linesToPrint > 0 && fileStream.getline(lineBuf, sizeof lineBuf)) {
        PrintLineNumber(stream, curLineNumber) << lineBuf << std::endl;
        --linesToPrint;
        ++curLineNumber;
      }
    }
  }
  
#pragma mark Evaluate Expression (and print)
  void LuaDebugger::EvaluateExpression(std::ostream& stream, const std::string& expression, bool printResult)
  {
    DebuggerState savedState = _debuggerState;
    _debuggerState = DebuggerState::EvaluatingExpression;
    UpdateHooks();
    
    lua_State* state = _script.GetLuaThread();
    lua_Debug dbg{0};
    GetDebugStack(&dbg);
    
    // Save the stack size so we can clear it for later.
    int stackSize = lua_gettop(state);

    // Lua makes it tough to get at the locals and upvalues (captured variables) of the current
    //  stack frame.  To get around this, we wrap the expression we wish to print in
    //  a function that pipes these values in through two methods.
    // 1) We pass all upvalues into the initial pcall, and stuff them into the _ENV table of our wrapper function
    // 2) We pass all locals directly into the wrapper function via the elipsis argument and unpack them directly
    //   as locals on the other side.  We cannot effectively change the value of locals with this technique.
    
    std::stringstream printExpression;
    // Slam the current upvalues into this function
    int numUpValues = 0;
    {
      GetDebugInfo(&dbg, "f");
      // Calling function is on the top of the stack now
      // Grab the upvalues from the current frame and stuff them into the function..
      const char* upvalName = nullptr;
      printExpression << "local _ENV = {}" << std::endl;
      printExpression << "local upvals = {...}" << std::endl;
      for(;;) {
        upvalName = lua_getupvalue(state, -1, numUpValues + 1);
        if(upvalName == nullptr) break;
        if(std::string("_ENV") == upvalName) {
          // if they gave us _ENV, capture the whole thing.
          printExpression << "_ENV = upvals[" << (numUpValues + 1) << "]" << std::endl;
        }
        else {
          // otherwise, suck the given upvalue over, by stuffing it into the special _ENV variable
          // lua uses to sandbox closures.
          printExpression << "_ENV[\"" << upvalName << "\"] = upvals[" << (numUpValues + 1) << "]" << std::endl;
        }
        numUpValues++;
      }
      // pop function from the stack.
      lua_remove(state, -(numUpValues + 1));
    }
    
    printExpression << "return function(...)" << std::endl;
    printExpression << "  local locals = {...}" << std::endl;
    
    // fetch locals
    int numLocals = 0;
    {
      const char* localName = nullptr;
      std::string sentinel("(*temporary)");
      for(;;) {
        localName = lua_getlocal(state, &dbg, numLocals + 1);
        if(sentinel == localName) break;
        printExpression << "local " << localName << " = locals[" << (numLocals + 1) << "]" << std::endl;
        numLocals++;
      }
      // stray nil on the stack
      lua_pop(state, 1);
    }
    // return allows for arbitrary expressions.
    printExpression << "  return " << expression << std::endl;
    printExpression << "end" << std::endl;
    
    const std::string debugWrappedExpression(printExpression.str());

    int status = luaL_loadstring(state, debugWrappedExpression.c_str());
    // there are now numLocals + numUpValues on the stack, followed by the function (which will return a function)
    if(status == LUA_OK && lua_isfunction(state, -1)) {
      // run the script (returns a function)
      for (int i = 0; i < numUpValues; i++) {
        lua_pushvalue(state, -(numUpValues + numLocals + 1));
        lua_remove(state, -(numUpValues + numLocals + 2));
      }
      status = lua_pcall(state, numUpValues, 1, 0);
    }
    if(status == LUA_OK && lua_isfunction(state, -1)) {
      // function is now on top of the stack (put it before the upvalue)
      lua_insert(state, stackSize + 1);
      status = lua_pcall(state, numLocals, LUA_MULTRET, 0);
    }
    if(status == LUA_OK) {
      // Wrap with a print statement, to print the result.
      luaL_checkstack(state, LUA_MINSTACK, "too many results to print");
      stream << ">>> ";
      lua_getglobal(state, "print");
      lua_insert(state, stackSize + 1);
      status = lua_pcall(state, lua_gettop(state) - stackSize - 1, 0, 0);
    }
    
    if(status != LUA_OK) {
      // report the error here
      const char* errorStr = lua_tolstring(state, -1, nullptr);
      if(errorStr == nullptr) errorStr = "( no description )";
      stream << "**Error: " << errorStr << std::endl;
    }
    lua_settop(state, stackSize);
    _debuggerState = savedState;
    UpdateHooks();
  }

#pragma mark - Function Breakpoints
  void LuaDebugger::SetFunctionBreakpoint(const std::string& functionName)
  {
    LuaBreakpoint_Function breakpoint(functionName);
    auto ret = _functionBreakpoints.emplace(breakpoint);
    if(ret.second) {
      // This was a new breakpoint.
      _breakpoints.push_back(&(*ret.first));
    }
    UpdateHooks();
  }

  void LuaDebugger::UnsetFunctionBreakpoint(const std::string& functionName)
  {
    const LuaBreakpoint_Function breakpoint(functionName);
    const auto& toDelete = _functionBreakpoints.find(breakpoint);
    if (toDelete != _functionBreakpoints.end()) {
      for(auto iter = _breakpoints.begin(); iter != _breakpoints.end(); ++iter) {
        if(*iter == &(*toDelete)) {
          _breakpoints.erase(iter);
          break;
        }
      }
      _functionBreakpoints.erase(toDelete);
    }
    UpdateHooks();
  }
  
  const LuaDebugger::LuaBreakpoint_Function* LuaDebugger::GetBreakpointForFunction(const std::string& functionName) const
  {
    const LuaBreakpoint_Function test(functionName);
    auto iter = _functionBreakpoints.find(test);
    if(iter != _functionBreakpoints.end()) {
      const LuaBreakpoint_Function& bp = *iter;
      return &bp;
    }
    return nullptr;
  }
  
  
#pragma mark - Line Number Breakpoints
  void LuaDebugger::SetLineBreakpoint(const std::string& fileName, const int lineNumber)
  {
    const LuaBreakpoint_Line breakpoint(fileName, lineNumber);
    auto ret = _lineBreakpoints.emplace(breakpoint);
    if(ret.second) {
      _breakpoints.push_back(&(*ret.first));
    }
    UpdateHooks();
  }
  
  void LuaDebugger::UnsetLineBreakpoint(const std::string &fileName, const int lineNumber)
  {
    const LuaBreakpoint_Line breakpoint(fileName, lineNumber);
    const auto& toDelete = _lineBreakpoints.find(breakpoint);
    if(toDelete != _lineBreakpoints.end()) {
      for(auto iter = _breakpoints.begin(); iter != _breakpoints.end(); ++iter) {
        if(*iter == &(*toDelete)) {
          _breakpoints.erase(iter);
          break;
        }
      }
      _lineBreakpoints.erase(toDelete);
    }
    UpdateHooks();
  }
  
  const LuaDebugger::LuaBreakpoint_Line* LuaDebugger::GetBreakpointForLine(const std::string& fileName, const int lineNumber) const
  {
    //TODO: allow matching on partial filename.
    const LuaBreakpoint_Line test(fileName, lineNumber);
    auto iter = _lineBreakpoints.find(test);
    if(iter != _lineBreakpoints.end()) {
      const LuaBreakpoint_Line& bp = *iter;
      return &bp;
    }
    return nullptr;
  }
  
#pragma mark - Internals
  bool LuaDebugger::GetDebugStack(lua_Debug *debugInfo,
                                  const unsigned int stackFrame) const
  {
    assert(debugInfo != nullptr);
    lua_State* thread = _script.GetLuaThread();
    const int status = lua_getstack(thread, stackFrame, debugInfo);
    return (status != 0);
  }
  
  bool LuaDebugger::GetDebugInfo(lua_Debug *debugInfo, const char* what) const
  {
    assert(debugInfo != nullptr);
    assert(what != nullptr);
    lua_State* thread = _script.GetLuaThread();
    const int status = lua_getinfo(thread, what, debugInfo);
    return (status != 0);
  }
  
#pragma mark - Debug Hook
  extern "C"
  {
    static void DebugHookTrampoline(lua_State* thread, lua_Debug* ar)
    {
      auto iter = sDebuggerMap.find(thread);
      if(iter != sDebuggerMap.end()) {
        auto debugHook = iter->second;
        // Recall that we used std::bind to stash the corresponding 'this' in the constructor when we inserted into the debuggerMap
        debugHook(ar);
      }
    }
  }
  
  void LuaDebugger::UpdateHooks()
  {
    lua_State* thread = _script.GetLuaThread();
    int mask = 0;
    if(_debuggerState != DebuggerState::EvaluatingExpression) {
      if(!_functionBreakpoints.empty()) {
        mask |= LUA_MASKCALL;
      }
      if(!_lineBreakpoints.empty()) {
        mask |= LUA_MASKLINE;
        //mask |= LUA_MASKCOUNT;
      }
      if(_debuggerState == DebuggerState::Stepping) {
        mask |= LUA_MASKLINE;
        mask |= LUA_MASKRET;
      }
    }
    lua_sethook(thread, DebugHookTrampoline, mask, 0);
  }
  
  void LuaDebugger::DebugHook(lua_Debug *ar)
  {
    // Lua is calling us back, figure out why we're here.
    // Either we're stepping, or we've hit a breakpoint.
    
    // Optimize: GetDebugInfo may be expensive..
    __attribute__((unused)) const int success = GetDebugInfo(ar);
    assert(success);
    assert(_inStream != nullptr);
    assert(_outStream != nullptr);
    
    // If our streams are bogus, give up!
    if(!_inStream->good())return;
    if(!_outStream->good())return;
    
    // Check to see if we're stepping.
    if(_debuggerState == DebuggerState::Stepping) {
      assert(_stepBreakpoint != nullptr);
      bool matched = false;
      if(_stepBreakpoint) {
        const LuaBreakpoint& curLineBP = *_stepBreakpoint;
        LuaBreakpoint_Line testBP(ar->short_src, ar->currentline);
        matched = (curLineBP == testBP || curLineBP == LuaBreakpoint_Line::AnyLine());
      }
      
      if(matched) {
        PrintSource(*_outStream);
        EnterDebugger(*_inStream, *_outStream);
        return;
      }
    }
    
    // Otherwise, there must be a good reason we're here.
    //  The event will tell us..
    switch (ar->event) {
      case LUA_HOOKTAILCALL:
        // HOOKTAILCALL => We are about to perform a tail-call
        // (so the previous frame won't appear in the stack!)
        //  For now, treat this just like a normal call.
      case LUA_HOOKCALL:
        // HOOKCALL => We're calling a function.
        if(ar->name) {
          const LuaBreakpoint_Function* funcBP = GetBreakpointForFunction(ar->name);
          if(funcBP != nullptr) {
            funcBP->Hit();
            PrintBacktrace(*_outStream);
            PrintSource(*_outStream);
            EnterDebugger(*_inStream, *_outStream);
          }
        }
        break;
        
      case LUA_HOOKLINE:
        // HOOKLINE => We're executing a new lua source line.
        {
          const LuaBreakpoint_Line *lineBP = GetBreakpointForLine(ar->short_src, ar->currentline);
          if(lineBP != nullptr) {
            lineBP->Hit();
            PrintBacktrace(*_outStream);
            PrintSource(*_outStream);
            EnterDebugger(*_inStream, *_outStream);
          }
        }
        break;
        
      case LUA_HOOKCOUNT:
        // HOOKCOUNT => We skipped a few lines.
        // unused for now (maybe useful for line breakpoint optimization)
        break;
        
      case LUA_HOOKRET:
        // HOOKRET => We are returning from a function.
        if(_debuggerState == DebuggerState::Stepping) {
          if(_stepBreakpoint != nullptr && ar->name != nullptr) {
            const LuaBreakpoint_Function testBP = LuaBreakpoint_Function(ar->name);
            if(*_stepBreakpoint == testBP) {
              PrintBacktrace(*_outStream);
              PrintSource(*_outStream);
              EnterDebugger(*_inStream, *_outStream);
            }
          }
        }
        break;
        
      default:
        break;
    }
    
  }

} // namespace
} // namespace
