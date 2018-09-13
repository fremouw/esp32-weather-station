picocom --noreset -b 115200 /dev/ttyUSB0|awk '{ print strftime("%Y-%m-%d %T: "), $0; fflush(); }' | tee /tmp/esp32-serial.txt
