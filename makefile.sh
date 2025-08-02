Cc=clang
flgs=-fPIC -shared
cf=main.c
libobj=liblua-posix-syscalls.so
libpath=/usr/lib
if [[ $1 == "build" ]]; then

$Cc $flgs $cf -o $libobj

elif [[ $1 == "clean" ]]; then 

rm $libobj

elif [[ $1 == "mv" ]]; then 

mv $libobj $libpath/$libobj
fi
