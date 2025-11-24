@echo off
echo ========================================
echo Coordinator Diagnostics
echo ========================================
echo.

echo [1/5] Checking if coordinator is publishing MQTT...
echo Waiting 5 seconds for messages...
docker exec iot-mosquitto sh -c "timeout 5 mosquitto_sub -h localhost -t 'site/#' -C 1 -v 2>&1 || echo 'NO MESSAGES RECEIVED'"
echo.

echo [2/5] Checking backend logs for telemetry...
docker logs iot-backend --tail 30 2>&1 | findstr /i "telemetry coord"
echo.

echo [3/5] Checking database for coordinators...
docker exec iot-mongodb mongosh iot_smarttile --username admin --password admin123 --authenticationDatabase admin --quiet --eval "db.coordinators.countDocuments()" 2>&1
echo.

echo [4/5] Checking MQTT broker status...
docker exec iot-mosquitto sh -c "mosquitto_sub -h localhost -t '$SYS/broker/clients/active' -C 1 -W 2 2>&1 || echo 'BROKER NOT RESPONDING'"
echo.

echo [5/5] Checking if coordinator is listed in MQTT clients...
docker exec iot-mosquitto sh -c "mosquitto_sub -h localhost -t '$SYS/broker/clients/#' -C 10 -W 3 2>&1" | findstr /i "coord esp"
echo.

echo ========================================
echo Diagnosis Complete
echo ========================================
echo.
echo What the results mean:
echo.
echo [1] NO MESSAGES = Coordinator NOT publishing (firmware not flashed?)
echo [2] No "telemetry saved" = Backend not receiving messages
echo [3] Count = 0 = Database empty
echo [4] BROKER NOT RESPONDING = MQTT broker issue
echo [5] No clients = Coordinator not connected to MQTT
echo.
echo ========================================
echo SOLUTION:
echo ========================================
echo.
echo If coordinator is NOT publishing:
echo   1. Is the coordinator powered on?
echo   2. Did you flash the updated firmware?
echo      cd coordinator
echo      pio run -e esp32-s3-devkitc-1 -t upload -t monitor
echo   3. Check serial output for MQTT connection status
echo.
pause
