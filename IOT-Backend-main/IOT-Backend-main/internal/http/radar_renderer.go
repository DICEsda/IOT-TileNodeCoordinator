package http

import (
	"bytes"
	"image/png"
	"math"

	"github.com/DICEsda/IOT-TileNodeCoordinator/backend/internal/types"
	"github.com/fogleman/gg"
)

type RadarRenderer struct {
	width          int
	height         int
	detectionRange float64 // meters
	detectionAngle float64 // degrees
}

func NewRadarRenderer() *RadarRenderer {
	return &RadarRenderer{
		width:          600,
		height:         600,
		detectionRange: 6.0,  // 6 meters (HLK-LD2450 range)
		detectionAngle: 120.0, // 120 degrees
	}
}

// RenderRadar generates a PNG image of the radar with targets
func (r *RadarRenderer) RenderRadar(frame *types.MmwaveFrame) ([]byte, error) {
	// Create drawing context
	dc := gg.NewContext(r.width, r.height)

	// Dark background
	dc.SetRGB(0.05, 0.05, 0.05)
	dc.Clear()

	centerX := float64(r.width) / 2
	centerY := float64(r.height) - 50 // Bottom center

	// Scale: pixels per meter
	scale := (float64(r.height) - 100) / r.detectionRange

	// Draw detection cone (120° arc)
	r.drawDetectionCone(dc, centerX, centerY, scale)

	// Draw concentric range circles
	r.drawRangeCircles(dc, centerX, centerY, scale)

	// Draw radial grid lines
	r.drawRadialLines(dc, centerX, centerY, scale)

	// Draw radar sensor at origin
	r.drawRadarSensor(dc, centerX, centerY)

	// Draw targets
	if frame != nil && len(frame.Targets) > 0 {
		r.drawTargets(dc, centerX, centerY, scale, frame.Targets)
	}

	// Draw info text
	r.drawInfo(dc, frame)

	// Encode to PNG
	var buf bytes.Buffer
	if err := png.Encode(&buf, dc.Image()); err != nil {
		return nil, err
	}

	return buf.Bytes(), nil
}

func (r *RadarRenderer) drawDetectionCone(dc *gg.Context, cx, cy, scale float64) {
	// Draw 120° detection cone
	halfAngle := r.detectionAngle / 2 * math.Pi / 180
	radius := r.detectionRange * scale

	// Fill detection zone with subtle color
	dc.SetRGBA(0.1, 0.1, 0.15, 0.3)
	dc.NewSubPath()
	dc.MoveTo(cx, cy)
	
	// Draw arc from -60° to +60°
	startAngle := -math.Pi/2 - halfAngle
	endAngle := -math.Pi/2 + halfAngle
	
	for angle := startAngle; angle <= endAngle; angle += 0.02 {
		x := cx + radius*math.Cos(angle)
		y := cy + radius*math.Sin(angle)
		dc.LineTo(x, y)
	}
	dc.LineTo(cx, cy)
	dc.Fill()

	// Draw cone boundary lines
	dc.SetRGBA(0.5, 0.5, 0.5, 0.8)
	dc.SetLineWidth(2)
	
	// Left edge
	leftX := cx + radius*math.Cos(startAngle)
	leftY := cy + radius*math.Sin(startAngle)
	dc.DrawLine(cx, cy, leftX, leftY)
	dc.Stroke()
	
	// Right edge
	rightX := cx + radius*math.Cos(endAngle)
	rightY := cy + radius*math.Sin(endAngle)
	dc.DrawLine(cx, cy, rightX, rightY)
	dc.Stroke()
}

func (r *RadarRenderer) drawRangeCircles(dc *gg.Context, cx, cy, scale float64) {
	dc.SetRGBA(0.3, 0.3, 0.3, 0.5)
	dc.SetLineWidth(1)

	halfAngle := r.detectionAngle / 2 * math.Pi / 180
	startAngle := -math.Pi/2 - halfAngle
	endAngle := -math.Pi/2 + halfAngle

	// Draw concentric arcs for each meter
	for i := 1; i <= int(r.detectionRange); i++ {
		radius := float64(i) * scale
		dc.NewSubPath()
		dc.DrawArc(cx, cy, radius, startAngle, endAngle)
		dc.Stroke()

		// Label distance
		if i%2 == 0 {
			labelX := cx
			labelY := cy - radius - 5
			dc.SetRGBA(0.5, 0.5, 0.5, 1.0)
			dc.DrawStringAnchored(string(rune('0'+i))+"m", labelX, labelY, 0.5, 1)
		}
	}
}

func (r *RadarRenderer) drawRadialLines(dc *gg.Context, cx, cy, scale float64) {
	dc.SetRGBA(0.2, 0.2, 0.2, 0.3)
	dc.SetLineWidth(1)

	angles := []float64{-60, -45, -30, 0, 30, 45, 60} // degrees
	radius := r.detectionRange * scale

	for _, deg := range angles {
		angle := (deg - 90) * math.Pi / 180
		x := cx + radius*math.Cos(angle)
		y := cy + radius*math.Sin(angle)
		dc.DrawLine(cx, cy, x, y)
		dc.Stroke()
	}
}

func (r *RadarRenderer) drawRadarSensor(dc *gg.Context, cx, cy float64) {
	// Draw glowing sensor at origin
	dc.SetRGBA(0.8, 0.8, 1.0, 1.0)
	dc.DrawCircle(cx, cy, 5)
	dc.Fill()

	// Outer glow
	dc.SetRGBA(0.5, 0.5, 0.8, 0.3)
	dc.DrawCircle(cx, cy, 10)
	dc.Fill()
}

func (r *RadarRenderer) drawTargets(dc *gg.Context, cx, cy, scale float64, targets []types.MmwaveTarget) {
	for i, target := range targets {
		// Convert mm to meters, then to pixels
		x := float64(target.PositionXMM) / 1000.0 * scale
		y := float64(target.PositionYMM) / 1000.0 * scale

		// Target position (y points forward, x points sideways)
		targetX := cx + x
		targetY := cy - y // Negative because canvas Y increases downward

		// Skip if outside detection cone
		if y < 0 || y > r.detectionRange*scale {
			continue
		}

		// Color based on target index
		brightness := 1.0 - float64(i)*0.2
		if brightness < 0.5 {
			brightness = 0.5
		}

		// Draw target with glow
		dc.SetRGBA(brightness, brightness, brightness, 1.0)
		dc.DrawCircle(targetX, targetY, 8)
		dc.Fill()

		// Outer glow
		dc.SetRGBA(brightness, brightness, brightness, 0.3)
		dc.DrawCircle(targetX, targetY, 15)
		dc.Fill()

		// Draw target ID
		dc.SetRGBA(1, 1, 1, 0.8)
		dc.DrawStringAnchored(string(rune('0'+target.ID)), targetX, targetY-20, 0.5, 0.5)
	}
}

func (r *RadarRenderer) drawInfo(dc *gg.Context, frame *types.MmwaveFrame) {
	dc.SetRGBA(0.7, 0.7, 0.7, 1.0)

	if frame != nil {
		// Top left info
		info := ""
		if frame.Presence {
			info += "● PRESENCE DETECTED\n"
		} else {
			info += "○ No Presence\n"
		}
		info += "Targets: " + string(rune('0'+len(frame.Targets)))
		
		dc.DrawStringAnchored(info, 20, 20, 0, 0)
	} else {
		dc.DrawStringAnchored("Waiting for radar data...", 20, 20, 0, 0)
	}

	// Bottom legend
	dc.SetRGBA(0.5, 0.5, 0.5, 1.0)
	dc.DrawStringAnchored("HLK-LD2450 mmWave Radar | 6m Range | 120° FOV", 
		float64(r.width)/2, float64(r.height)-10, 0.5, 1)
}
