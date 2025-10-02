#include <lua.h>
#include <lauxlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

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
    size_t len_arg;
    const char *buf = luaL_checklstring(L, 2, &len_arg);
    ssize_t bytes_written = write(fd, buf, len_arg);

    if (bytes_written == -1) {
        return luaL_error(L, "write() failed: %s", strerror(errno));
    }
    
    lua_pushinteger(L, (lua_Integer)bytes_written);
    return 1;
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
    if (lua_gettop(L) >= 3) {
        permissions = luaL_checkinteger(L, 3);
    }

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
        return luaL_error(L, "invalid file open mode '%s'", flags_str);
    }

    if (open_flags & (O_CREAT)) { 
        fd = open(pathname, open_flags, permissions);
    } else {
        fd = open(pathname, open_flags); 
    }
    
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
        lua_pushnil(L); 
        free(buf);
        return 1;
    } else {
        free(buf); 
        return luaL_error(L, "read() failed: %s", strerror(errno));
    }
}

static int l_ioctl(lua_State* L) {
    int fd = luaL_checkinteger(L, 1);
    unsigned long request = (unsigned long)luaL_checkinteger(L, 2);
    int ret;

    if (lua_gettop(L) >= 3) {
        int arg = luaL_checkinteger(L, 3); 
        ret = ioctl(fd, request, (void*)(intptr_t)arg);
    } else {
        ret = ioctl(fd, request);
    }

    if (ret == -1) {
        return luaL_error(L, "ioctl() failed: %s", strerror(errno));
    }

    lua_pushinteger(L, ret);
    return 1;
}

static char **make_argv(lua_State *L, int table_idx, const char *pathname, int *count) {
    if (table_idx < 0) {
        table_idx = lua_gettop(L) + table_idx + 1;
    }
    
    int n_args = luaL_len(L, table_idx);
    
    char **argv = (char**)malloc(sizeof(char*) * (n_args + 2)); 
    
    if (argv == NULL) {
        luaL_error(L, "malloc failed for argv array");
        return NULL; 
    }

    argv[0] = (char*)pathname;

    for (int i = 1; i <= n_args; i++) {
        lua_geti(L, table_idx, i);
        argv[i] = (char*)luaL_checkstring(L, -1);
        lua_pop(L, 1);
    }
    argv[n_args + 1] = NULL;
    
    *count = n_args + 1;
    return argv;
}

static int l_execve(lua_State* L) {
    const char *pathname = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    luaL_checktype(L, 3, LUA_TTABLE);

    int argv_count, envp_count;
    
    char **argv = make_argv(L, 2, pathname, &argv_count);

    int n_env = luaL_len(L, 3);
    char **envp = (char**)malloc(sizeof(char*) * (n_env + 1));
    if (envp == NULL) {
        free(argv);
        luaL_error(L, "malloc failed for envp array");
        return 0;
    }
    
    for (int i = 1; i <= n_env; i++) {
        lua_geti(L, 3, i);
        envp[i - 1] = (char*)luaL_checkstring(L, -1);
        lua_pop(L, 1);
    }
    envp[n_env] = NULL;

    execve(pathname, argv, envp);

    int err = errno;
    free(argv);
    free(envp);

    return luaL_error(L, "execve() failed: %s", strerror(err));
}

static int l_waitpid(lua_State* L) {
    pid_t pid = (pid_t)luaL_checkinteger(L, 1);
    
    int options = 0;
    if (lua_gettop(L) >= 2) {
        options = luaL_checkinteger(L, 2);
    }
    
    int status;
    pid_t ret_pid = waitpid(pid, &status, options);

    if (ret_pid == -1) {
        return luaL_error(L, "waitpid() failed: %s", strerror(errno));
    }

    lua_pushinteger(L, ret_pid);
    lua_pushinteger(L, status);
    return 2;
}

static int l_execv(lua_State* L) {
    const char *pathname = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);

    int argv_count;
    char **argv = make_argv(L, 2, pathname, &argv_count);

    execv(pathname, argv);

    int err = errno;
    free(argv); 
    return luaL_error(L, "execv() failed: %s", strerror(err));
}


int luaopen_sys(lua_State *L){
    static const struct luaL_Reg sys[] = {
        {"pipe", l_pipe},
        {"fork", l_fork},
        {"write", l_write},
        {"open", l_open},
        {"getpid", l_getpid},
        {"read", l_read},
        {"ioctl", l_ioctl},
        {"execve", l_execve}, 
        {"waitpid", l_waitpid}, 
        {"execv", l_execv}, 
        {NULL, NULL}
    };

    luaL_newlib(L, sys);
    return 1;
}
