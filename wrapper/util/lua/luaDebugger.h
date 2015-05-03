//
//  luaDebugger.h
//  BaseStation
//
//  Created by Mark Pauley on 6/21/14.
//  Copyright (c) 2014 Anki. All rights reserved.
//

#ifndef UTIL_LUA_LUADEBUGGER_H_
#define UTIL_LUA_LUADEBUGGER_H_

#include "util/helpers/noncopyable.h"
#include "util/helpers/includeIostream.h"
#include <set>
#include <vector>
#include <string>


struct lua_State;
struct lua_Debug;

namespace Anki{ namespace Util {
  
class LuaScript;
class LuaDebugger : public Anki::Util::noncopyable {
#pragma mark - Interfaces
  //
  // Interfaces
  //
public:
  LuaDebugger(LuaScript& scriptToDebug, std::istream& inStream = std::cin, std::ostream& outStream = std::cout);
  ~LuaDebugger();
  
  void SetInStream(std::istream& inStream) { _inStream = &inStream; };
  void SetOutStream(std::ostream& outStream) { _outStream = &outStream; };
  
#pragma mark - REPL interfaces
  void EnterDebugger(std::istream& inStream, std::ostream& outStream);
  
#pragma mark Control interfaces
  //
  // Control interfaces
  //
  // Step until the next line in current source file
  void Step();
  
  // Step until the next line in any source file
  void Next();

  // Step until the return to the calling function
  void StepToReturn();
  
  // Continue (exit from the current breakpoint)
  void Continue();
  
#pragma mark Breakpoint interfaces
  //
  // Breakpoint Interfaces
  //
  void SetFunctionBreakpoint(const std::string& functionName);
  void UnsetFunctionBreakpoint(const std::string& functionName);
  
  void SetLineBreakpoint(const std::string& fileName, const int lineNumber);
  void UnsetLineBreakpoint(const std::string& fileName, const int lineNumber);
  
  void UnsetBreakpoint(int breakpointIndex);

#pragma mark Display interfaces
  //
  // Display interfaces
  //
  void PrintCurrentStackFrame(std::ostream& stream) const;
  void PrintStackFrame(std::ostream& stream, const int frameNumber) const;
  void PrintBacktrace(std::ostream& stream) const;

  void PrintBreakpoints(std::ostream& stream) const;
  
  // List source for current stack (+/- context # lines)
  void PrintSource(std::ostream& stream,
                   const unsigned int context = 4,
                   const unsigned int stackFrame = 0) const;
  
  // Evaluate the given string as an expression in the current stack frame (and print the result)
  void EvaluateExpression(std::ostream& stream, const std::string& expression, bool printResult = false);
  

  //
  // End of Interfaces
  //
  
  
#pragma mark - Internals
  //
  // Internals
  //
protected:
  bool GetDebugStack(lua_Debug* debugInfo, const unsigned int stackFrame = 0) const;
  bool GetDebugInfo(lua_Debug* debugInfo, const char* what = "Snul") const;
  
#pragma mark Breakpoints in sub-header

#include "util/lua/luaDebugger_LuaBreakpoint.hpp"

private:
  void UpdateHooks();
  void DebugHook(lua_Debug* ar);
  
  const LuaBreakpoint_Function* GetBreakpointForFunction(const std::string& functionName) const;
  const LuaBreakpoint_Line* GetBreakpointForLine(const std::string& fileName, const int lineNumber) const;
  
  enum class DebuggerState {
    Idle,
    Debugging,
    Stepping,
    EvaluatingExpression,
    Continuing,
    Count
  };
  
  // Script that owns us..
  LuaScript& _script;
  
  std::istream* _inStream;
  std::ostream* _outStream;
  
  std::vector<const LuaBreakpoint*> _breakpoints;
  std::set<LuaBreakpoint_Function> _functionBreakpoints;
  std::set<LuaBreakpoint_Line> _lineBreakpoints;
  LuaBreakpoint *_stepBreakpoint;
  DebuggerState _debuggerState;
}; // LuaDebugger
  
  
} // namespace
} // namespace

#endif /* defined(UTIL_LUA_LUADEBUGGER_H_) */
