# mariadb link issue

## issue description

When I linked for `libmysqlclient.so`, using command `mysql_config` in terminal, it echoed as follows:
```
--libs        [-L/usr/lib/x86_64-linux-gnu/ -lmariadb]
--libs_sys      [-ldl -lm -lpthread -lssl -lcrypto]
```
So I added these flags to `CMAKE_CXX_FLAGS` in `CMAKELIST.txt`.
```
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lmariadb -ldl -lm -lssl -lcrypto")
```
But it didn't work, with displaying `undifined reference to 'mysql_*'`.

So I tried to compile solidtary `sqlConn.cpp`, using flags as above, displaying `cannot find mariadb`.
I guessed it must due to the `global variable` for `g++`. But I am not feel like revising it.

So it should be `-lmysqlclient` instead:
```
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lmysqlclient -ldl -lm -lssl -lcrypto")
```
It worked finally.

Buy the way, don't forget to specify directory for `libmysqlclient.so`
`target_link_libraries(<target> /usr/lib/x86_64-linux-gnu/libmysqlclient.so)`
