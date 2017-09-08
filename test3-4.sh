

cat /dev/rts/RS2 > fromRS6 &
catPid="$!"
cat ./log > /dev/rts/RS6

kill -13 "$catPid" &>/dev/null