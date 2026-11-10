#!/bin/bash
# Usage: ./run.sh path/to/voiceover_device_ble.ino
set -u
INO=$(readlink -f "$1"); W=$(mktemp -d); cd "$(dirname "$0")"
sed -E 's/#define HOLD_MS +0 /#define HOLD_MS 150 /' "$INO" > $W/hold.ino
sed -E 's/#define IGNORE_REPEAT_MS +0 /#define IGNORE_REPEAT_MS 400 /' "$INO" > $W/repeat.ino
g++ -std=c++17 -O1 -Wall -Wno-unused -Imock -DFW="\"$INO\"" test_fw.cpp -o $W/sim || exit 1
g++ -std=c++17 -O1 -Imock -DVARIANT_HOLD -DFW="\"$W/hold.ino\"" test_fw.cpp -o $W/sim_hold || exit 1
g++ -std=c++17 -O1 -Imock -DVARIANT_REPEAT -DFW="\"$W/repeat.ino\"" test_fw.cpp -o $W/sim_rep || exit 1
pass=0; fail=0
run() { local bin=$1 sc=$2 ee=$3; out=$($W/$bin "$sc" "$ee"); echo "$out"; [[ $out == *"PASS $sc"* ]] && pass=$((pass+1)) || fail=$((fail+1)); }
for sc in $($W/sim); do
  [[ $sc == mode_persists_reboot ]] && continue
  rm -f $W/ee.bin; run sim "$sc" $W/ee.bin
  [[ $sc == combo_hold_switches_mode ]] && run sim mode_persists_reboot $W/ee.bin
done
for sc in $($W/sim_hold | grep -v -x -f <($W/sim)); do rm -f $W/ee.bin; run sim_hold "$sc" $W/ee.bin; done
for sc in $($W/sim_rep  | grep -v -x -f <($W/sim)); do rm -f $W/ee.bin; run sim_rep  "$sc" $W/ee.bin; done
echo "== $pass passed, $fail failed"
rm -rf $W; [ $fail -eq 0 ]
