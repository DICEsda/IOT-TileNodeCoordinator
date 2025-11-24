// Quick script to manually insert coordinator into database
// Run with: docker exec -i iot-mongodb mongosh iot_smarttile --username admin --password admin123 --authenticationDatabase admin < insert-coordinator.js

db.coordinators.insertOne({
  _id: "74:4D:BD:AB:A9:F4",
  site_id: "site001",
  coordinator_id: "74:4D:BD:AB:A9:F4",
  fw_version: "1.0.0",
  nodes_online: 0,
  wifi_rssi: -42,
  mmwave_event_rate: 0,
  light_lux: 88,
  temp_c: 40.5,
  last_seen: new Date()
});

print("Coordinator inserted successfully!");
db.coordinators.find().pretty();
