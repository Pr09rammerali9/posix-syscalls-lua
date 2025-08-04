#include <lua.h>
#include <lauxlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

static int l_fork(lua_State* L) {
        pid_t pid = fork();

        if (pid == -1) {
                return luaL_error(L, "fork() failed: %s", strerror(errno));
        } else {
                lua_pushinteger(L, (lua_Integer)pid);
                return 1;
        }
}

static int l_write(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  const char *buf = luaL_checkstring(L, 2);
  size_t len = luaL_checkinteger(L, 3);
  ssize_t bytes_written = write(fd, buf, len);

  if (bytes_written == (ssize_t)len) {
    lua_pushinteger(L, (lua_Integer)bytes_written);
    return 1;
  } else if (bytes_written == -1) {
    return luaL_error(L, "write() failed: %s", strerror(errno));
  } else {
    return luaL_error(L, "Partial write: %zd bytes written out of %zu", bytes_written, len);
  }
}

static int l_getpid(lua_State *L) {
        lua_pushinteger(L, (lua_Integer)getpid());
        return 1;
}

static int l_open(lua_State *L) {
    const char *pathname = luaL_checkstring(L, 1);
    const char *flags_str = luaL_checkstring(L, 2);
    int fd = -1;
    int open_flags = 0;
    int permissions = 0644;

    if (strcmp(flags_str, "r") == 0 || strcmp(flags_str, "RD") == 0) {
        open_flags = O_RDONLY;
    } else if (strcmp(flags_str, "w") == 0 || strcmp(flags_str, "WR") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(flags_str, "a") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (strcmp(flags_str, "r+") == 0 || strcmp(flags_str, "WR_RD") == 0) {
        open_flags = O_RDWR;
    } else if (strcmp(flags_str, "w+") == 0) {
        open_flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strcmp(flags_str, "a+") == 0) {
        open_flags = O_RDWR | O_CREAT | O_APPEND;
    } else if (strcmp(flags_str, "CRT") == 0) {
        open_flags = O_CREAT | O_EXCL | O_WRONLY;
    } else if (strcmp(flags_str, "WRC") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else {
        luaL_error(L, "invalid file open mode '%s'", flags_str);
        return 0;
    }

    fd = open(pathname, open_flags, permissions);

    if (fd == -1) {
        return luaL_error(L, "open() failed: %s", strerror(errno));
    } else {
        lua_pushinteger(L, fd);
        return 1;
    }
}

static int l_pipe(lua_State *L) {
        int pipefd[2];

        if (pipe(pipefd) == -1) {
                return luaL_error(L, "pipe() failed: %s", strerror(errno));
        } else {
        lua_pushinteger(L, pipefd[0]);
        lua_pushinteger(L, pipefd[1]);
        return 2;
        }
}

static int l_read(lua_State *L) {
    int fd = luaL_checkinteger(L, 1);
    size_t count = luaL_checkinteger(L, 2);
    
    char* buf = (char *)malloc(count);
    
    if (buf == NULL) {
        return luaL_error(L, "Failed to allocate buffer for reading: %s", strerror(errno));
    }
    
    ssize_t bytes_read = read(fd, buf, count);
    
    if (bytes_read > 0) {
        lua_pushlstring(L, buf, bytes_read);
        free(buf);
        return 1;
    } else if (bytes_read == 0) {
        lua_pushstring(L, "");
        free(buf);
        return 1;
    } else {
        // Corrected path: free the buffer before returning the error
        free(buf); 
        return luaL_error(L, "read() failed: %s", strerror(errno));
    }
}

int luaopen_sys(lua_State *L){
static const struct luaL_Reg sys[] = {
        {"pipe", l_pipe},
        {"fork", l_fork},
        {"write", l_write},
        {"open", l_open},
        {"getpid", l_getpid},
        {"read", l_read},
        {NULL, NULL}
};

luaL_newlib(L, sys);

return 1;
}#include <lua.h>
#include <lauxlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>

static int l_fork(lua_State* L) {
        pid_t pid = fork();

        if (pid == -1) {
                return luaL_error(L, "fork() failed: %s", strerror(errno));
        } else {
                lua_pushinteger(L, (lua_Integer)pid);
                return 1;
        }
}

static int l_write(lua_State *L) {
  int fd = luaL_checkinteger(L, 1);
  const char *buf = luaL_checkstring(L, 2);
  size_t len = luaL_checkinteger(L, 3);
  ssize_t bytes_written = write(fd, buf, len);

  if (bytes_written == (ssize_t)len) {
    lua_pushinteger(L, (lua_Integer)bytes_written);
    return 1;
  } else if (bytes_written == -1) {
    return luaL_error(L, "write() failed: %s", strerror(errno));
  } else {
    return luaL_error(L, "Partial write: %zd bytes written out of %zu", bytes_written, len);
  }
}

static int l_getpid(lua_State *L) {
        lua_pushinteger(L, (lua_Integer)getpid());
        return 1;
}

static int l_open(lua_State *L) {
    const char *pathname = luaL_checkstring(L, 1);
    const char *flags_str = luaL_checkstring(L, 2);
    int fd = -1;
    int open_flags = 0;
    int permissions = 0644;

    if (strcmp(flags_str, "r") == 0 || strcmp(flags_str, "RD") == 0) {
        open_flags = O_RDONLY;
    } else if (strcmp(flags_str, "w") == 0 || strcmp(flags_str, "WR") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else if (strcmp(flags_str, "a") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_APPEND;
    } else if (strcmp(flags_str, "r+") == 0 || strcmp(flags_str, "WR_RD") == 0) {
        open_flags = O_RDWR;
    } else if (strcmp(flags_str, "w+") == 0) {
        open_flags = O_RDWR | O_CREAT | O_TRUNC;
    } else if (strcmp(flags_str, "a+") == 0) {
        open_flags = O_RDWR | O_CREAT | O_APPEND;
    } else if (strcmp(flags_str, "CRT") == 0) {
        open_flags = O_CREAT | O_EXCL | O_WRONLY;
    } else if (strcmp(flags_str, "WRC") == 0) {
        open_flags = O_WRONLY | O_CREAT | O_TRUNC;
    } else {
        luaL_error(L, "invalid file open mode '%s'", flags_str);
        return 0;
    }

    fd = open(pathname, open_flags, permissions);

    if (fd == -1) {
        return luaL_error(L, "open() failed: %s", strerror(errno));
    } else {
        lua_pushinteger(L, fd);
        return 1;
    }
}

static int l_pipe(lua_State *L) {
        int pipefd[2];

        if (pipe(pipefd) == -1) {
                return luaL_error(L, "pipe() failed: %s", strerror(errno));
        } else {
        lua_pushinteger(L, pipefd[0]);
        lua_pushinteger(L, pipefd[1]);
        return 2;
        }
}

static int l_read(lua_State *L) {
        int fd = luaL_checkinteger(L, 1);

        size_t count = luaL_checkinteger(L, 2);

        char* buf = (char *)malloc(count);

        if (buf == NULL) {
                return luaL_error(L, "Failed to allocate buffer for reading: %s", strerror(errno));
        }

        ssize_t bytes_read = read(fd, buf, count);

        if (bytes_read > 0) {
            lua_pushlstring(L, buf, bytes_read);
            free(buf);
            return 1;
        } else if (bytes_read == 0) {
            lua_pushstring(L, "");
            free(buf);
            return 1;
        } else {
            free(buf);
            return luaL_error(L, "read() failed: %s", strerror(errno));
        }
}

int luaopen_sys(lua_State *L){
static const struct luaL_Reg sys[] = {
        {"pipe", l_pipe},
        {"fork", l_fork},
        {"write", l_write},
        {"open", l_open},
        {"getpid", l_getpid},
        {"read", l_read},
        {NULL, NULL}
};

luaL_newlib(L, sys);

return 1;
}
