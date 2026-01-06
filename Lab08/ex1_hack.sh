#!/bin/bash

pid=$(cat /tmp/ex1.pid)
echo Pid is: $pid

map_line=$(sudo cat /proc/$pid/maps | grep heap)

start=${map_line%%-*}
end=${map_line#*-}
end=${end%% *}

text=$(sudo xxd -s 0x$start -l $((0x$end-0x$start)) /proc/$pid/mem | grep -m 1 pass:)
address="${text%%:*}"
echo Address is: 0x$address
pass="${text#*pass:}"
pass="${pass%???}"
echo Password is: pass:$pass
kill -s KILL $pid
