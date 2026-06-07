package main

import (
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"time"

	"github.com/openbao/openbao/sdk/v2/framework"
	"github.com/openbao/openbao/sdk/v2/logical"
)

func pathAuthToken(b *backend) *framework.Path {
	return &framework.Path{
		Pattern: "auth-token/(?P<name>[a-zA-Z0-9_.-]+)",
		Fields: map[string]*framework.FieldSchema{
			"name": {
				Type:        framework.TypeString,
				Description: "The name/purpose of the token, used in the key's description",
			},
			"reusable": {
				Type:        framework.TypeBool,
				Description: "Whether the generated key is reusable across multiple devices",
				Default:     false,
			},
			"ephemeral": {
				Type:        framework.TypeBool,
				Description: "Whether the device created with this key is ephemeral (removed when offline)",
				Default:     true,
			},
			"preauthorized": {
				Type:        framework.TypeBool,
				Description: "Whether the device is preauthorized (skips admin console approval)",
				Default:     true,
			},
			"tags": {
				Type:        framework.TypeCommaStringSlice,
				Description: "Comma-separated list of tags to assign to the device (must start with tag:)",
				Default:     []string{"tag:docker"},
			},
			"expiry_seconds": {
				Type:        framework.TypeInt,
				Description: "Expiry of the Tailscale auth key in seconds (default 3600)",
				Default:     3600,
			},
		},
		Operations: map[logical.Operation]framework.OperationHandler{
			logical.ReadOperation: &framework.PathOperation{Callback: b.pathAuthTokenRead},
		},
		HelpSynopsis:    "Generate a new dynamic Tailscale authentication key with a custom name.",
		HelpDescription: "Reads from this endpoint dynamically call the Tailscale API to generate a new device auth key, with the name in the path used as the key description.",
	}
}

type tailscaleDeviceCreate struct {
	Reusable      bool     `json:"reusable"`
	Ephemeral     bool     `json:"ephemeral"`
	Preauthorized bool     `json:"preauthorized"`
	Tags          []string `json:"tags,omitempty"`
}

type tailscaleDevices struct {
	Create tailscaleDeviceCreate `json:"create"`
}

type tailscaleCapabilities struct {
	Devices tailscaleDevices `json:"devices"`
}

type tailscaleKeyReq struct {
	Capabilities  tailscaleCapabilities `json:"capabilities"`
	ExpirySeconds int                   `json:"expirySeconds"`
	Description   string                `json:"description"`
}

type tailscaleKeyResp struct {
	ID           string    `json:"id"`
	Key          string    `json:"key"`
	Description  string    `json:"description"`
	Created      time.Time `json:"created"`
	Expires      time.Time `json:"expires"`
}

func (b *backend) pathAuthTokenRead(ctx context.Context, req *logical.Request, data *framework.FieldData) (*logical.Response, error) {
	config, err := b.getConfig(ctx, req.Storage)
	if err != nil {
		return nil, fmt.Errorf("failed to retrieve config: %w", err)
	}
	if config == nil || config.APIKey == "" || config.Tailnet == "" {
		return nil, errors.New("plugin is not configured: api_key and tailnet must be set")
	}

	name := data.Get("name").(string)
	if name == "" {
		return nil, errors.New("name is required in the path (e.g. auth-token/<name>)")
	}

	reusable := data.Get("reusable").(bool)
	ephemeral := data.Get("ephemeral").(bool)
	preauthorized := data.Get("preauthorized").(bool)
	tags := data.Get("tags").([]string)
	expirySeconds := data.Get("expiry_seconds").(int)

	// Build Tailscale API request body
	tsReqBody := tailscaleKeyReq{
		Capabilities: tailscaleCapabilities{
			Devices: tailscaleDevices{
				Create: tailscaleDeviceCreate{
					Reusable:      reusable,
					Ephemeral:     ephemeral,
					Preauthorized: preauthorized,
					Tags:          tags,
				},
			},
		},
		ExpirySeconds: expirySeconds,
		Description:   fmt.Sprintf("OpenBao dynamic token for %s", name),
	}

	reqBytes, err := json.Marshal(tsReqBody)
	if err != nil {
		return nil, fmt.Errorf("failed to marshal request: %w", err)
	}

	apiURL := fmt.Sprintf("%s/api/v2/tailnet/%s/keys", config.BaseURL, config.Tailnet)
	httpReq, err := http.NewRequestWithContext(ctx, "POST", apiURL, bytes.NewBuffer(reqBytes))
	if err != nil {
		return nil, fmt.Errorf("failed to create http request: %w", err)
	}

	httpReq.Header.Set("Content-Type", "application/json")
	// Tailscale API accepts the API Key via Basic Auth (API key as username, empty password)
	httpReq.SetBasicAuth(config.APIKey, "")

	client := &http.Client{
		Timeout: 15 * time.Second,
	}

	resp, err := client.Do(httpReq)
	if err != nil {
		return nil, fmt.Errorf("http request to tailscale failed: %w", err)
	}
	defer resp.Body.Close()

	if resp.StatusCode != http.StatusCreated && resp.StatusCode != http.StatusOK {
		var errResp map[string]interface{}
		_ = json.NewDecoder(resp.Body).Decode(&errResp)
		return nil, fmt.Errorf("tailscale API returned status %d: %v", resp.StatusCode, errResp)
	}

	var tsResp tailscaleKeyResp
	if err := json.NewDecoder(resp.Body).Decode(&tsResp); err != nil {
		return nil, fmt.Errorf("failed to decode response: %w", err)
	}

	// Return the generated key. For dynamic secrets in OpenBao, we expose it inside the Data map.
	return &logical.Response{
		Data: map[string]interface{}{
			"auth_token":  tsResp.Key,
			"key_id":      tsResp.ID,
			"description": tsResp.Description,
			"expires":     tsResp.Expires.Format(time.RFC3339),
		},
	}, nil
}
