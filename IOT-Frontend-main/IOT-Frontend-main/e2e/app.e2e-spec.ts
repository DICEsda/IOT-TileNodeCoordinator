import { test, expect } from '@playwright/test';

test.describe('IOT TileNode Coordinator E2E Tests', () => {
  
  test.beforeEach(async ({ page }) => {
    await page.goto('http://localhost:4200');
  });

  test('should load the application', async ({ page }) => {
    await expect(page).toHaveTitle(/IOT/i);
  });

  test('should display login page', async ({ page }) => {
    await expect(page.locator('app-login')).toBeVisible();
  });

  test('should login successfully with valid credentials', async ({ page }) => {
    await page.fill('input[name="username"]', 'admin');
    await page.fill('input[name="password"]', 'admin123');
    await page.click('button[type="submit"]');
    
    // Should redirect to dashboard
    await expect(page).toHaveURL(/dashboard/);
  });

  test('should show error with invalid credentials', async ({ page }) => {
    await page.fill('input[name="username"]', 'invalid');
    await page.fill('input[name="password"]', 'wrong');
    await page.click('button[type="submit"]');
    
    await expect(page.locator('.error-message')).toBeVisible();
  });

  test('should display coordinators list', async ({ page }) => {
    // Login first
    await page.fill('input[name="username"]', 'admin');
    await page.fill('input[name="password"]', 'admin123');
    await page.click('button[type="submit"]');
    
    await page.waitForURL(/dashboard/);
    await page.click('a[href*="coordinators"]');
    
    await expect(page.locator('.coordinator-list')).toBeVisible();
  });

  test('should create new coordinator', async ({ page }) => {
    // Login and navigate
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/coordinators');
    
    // Click create button
    await page.click('button:has-text("Create Coordinator")');
    
    // Fill form
    await page.fill('input[name="name"]', 'Test Coordinator');
    await page.fill('input[name="siteId"]', 'site-test-001');
    await page.fill('input[name="location"]', 'Test Lab');
    
    // Submit
    await page.click('button[type="submit"]');
    
    // Verify created
    await expect(page.locator('text=Test Coordinator')).toBeVisible();
  });

  test('should display nodes for coordinator', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/coordinators/coord-001/nodes');
    
    await expect(page.locator('.node-list')).toBeVisible();
  });

  test('should show real-time telemetry updates', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/dashboard');
    
    // Wait for WebSocket connection
    await page.waitForTimeout(2000);
    
    // Check if telemetry is updating
    const tempElement = page.locator('.temperature-value');
    await expect(tempElement).toBeVisible();
  });

  test('should send pairing command', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/coordinators/coord-001');
    
    // Click pairing button
    await page.click('button:has-text("Start Pairing")');
    
    // Verify pairing mode indicator
    await expect(page.locator('.pairing-active')).toBeVisible();
  });

  test('should control zone lighting', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/zones');
    
    // Select zone
    await page.click('.zone-card:first-child');
    
    // Adjust brightness
    const brightnessSlider = page.locator('input[type="range"][name="brightness"]');
    await brightnessSlider.fill('80');
    
    // Verify value
    await expect(brightnessSlider).toHaveValue('80');
  });

  test('should display system status', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/dashboard');
    
    await expect(page.locator('.system-status')).toBeVisible();
    await expect(page.locator('.coordinator-count')).toBeVisible();
    await expect(page.locator('.node-count')).toBeVisible();
  });

  test('should handle WebSocket disconnection', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/dashboard');
    
    // Simulate offline
    await page.context().setOffline(true);
    
    // Should show disconnected status
    await expect(page.locator('.connection-status.offline')).toBeVisible();
    
    // Go back online
    await page.context().setOffline(false);
    await page.waitForTimeout(1000);
    
    // Should reconnect
    await expect(page.locator('.connection-status.online')).toBeVisible();
  });

  test('should filter nodes by status', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/nodes');
    
    // Select online filter
    await page.selectOption('select[name="status"]', 'online');
    
    // Verify filtered results
    const nodes = page.locator('.node-card');
    const count = await nodes.count();
    
    for (let i = 0; i < count; i++) {
      await expect(nodes.nth(i).locator('.status-badge')).toHaveText('online');
    }
  });

  test('should display telemetry charts', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/telemetry');
    
    await expect(page.locator('canvas.temperature-chart')).toBeVisible();
    await expect(page.locator('canvas.brightness-chart')).toBeVisible();
  });

  test('should update node configuration', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/nodes/node-001/config');
    
    // Update zone
    await page.selectOption('select[name="zone"]', '2');
    
    // Save
    await page.click('button:has-text("Save")');
    
    // Verify success message
    await expect(page.locator('.success-message')).toBeVisible();
  });

  test('should handle API errors gracefully', async ({ page }) => {
    // Mock API error
    await page.route('**/api/coordinators', route => {
      route.fulfill({
        status: 500,
        body: JSON.stringify({ error: 'Internal Server Error' })
      });
    });
    
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/coordinators');
    
    // Should show error message
    await expect(page.locator('.error-notification')).toBeVisible();
  });

  test('should logout successfully', async ({ page }) => {
    await loginAsAdmin(page);
    
    // Click logout
    await page.click('button:has-text("Logout")');
    
    // Should redirect to login
    await expect(page).toHaveURL(/login/);
  });

  test('should display mmWave presence detection', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/coordinators/coord-001');
    
    await expect(page.locator('.mmwave-indicator')).toBeVisible();
  });

  test('should show node health status', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/nodes');
    
    const healthBadge = page.locator('.health-badge').first();
    await expect(healthBadge).toBeVisible();
  });

  test('should export telemetry data', async ({ page }) => {
    await loginAsAdmin(page);
    await page.goto('http://localhost:4200/telemetry');
    
    // Listen for download
    const downloadPromise = page.waitForEvent('download');
    await page.click('button:has-text("Export")');
    const download = await downloadPromise;
    
    // Verify download
    expect(download.suggestedFilename()).toMatch(/telemetry.*\.csv/);
  });

  test('should support dark mode toggle', async ({ page }) => {
    await loginAsAdmin(page);
    
    // Toggle dark mode
    await page.click('button.theme-toggle');
    
    // Verify dark mode applied
    await expect(page.locator('body')).toHaveClass(/dark-theme/);
  });

  test('should display responsive layout on mobile', async ({ page, viewport }) => {
    await page.setViewportSize({ width: 375, height: 667 }); // iPhone SE size
    
    await loginAsAdmin(page);
    
    // Check mobile menu
    await expect(page.locator('.mobile-menu-toggle')).toBeVisible();
  });
});

// Helper function
async function loginAsAdmin(page: any) {
  await page.goto('http://localhost:4200/login');
  await page.fill('input[name="username"]', 'admin');
  await page.fill('input[name="password"]', 'admin123');
  await page.click('button[type="submit"]');
  await page.waitForURL(/dashboard/);
}
