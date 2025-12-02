# Multi-stage build for Go backend
FROM golang:1.23-alpine AS builder

# Install build dependencies
RUN apk add --no-cache git gcc musl-dev

# Set working directory
WORKDIR /app

# Copy only go.mod first for caching deps
COPY go.mod ./

# Download module deps (will create go.sum in the image layer)
RUN go mod download && go mod verify

# Copy source code (includes host go.sum which may be empty)
COPY . .

# Tidy dependencies to update go.sum
RUN go mod tidy

# Build the application
RUN CGO_ENABLED=0 GOOS=linux go build -a -installsuffix cgo -o main ./cmd/iot

# Final stage - minimal runtime image
FROM alpine:latest

RUN apk --no-cache add ca-certificates wget

WORKDIR /root/

# Copy the binary from builder
COPY --from=builder /app/main .

# Copy configuration files
COPY --from=builder /app/internal/config/config.yaml ./internal/config/

# Expose port
EXPOSE 8000

# Health check
HEALTHCHECK --interval=10s --timeout=5s --start-period=20s --retries=3 \
  CMD wget --quiet --tries=1 --spider http://localhost:8000/health || exit 1

# Run the binary
CMD ["./main"]
