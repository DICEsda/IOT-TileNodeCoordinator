package config

import (
	"log"
	"os"

	yaml "gopkg.in/yaml.v3"
)

type Config struct {
	HTTP struct {
		Addr string `yaml:"addr"`
	} `yaml:"server"`
	MQTT struct {
		Broker   string `yaml:"broker"`
		Username string `yaml:"username"`
		Password string `yaml:"password"`
	} `yaml:"mqtt"`
	DB struct {
		URI      string `yaml:"uri"`
		Database string `yaml:"database"`
	} `yaml:"db"`
}

func NewConfig() (*Config, error) {
	cfg := &Config{}
	
	// Try to load from config.yaml file first
	configPath := os.Getenv("CONFIG_PATH")
	if configPath == "" {
		configPath = "./internal/config/config.yaml"
	}
	
	f, err := os.Open(configPath)
	if err == nil {
		defer func() {
			if err := f.Close(); err != nil {
				log.Printf("failed to close file %v", err)
			}
		}()
		
		decoder := yaml.NewDecoder(f)
		err = decoder.Decode(cfg)
		if err != nil {
			log.Printf("failed to decode config file: %v", err)
		}
	} else {
		log.Printf("config file not found, using environment variables and defaults")
	}
	
	// Override with environment variables if set
	if addr := os.Getenv("HTTP_ADDR"); addr != "" {
		cfg.HTTP.Addr = addr
	} else if cfg.HTTP.Addr == "" {
		cfg.HTTP.Addr = ":8000"
	}
	
	if broker := os.Getenv("MQTT_BROKER"); broker != "" {
		cfg.MQTT.Broker = broker
	} else if cfg.MQTT.Broker == "" {
		cfg.MQTT.Broker = "tcp://localhost:1883"
	}
	
	if username := os.Getenv("MQTT_USERNAME"); username != "" {
		cfg.MQTT.Username = username
	} else if cfg.MQTT.Username == "" {
		cfg.MQTT.Username = "user1"
	}
	
	if password := os.Getenv("MQTT_PASSWORD"); password != "" {
		cfg.MQTT.Password = password
	} else if cfg.MQTT.Password == "" {
		cfg.MQTT.Password = "user1"
	}
	
	if uri := os.Getenv("MONGO_URI"); uri != "" {
		cfg.DB.URI = uri
	} else if cfg.DB.URI == "" {
		cfg.DB.URI = "mongodb://localhost:27017"
	}
	
	if db := os.Getenv("MONGO_DB"); db != "" {
		cfg.DB.Database = db
	} else if cfg.DB.Database == "" {
		cfg.DB.Database = "iot_smarttile"
	}
	
	log.Printf("Config loaded: HTTP=%s, MQTT=%s, DB=%s", 
		cfg.HTTP.Addr, cfg.MQTT.Broker, cfg.DB.URI)
	
	return cfg, nil
}
