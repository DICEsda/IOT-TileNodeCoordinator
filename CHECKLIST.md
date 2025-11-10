# IOT Smart Tile System - Checklist

## 🎯 Setup & Deployment

### Prerequisites
- [ ] Docker Desktop installed and running
- [ ] Git installed
- [ ] 8GB+ RAM available
- [ ] 10GB+ free disk space
- [ ] ESP-IDF 5.x (for firmware development)
- [ ] PlatformIO (for firmware flashing)

### Initial Setup
- [ ] Repository cloned
- [ ] `.env` file created from `.env.example`
- [ ] Environment variables configured
- [ ] Docker images built (`docker-build.bat`)
- [ ] Services started (`docker-run.bat`)
- [ ] Health checks passed

### Service Verification
- [ ] Frontend accessible: http://localhost:4200
- [ ] Backend healthy: http://localhost:8000/health
- [ ] MQTT broker responding: `mosquitto_sub -h localhost -t '#'`
- [ ] MongoDB accepting connections

## 🔧 Backend Configuration

### Environment
- [ ] HTTP_ADDR configured
- [ ] MQTT_BROKER URL set
- [ ] MQTT credentials configured
- [ ] MONGO_URI set correctly
- [ ] MONGO_DB name configured

### API Endpoints
- [ ] Health check working (`/health`)
- [ ] Sites API responding (`/sites`)
- [ ] Coordinators API responding (`/coordinators/{id}`)
- [ ] Nodes API responding (`/nodes/{id}`)
- [ ] Commands API working (`/set-light`, `/color-profile`)
- [ ] OTA endpoints functional (`/ota/*`)
- [ ] WebSocket connection established (`/ws`)

### MQTT Integration
- [ ] MQTT client connects successfully
- [ ] Subscribes to telemetry topics
- [ ] Node telemetry handler working
- [ ] Coordinator telemetry handler working
- [ ] mmWave events being received
- [ ] Data persisting to MongoDB

### Database
- [ ] MongoDB connection established
- [ ] Collections created (nodes, coordinators, sites, ota_jobs)
- [ ] Upsert operations working
- [ ] Queries returning data
- [ ] Indexes created (if needed)

## 🎨 Frontend Configuration

### Build & Deploy
- [ ] Docker image builds successfully
- [ ] Nginx configuration valid
- [ ] Static assets served correctly
- [ ] API proxy working (`/api/*`)
- [ ] WebSocket proxy working (`/ws`)

### Services (TO BE IMPLEMENTED)
- [ ] API service created
- [ ] WebSocket service created
- [ ] MQTT service created
- [ ] Auth service integrated
- [ ] Real-time updates working

### Components
- [ ] Dashboard component functional
- [ ] Device list displaying
- [ ] Node controls working
- [ ] Room visualizer rendering
- [ ] Logs displaying telemetry
- [ ] Settings page functional

## 📡 ESP32 Firmware

### Coordinator (ESP32-S3)
- [ ] Firmware compiles without errors
- [ ] WiFi credentials configured
- [ ] MQTT broker address set
- [ ] ESP-NOW initialized
- [ ] mmWave sensor integrated
- [ ] Touch button working
- [ ] LED indicators functional
- [ ] Telemetry publishing to correct topics
- [ ] Commands received and executed
- [ ] Pairing flow working

### Node (ESP32-C3)
- [ ] Firmware compiles without errors
- [ ] ESP-NOW initialized
- [ ] SK6812B LEDs controlled
- [ ] Button input working
- [ ] Temperature sensor reading (if present)
- [ ] Battery monitoring working
- [ ] Pairing mode activates
- [ ] Commands received and executed
- [ ] Telemetry sending
- [ ] Power management active

## 🔐 Security

### MQTT
- [ ] Authentication enabled
- [ ] Password file configured
- [ ] Anonymous access disabled
- [ ] TLS/SSL configured (production)

### MongoDB
- [ ] Root password changed from default
- [ ] Database user created
- [ ] Authentication enabled
- [ ] Network access restricted

### Backend
- [ ] CORS configured correctly
- [ ] Rate limiting implemented (optional)
- [ ] Input validation added
- [ ] Error handling in place

### ESP32
- [ ] ESP-NOW encryption enabled
- [ ] PMK/LMK configured
- [ ] Secure boot enabled (optional)
- [ ] Flash encryption enabled (optional)

## 📊 Monitoring & Logging

### Health Checks
- [ ] All services have health endpoints
- [ ] Docker healthchecks configured
- [ ] Automated monitoring set up (optional)

### Logging
- [ ] Backend logs structured (JSON)
- [ ] MQTT logs accessible
- [ ] Frontend error logging
- [ ] ESP32 serial logs captured

### Metrics
- [ ] Telemetry data flowing
- [ ] Database metrics tracked
- [ ] MQTT message rates monitored
- [ ] API response times measured

## 🧪 Testing

### Unit Tests
- [ ] Backend tests written
- [ ] Backend tests passing
- [ ] Frontend tests written
- [ ] Frontend tests passing

### Integration Tests
- [ ] Docker services start correctly
- [ ] Health checks pass
- [ ] API endpoints respond
- [ ] MQTT messages flow correctly

### End-to-End Tests
- [ ] Coordinator connects to backend
- [ ] Node pairs with coordinator
- [ ] Telemetry reaches backend
- [ ] Data persists in database
- [ ] Frontend displays real-time data
- [ ] Commands reach nodes
- [ ] LEDs respond to commands
- [ ] Presence detection triggers lights

## 🚀 Deployment

### Local Development
- [ ] Quick start script works
- [ ] All services start correctly
- [ ] Hot reload working (dev mode)
- [ ] Logs accessible

### Production
- [ ] Production `.env` configured
- [ ] SSL/TLS certificates installed
- [ ] Domain names configured
- [ ] Reverse proxy set up
- [ ] Firewall rules configured
- [ ] Backup strategy in place
- [ ] Monitoring configured

## 📚 Documentation

### User Documentation
- [ ] README complete
- [ ] Quick start guide written
- [ ] API documentation available
- [ ] Troubleshooting guide included

### Developer Documentation
- [ ] Architecture documented
- [ ] Code comments adequate
- [ ] Setup instructions clear
- [ ] Deployment guide complete

### Operational Documentation
- [ ] Monitoring setup documented
- [ ] Backup procedures documented
- [ ] Disaster recovery plan created
- [ ] Scaling guide written

## 🔄 CI/CD

### GitHub Actions
- [ ] Workflow file created
- [ ] Backend tests run on push
- [ ] Frontend tests run on push
- [ ] Docker images build automatically
- [ ] Firmware compiles in CI
- [ ] Integration tests run
- [ ] Deployment automated (optional)

## 📋 Known Issues & TODOs

### High Priority
- [ ] Frontend services need API integration
- [ ] MQTT topics need alignment with PRD
- [ ] End-to-end testing with hardware

### Medium Priority
- [ ] WebSocket real-time updates in frontend
- [ ] Authentication/authorization system
- [ ] Admin dashboard for configuration

### Low Priority
- [ ] Google Home API integration
- [ ] Mobile app development
- [ ] Advanced analytics
- [ ] Multi-site management

## ✅ Sign-off

### Development Team
- [ ] Backend developer approved
- [ ] Frontend developer approved
- [ ] Embedded developer approved
- [ ] DevOps engineer approved

### Testing Team
- [ ] Unit tests passed
- [ ] Integration tests passed
- [ ] System tests passed
- [ ] User acceptance testing passed

### Stakeholders
- [ ] Product owner approved
- [ ] Technical lead approved
- [ ] Operations approved
- [ ] Security review passed

---

**Project Status:** 85% Complete  
**Ready for:** Development Testing  
**Blocked by:** Hardware integration testing

**Last Updated:** 2025-11-05  
**Next Review:** After hardware testing
