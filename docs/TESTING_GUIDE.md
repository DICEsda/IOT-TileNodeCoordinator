# IOT-TileNodeCoordinator Testing Guide

This document provides comprehensive information about the test suite for the IOT-TileNodeCoordinator project.

## Table of Contents
- [Overview](#overview)
- [Test Types](#test-types)
- [Running Tests](#running-tests)
- [Test Coverage](#test-coverage)
- [CI/CD Integration](#cicd-integration)
- [Writing Tests](#writing-tests)

## Overview

The project includes comprehensive testing across all components:
- **Backend (Go)**: Unit tests, integration tests, benchmarks
- **Frontend (Angular)**: Unit tests, E2E tests
- **Firmware (C++)**: Unit tests using PlatformIO
- **System**: End-to-end integration tests

## Test Types

### 1. Backend Tests (Go)

#### Unit Tests
Located in: `IOT-Backend-main/IOT-Backend-main/internal/**/*_test.go`

Tests include:
- **Repository Tests**: Database operations, CRUD operations
- **MQTT Handler Tests**: Message publishing, subscription, command handling
- **HTTP Handler Tests**: REST API endpoints, request validation
- **Type Tests**: JSON serialization, data validation

**Run unit tests:**
```bash
cd IOT-Backend-main/IOT-Backend-main
go test -v ./...
```

**With coverage:**
```bash
go test -v -race -coverprofile=coverage.txt -covermode=atomic ./...
go tool cover -html=coverage.txt
```

#### Integration Tests
Located in: `IOT-Backend-main/IOT-Backend-main/internal/integration_test.go`

Tests full system integration with:
- MongoDB
- MQTT broker
- WebSocket connections

**Run integration tests:**
```bash
# Requires MongoDB and MQTT broker running
docker-compose up -d mongodb mosquitto
go test -v -tags=integration ./...
```

#### Benchmarks
```bash
go test -bench=. -benchmem ./...
```

### 2. Frontend Tests (Angular)

#### Unit Tests
Located in: `IOT-Frontend-main/IOT-Frontend-main/src/**/*.spec.ts`

Uses Jasmine and Karma for:
- Component tests
- Service tests
- Pipe and guard tests

**Run unit tests:**
```bash
cd IOT-Frontend-main/IOT-Frontend-main
npm test
```

**With coverage:**
```bash
npm run test -- --code-coverage
```

#### E2E Tests
Located in: `IOT-Frontend-main/IOT-Frontend-main/e2e/**/*.e2e-spec.ts`

Uses Playwright for:
- User flow testing
- UI interaction testing
- Real-time update testing
- Cross-browser compatibility

**Run E2E tests:**
```bash
npm run e2e
```

**Specific browser:**
```bash
npx playwright test --project=chromium
npx playwright test --project=firefox
npx playwright test --project=webkit
```

**Debug mode:**
```bash
npx playwright test --debug
```

### 3. Firmware Tests (C++)

#### Coordinator Tests
Located in: `coordinator/test/test_coordinator.cpp`

Tests include:
- Node registry operations
- MAC address handling
- Telemetry packet validation
- Command packet creation
- Time utilities
- Sensor value ranges

**Run coordinator tests:**
```bash
cd coordinator
pio test -e native
```

#### Node Tests
Similar structure in `node/test/`

**Run node tests:**
```bash
cd node
pio test -e native
```

### 4. System Integration Tests

End-to-end tests covering:
- Node → Coordinator → Backend → Frontend data flow
- MQTT topic alignment
- Command execution
- Pairing process
- Zone control

**Requirements:**
- All services running (backend, frontend, MQTT, MongoDB)
- Physical or simulated coordinator and nodes

## Running Tests

### Quick Start - Run All Tests

```bash
# Backend
cd IOT-Backend-main/IOT-Backend-main && go test -v ./...

# Frontend
cd IOT-Frontend-main/IOT-Frontend-main && npm test

# Firmware
cd coordinator && pio test -e native
cd ../node && pio test -e native
```

### Development Workflow

1. **Write code**
2. **Run relevant tests:**
   ```bash
   # For backend changes
   go test -v ./internal/mqtt
   
   # For frontend changes
   npm test -- --include='**/component.spec.ts'
   
   # For firmware changes
   pio test -e native
   ```
3. **Check coverage:**
   ```bash
   go test -cover ./...
   ```
4. **Run full suite before commit:**
   ```bash
   ./run-all-tests.sh  # See below
   ```

### Automated Test Script

Create `run-all-tests.sh`:
```bash
#!/bin/bash
set -e

echo "=== Running Backend Tests ==="
cd IOT-Backend-main/IOT-Backend-main
go test -v -race ./...

echo ""
echo "=== Running Frontend Tests ==="
cd ../../IOT-Frontend-main/IOT-Frontend-main
npm test -- --watch=false --browsers=ChromeHeadless

echo ""
echo "=== Running Coordinator Tests ==="
cd ../../coordinator
pio test -e native

echo ""
echo "=== Running Node Tests ==="
cd ../node
pio test -e native

echo ""
echo "✅ All tests passed!"
```

## Test Coverage

### Coverage Goals
- **Backend**: >80% code coverage
- **Frontend**: >70% code coverage
- **Firmware**: >60% code coverage (hardware-dependent code excluded)

### View Coverage Reports

**Backend:**
```bash
go test -coverprofile=coverage.out ./...
go tool cover -html=coverage.out
```

**Frontend:**
```bash
npm test -- --code-coverage
open coverage/index.html
```

### Coverage in CI/CD

Coverage reports are automatically generated and uploaded to Codecov on every push/PR.

View reports at: `https://codecov.io/gh/YOUR-ORG/IOT-TileNodeCoordinator`

## CI/CD Integration

### GitHub Actions Workflows

1. **Backend Tests** (`.github/workflows/backend-tests.yml`)
   - Runs on: Push to main/develop, PRs
   - Services: MongoDB, Mosquitto MQTT
   - Steps: Test → Build → Lint
   - Artifacts: Coverage reports, binaries

2. **Frontend Tests** (`.github/workflows/frontend-tests.yml`)
   - Runs on: Push to main/develop, PRs
   - Steps: Unit tests → Build → E2E tests
   - Artifacts: Coverage, build dist, E2E reports

3. **Firmware Tests** (`.github/workflows/firmware-tests.yml`)
   - Runs on: Push to main/develop, PRs
   - Steps: Build coordinator → Build node → Static analysis
   - Artifacts: Firmware binaries, analysis reports

### Status Badges

Add to README.md:
```markdown
![Backend Tests](https://github.com/YOUR-ORG/IOT-TileNodeCoordinator/workflows/Backend%20Tests/badge.svg)
![Frontend Tests](https://github.com/YOUR-ORG/IOT-TileNodeCoordinator/workflows/Frontend%20Tests/badge.svg)
![Firmware Tests](https://github.com/YOUR-ORG/IOT-TileNodeCoordinator/workflows/Firmware%20Tests/badge.svg)
[![codecov](https://codecov.io/gh/YOUR-ORG/IOT-TileNodeCoordinator/branch/main/graph/badge.svg)](https://codecov.io/gh/YOUR-ORG/IOT-TileNodeCoordinator)
```

## Writing Tests

### Backend Test Example (Go)

```go
func TestMQTT_PublishTelemetry(t *testing.T) {
    // Arrange
    mockClient := &MockMQTTClient{connected: true}
    handler := &Handler{client: mockClient}
    
    telemetry := types.NodeTelemetry{
        NodeID: "node-001",
        Temperature: 25.5,
    }
    
    // Act
    err := handler.PublishNodeTelemetry("node-001", &telemetry)
    
    // Assert
    if err != nil {
        t.Fatalf("Failed: %v", err)
    }
    
    if len(mockClient.published) != 1 {
        t.Errorf("Expected 1 message, got %d", len(mockClient.published))
    }
}
```

### Frontend Test Example (TypeScript)

```typescript
describe('CoordinatorComponent', () => {
  it('should display coordinator name', () => {
    const fixture = TestBed.createComponent(CoordinatorComponent);
    fixture.componentInstance.coordinator = {
      name: 'Test Coordinator',
      siteId: 'site-001'
    };
    fixture.detectChanges();
    
    const compiled = fixture.nativeElement;
    expect(compiled.querySelector('h1').textContent)
      .toContain('Test Coordinator');
  });
});
```

### Firmware Test Example (C++)

```cpp
void test_node_registry_add_node() {
    NodeRegistry registry;
    uint8_t mac[6] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    
    bool result = registry.addNode(mac);
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(registry.isNodeRegistered(mac));
}
```

## Test Best Practices

1. **Keep tests independent**: Each test should run in isolation
2. **Use descriptive names**: `test_mqtt_publishes_telemetry_successfully`
3. **Follow AAA pattern**: Arrange → Act → Assert
4. **Mock external dependencies**: Database, MQTT, network
5. **Test edge cases**: Invalid input, null values, boundary conditions
6. **Keep tests fast**: Unit tests < 100ms, integration tests < 5s
7. **Use setup/teardown**: Clean state between tests
8. **Assert one thing**: One logical assertion per test
9. **Don't test framework code**: Focus on your business logic
10. **Keep coverage meaningful**: 100% coverage ≠ good tests

## Debugging Tests

### Backend
```bash
# Run specific test
go test -v -run TestMQTT_PublishTelemetry

# Debug with Delve
dlv test -- -test.run TestMQTT_PublishTelemetry
```

### Frontend
```bash
# Run specific spec
npm test -- --include='**/coordinator.component.spec.ts'

# Debug in browser
npm test -- --browsers=Chrome --watch
```

### Firmware
```bash
# Verbose output
pio test -e native -v

# Upload and monitor
pio test -e esp32-s3-devkitc-1 --upload-port COM3
```

## Continuous Improvement

- Review test failures in CI/CD
- Update tests when requirements change
- Add tests for bug fixes
- Refactor tests with code
- Monitor coverage trends
- Collect feedback on test quality

## Resources

- [Go Testing Documentation](https://golang.org/pkg/testing/)
- [Angular Testing Guide](https://angular.io/guide/testing)
- [Playwright Documentation](https://playwright.dev/)
- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/)
- [Jest Documentation](https://jestjs.io/)

## Support

For questions or issues with tests:
1. Check existing test examples
2. Review this guide
3. Ask in team chat
4. Create an issue on GitHub
