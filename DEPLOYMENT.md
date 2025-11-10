# Deployment Guide - IOT Smart Tile System

## 📦 Deployment Options

### Option 1: Local Development (Docker)

**Prerequisites:**
- Docker Desktop installed
- 8GB+ RAM available
- 10GB+ disk space

**Steps:**

1. Clone and configure:
```batch
git clone https://github.com/DICEsda/IOT-TileNodeCoordinator.git
cd IOT-TileNodeCoordinator
copy .env.example .env
```

2. Build and start:
```batch
docker-build.bat
docker-run.bat
```

3. Verify deployment:
- Frontend: http://localhost:4200
- Backend: http://localhost:8000/health
- MQTT: mqtt://localhost:1883

### Option 2: Production Deployment (Docker Swarm)

**Prerequisites:**
- Docker Swarm initialized
- Load balancer configured
- SSL certificates

**Steps:**

1. Create production environment:
```bash
cp .env.example .env.production
# Edit .env.production with production values
```

2. Build production images:
```bash
docker-compose -f docker-compose.yml -f docker-compose.prod.yml build
```

3. Deploy stack:
```bash
docker stack deploy -c docker-compose.yml iot-smarttile
```

4. Scale services:
```bash
docker service scale iot-smarttile_backend=3
```

### Option 3: Kubernetes Deployment

**Prerequisites:**
- Kubernetes cluster (1.24+)
- kubectl configured
- Helm 3.x

**Steps:**

1. Create namespace:
```bash
kubectl create namespace iot-smarttile
```

2. Deploy MongoDB:
```bash
helm repo add bitnami https://charts.bitnami.com/bitnami
helm install mongodb bitnami/mongodb -n iot-smarttile \
  --set auth.rootPassword=admin123 \
  --set auth.database=iot_smarttile
```

3. Deploy Mosquitto:
```bash
kubectl apply -f k8s/mosquitto-deployment.yml
```

4. Deploy Backend:
```bash
kubectl apply -f k8s/backend-deployment.yml
```

5. Deploy Frontend:
```bash
kubectl apply -f k8s/frontend-deployment.yml
```

## 🔧 Configuration

### Environment Variables

Create `.env` file with the following:

```bash
# Production MongoDB
MONGO_ROOT_USER=admin
MONGO_ROOT_PASSWORD=<strong-password>
MONGO_DB=iot_smarttile
MONGO_URI=mongodb://admin:<password>@mongodb:27017

# Production MQTT
MQTT_BROKER=tcp://mosquitto:1883
MQTT_USERNAME=<mqtt-user>
MQTT_PASSWORD=<mqtt-password>

# Backend
HTTP_ADDR=:8000

# Frontend
API_URL=https://api.yourdomain.com
WS_URL=wss://api.yourdomain.com/ws
```

### SSL/TLS Configuration

For production, enable SSL:

1. **Backend TLS:**
```yaml
# docker-compose.prod.yml
backend:
  environment:
    TLS_CERT: /certs/cert.pem
    TLS_KEY: /certs/key.pem
  volumes:
    - ./certs:/certs:ro
```

2. **MQTT TLS:**
```conf
# mosquitto.conf
listener 8883
cafile /mosquitto/certs/ca.crt
certfile /mosquitto/certs/server.crt
keyfile /mosquitto/certs/server.key
```

3. **Frontend (nginx):**
```conf
server {
    listen 443 ssl http2;
    ssl_certificate /etc/nginx/certs/cert.pem;
    ssl_certificate_key /etc/nginx/certs/key.pem;
}
```

## 🔐 Security Hardening

### 1. Change Default Passwords
```bash
# MongoDB
MONGO_ROOT_PASSWORD=$(openssl rand -base64 32)

# MQTT
mosquitto_passwd -c /mosquitto/config/pwfile <username>
```

### 2. Enable Firewall
```bash
# Allow only necessary ports
ufw allow 443/tcp  # HTTPS
ufw allow 8883/tcp # MQTT TLS
ufw deny 27017/tcp # Block external MongoDB
```

### 3. Use Secrets Management
```bash
# Docker secrets
echo "<password>" | docker secret create mongo_password -
```

## 📊 Monitoring

### Health Checks

All services have health endpoints:
```bash
# Backend
curl http://localhost:8000/health

# Frontend
curl http://localhost:4200/health

# MongoDB
docker exec mongodb mongosh --eval "db.adminCommand('ping')"

# MQTT
mosquitto_sub -h localhost -t '$SYS/#' -C 1
```

### Logging

View logs:
```bash
# All services
docker-compose logs -f

# Specific service
docker-compose logs -f backend

# Last 100 lines
docker-compose logs --tail=100 backend
```

### Metrics Collection

Consider adding Prometheus + Grafana:
```yaml
# docker-compose.monitoring.yml
prometheus:
  image: prom/prometheus
  volumes:
    - ./prometheus.yml:/etc/prometheus/prometheus.yml

grafana:
  image: grafana/grafana
  ports:
    - "3000:3000"
```

## 🔄 Updates and Rollback

### Rolling Update
```bash
# Pull latest images
docker-compose pull

# Restart with new images
docker-compose up -d
```

### Rollback
```bash
# Tag current version
docker tag iot-backend:latest iot-backend:rollback

# Revert to previous
docker-compose down
docker-compose up -d
```

## 🧪 Testing Deployment

### 1. Service Connectivity
```bash
# Test backend API
curl http://localhost:8000/sites

# Test MQTT
mosquitto_pub -h localhost -t "test/topic" -m "hello"
mosquitto_sub -h localhost -t "test/topic"
```

### 2. Database Connection
```bash
docker exec mongodb mongosh -u admin -p admin123 --eval "db.stats()"
```

### 3. End-to-End Flow
1. Flash ESP32 coordinator and nodes
2. Verify telemetry in backend logs
3. Check MongoDB for data
4. View real-time data in frontend

## 🚨 Troubleshooting

### Backend won't start
```bash
# Check logs
docker-compose logs backend

# Common fixes
docker-compose down -v
docker-compose build --no-cache backend
docker-compose up backend
```

### MongoDB connection refused
```bash
# Check MongoDB is running
docker-compose ps mongodb

# Test connection
docker exec -it mongodb mongosh -u admin -p admin123

# Reset MongoDB
docker-compose down mongodb
docker volume rm iot-tile_mongodb_data
docker-compose up -d mongodb
```

### MQTT broker issues
```bash
# Check Mosquitto status
docker-compose logs mosquitto

# Test connection
mosquitto_sub -h localhost -p 1883 -u user1 -P user1 -t '#'

# Restart Mosquitto
docker-compose restart mosquitto
```

### Frontend not loading
```bash
# Check nginx config
docker exec iot-frontend nginx -t

# Check backend connectivity
docker exec iot-frontend curl http://backend:8000/health

# Rebuild frontend
docker-compose build --no-cache frontend
docker-compose up -d frontend
```

## 📱 ESP32 Deployment

### Coordinator Setup

1. Flash firmware:
```bash
cd coordinator
pio run -t upload
```

2. Configure WiFi:
```cpp
// In Coordinator.cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

3. Set MQTT broker:
```cpp
const char* mqtt_broker = "192.168.1.100"; // Your server IP
```

### Node Setup

1. Flash firmware:
```bash
cd node
pio run -t upload
```

2. Pair with coordinator:
- Hold button for 2 seconds
- Press coordinator touch button
- Wait for green LED confirmation

## 🌐 Domain Setup

### DNS Configuration
```
A     api.yourdomain.com     → <server-ip>
A     app.yourdomain.com     → <server-ip>
CNAME mqtt.yourdomain.com    → api.yourdomain.com
```

### Reverse Proxy (nginx)
```conf
server {
    server_name api.yourdomain.com;
    
    location / {
        proxy_pass http://localhost:8000;
    }
}

server {
    server_name app.yourdomain.com;
    
    location / {
        proxy_pass http://localhost:4200;
    }
}
```

## 📈 Scaling

### Horizontal Scaling
```yaml
# docker-compose.scale.yml
backend:
  deploy:
    replicas: 3
    
frontend:
  deploy:
    replicas: 2
```

### Load Balancing
```bash
# Add nginx load balancer
docker-compose -f docker-compose.yml -f docker-compose.lb.yml up -d
```

## 💾 Backup and Recovery

### Database Backup
```bash
# Backup MongoDB
docker exec mongodb mongodump --out=/backup

# Restore MongoDB
docker exec mongodb mongorestore /backup
```

### Configuration Backup
```bash
# Backup all configs
tar -czf backup-$(date +%Y%m%d).tar.gz .env docker-compose.yml
```

## 📞 Support

For issues:
1. Check logs: `docker-compose logs`
2. Review documentation: `docs/`
3. Create GitHub issue
4. Contact: support@yourdomain.com

---

**Version:** 1.0.0  
**Last Updated:** 2025-11-05
