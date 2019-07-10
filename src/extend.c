#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include "extend.h"

void test() {
  lua_State *L;

  L = luaL_newstate();
  luaL_openlibs(L);

  int status = luaL_loadfile(L, "hello.lua");
  int ret = lua_pcall(L, 0, 0, 0);
  if (ret != 0) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    return;
  }

  lua_close(L);
}