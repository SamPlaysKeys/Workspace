package main

import (
	"context"
	"errors"

	"github.com/openbao/openbao/sdk/v2/framework"
	"github.com/openbao/openbao/sdk/v2/logical"
)

type configData struct {
	APIKey  string `json:"api_key"`
	Tailnet string `json:"tailnet"`
	BaseURL string `json:"base_url"`
}

func pathConfig(b *backend) *framework.Path {
	return &framework.Path{
		Pattern: "config",
		Fields: map[string]*framework.FieldSchema{
			"api_key": {
				Type:        framework.TypeString,
				Description: "Tailscale API Key or Access Token",
				Required:    true,
			},
			"tailnet": {
				Type:        framework.TypeString,
				Description: "Tailscale Tailnet name (org/account ID)",
				Required:    true,
			},
			"base_url": {
				Type:        framework.TypeString,
				Description: "Tailscale API Base URL",
				Default:     "https://api.tailscale.com",
			},
		},
		Operations: map[logical.Operation]framework.OperationHandler{
			logical.CreateOperation: &framework.PathOperation{Callback: b.pathConfigWrite},
			logical.UpdateOperation: &framework.PathOperation{Callback: b.pathConfigWrite},
			logical.ReadOperation:   &framework.PathOperation{Callback: b.pathConfigRead},
		},
		HelpSynopsis:    "Configure the Tailscale secrets engine API access.",
		HelpDescription: "Stores the Tailscale API key and Tailnet configuration to allow the engine to request auth tokens dynamically.",
	}
}

func (b *backend) pathConfigWrite(ctx context.Context, req *logical.Request, data *framework.FieldData) (*logical.Response, error) {
	apiKey := data.Get("api_key").(string)
	tailnet := data.Get("tailnet").(string)
	baseURL := data.Get("base_url").(string)

	if apiKey == "" {
		return nil, errors.New("api_key is required")
	}
	if tailnet == "" {
		return nil, errors.New("tailnet is required")
	}

	config := &configData{
		APIKey:  apiKey,
		Tailnet: tailnet,
		BaseURL: baseURL,
	}

	entry, err := logical.StorageEntryJSON("config", config)
	if err != nil {
		return nil, err
	}

	if err := req.Storage.Put(ctx, entry); err != nil {
		return nil, err
	}

	return nil, nil
}

func (b *backend) pathConfigRead(ctx context.Context, req *logical.Request, data *framework.FieldData) (*logical.Response, error) {
	config, err := b.getConfig(ctx, req.Storage)
	if err != nil {
		return nil, err
	}
	if config == nil {
		return nil, nil
	}

	return &logical.Response{
		Data: map[string]interface{}{
			"tailnet":  config.Tailnet,
			"base_url": config.BaseURL,
			// Do not expose the actual API key in plain text
			"api_key":  "********",
		},
	}, nil
}

func (b *backend) getConfig(ctx context.Context, s logical.Storage) (*configData, error) {
	entry, err := s.Get(ctx, "config")
	if err != nil {
		return nil, err
	}
	if entry == nil {
		return nil, nil
	}

	var config configData
	if err := entry.DecodeJSON(&config); err != nil {
		return nil, err
	}

	if config.BaseURL == "" {
		config.BaseURL = "https://api.tailscale.com"
	}

	return &config, nil
}
