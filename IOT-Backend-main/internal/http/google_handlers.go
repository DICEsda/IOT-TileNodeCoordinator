package http

import (
	"encoding/json"
	"net/http"
)

// ============================================================================
// Google Home Integration Handlers
// ============================================================================

// DisconnectGoogleHomeRequest represents a disconnect request
type DisconnectGoogleHomeRequest struct {
	SiteID string `json:"site_id"`
}

// InitiateGoogleAuth starts the OAuth flow
func (h *Handler) InitiateGoogleAuth(w http.ResponseWriter, r *http.Request) {
	// Redirect to Google OAuth authorization page
	// This URL should be constructed using the credentials from settings
	authURL := "https://accounts.google.com/o/oauth2/v2/auth?" +
		"client_id=YOUR_CLIENT_ID&" +
		"redirect_uri=http://localhost:8000/api/v1/google/callback&" +
		"response_type=code&" +
		"scope=https://www.googleapis.com/auth/homegraph&" +
		"access_type=offline"

	http.Redirect(w, r, authURL, http.StatusTemporaryRedirect)
}

// GoogleAuthCallback handles the OAuth callback
func (h *Handler) GoogleAuthCallback(w http.ResponseWriter, r *http.Request) {
	code := r.URL.Query().Get("code")
	if code == "" {
		http.Error(w, "No authorization code received", http.StatusBadRequest)
		return
	}

	// TODO: Exchange authorization code for access token
	// TODO: Store tokens in database

	// Send success message to opener window
	html := `
	<!DOCTYPE html>
	<html>
	<head>
		<title>Authorization Successful</title>
	</head>
	<body>
		<h2>Authorization Successful!</h2>
		<p>You can close this window.</p>
		<script>
			if (window.opener) {
				window.opener.postMessage({type: 'google-auth-success'}, '*');
				window.close();
			}
		</script>
	</body>
	</html>
	`
	w.Header().Set("Content-Type", "text/html")
	w.Write([]byte(html))
}

// DisconnectGoogleHome removes Google Home integration
func (h *Handler) DisconnectGoogleHome(w http.ResponseWriter, r *http.Request) {
	var req DisconnectGoogleHomeRequest
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, "Invalid request body", http.StatusBadRequest)
		return
	}

	// TODO: Remove stored tokens from database
	// TODO: Notify Google to unlink account

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(map[string]string{
		"status":  "success",
		"message": "Google Home disconnected",
	})
}
