//
// ldoc launcher
//

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <shlwapi.h>
#include <pathcch.h>
#include <string>
#include <vector>

#include <lua.hpp>

#define appName "ldoc.exe"

static std::string WideCharToUTF8(LPCWSTR text) {
  if (!text) return std::string();
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, text, -1, NULL, 0, NULL, NULL);
  if (size_needed <= 0) return std::string();
  std::vector<char> buffer(size_needed);
  WideCharToMultiByte(CP_UTF8, 0, text, -1, buffer.data(), size_needed, NULL, NULL);
  return std::string(buffer.data());
}

static const char *setPaths = R"(
function setPaths(basePath,paths,cpaths)
   local cleanBasePath = basePath:gsub("\\+$", "")
   local fullPaths = {}
   for _,v in ipairs(paths) do
      table.insert(fullPaths,cleanBasePath.."\\"..((v:gsub("^\\+", "")):gsub("\\+$", "")))
   end
   package.path = table.concat(fullPaths,";")
   local fullCpaths = {}
   for _,v in ipairs(cpaths) do
      table.insert(fullCpaths,cleanBasePath.."\\"..((v:gsub("^\\+", "")):gsub("\\+$", "")))
   end
   package.cpath = table.concat(fullCpaths,";")
end
)";

static const char* LUA_PATHS[] = {
  R"(bin\lua\?.lua)",
  R"(bin\lua\?\init.lua)",
  R"(bin\?.lua)",
  R"(bin\?\init.lua)",
  "share\\lua\\" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\\?.lua",
  "share\\lua\\" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\\?\\init.lua",
  R"(.\?.lua)",
  R"(.\?\init.lua)",
  NULL
};

static const char* LUA_CPATHS[] = {
  R"(bin\?.dll)",
  "lib\\lua\\" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "\\?.dll",
  R"(bin\loadall.dll)",
  R"(.\?.dll)",
  NULL
};

int main(int argc, char** argv) {
  // Determine path, where appName is currently located
  WCHAR installPrefix[PATHCCH_MAX_CCH];
  if (!GetModuleFileNameW(NULL, installPrefix, PATHCCH_MAX_CCH)) {
    fprintf(stderr, "%s: Could not find executable path.\n", appName);
    return 1;
  }

  // Navigate two levels up from <INSTALL_PREFIX>/bin/ldoc.exe to <INSTALL_PREFIX>
  PathCchRemoveFileSpec(installPrefix, PATHCCH_MAX_CCH);
  PathCchRemoveFileSpec(installPrefix, PATHCCH_MAX_CCH);
  std::string utf8Prefix = WideCharToUTF8(installPrefix);

  // Create new Lua state
  lua_State *L = luaL_newstate();
  if (!L) {
    fprintf(stderr, "%s: Failed to create Lua state.\n", appName);
    return 1;
  }

  // Open standard libs
  luaL_openlibs(L);

  // hand-over command-line args to lua by setting global table arg
  lua_newtable(L);
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i);
  }
  lua_setglobal(L, "arg");

  // Globally register function setPaths()
  luaL_dostring(L, setPaths);

  // Putsh function on stack
  lua_getglobal(L, "setPaths");

  // Push 1st arg (the INSTALL_PREFIX)
  lua_pushstring(L, utf8Prefix.c_str());

  // Push 2nd arg (table of paths for package.path)
  lua_newtable(L);
  for (int i = 0; LUA_PATHS[i] != NULL; ++i) {
    lua_pushstring(L, LUA_PATHS[i]);
    lua_rawseti(L, -2, i + 1);
  }

  // Push 3rd arg (table of paths for package.cpath)
  lua_newtable(L);
  for (int i = 0; LUA_CPATHS[i] != NULL; ++i) {
    lua_pushstring(L, LUA_CPATHS[i]);
    lua_rawseti(L, -2, i + 1);
  }

  // Run function
  if (lua_pcall(L, 3, 0, 0) != 0) {
    fprintf(stderr, "%s: Error setting paths: %s\n", appName, lua_tostring(L, -1));
  }
  // Lua state now fully initialized with all standard search paths

  // Path to script called by this launcher
  std::string scriptPath = utf8Prefix + "\\bin\\ldoc.lua";

  // Run script
  if (luaL_dofile(L, scriptPath.c_str()) != 0) {
    fprintf(stderr, "%s: Error: %s\n", appName, lua_tostring(L, -1));
    lua_close(L);
    return 1;
  }

  lua_close(L);
  return 0;
}
