package main

import (
	"context"

	"github.com/openbao/openbao/sdk/v2/framework"
	"github.com/openbao/openbao/sdk/v2/logical"
)

type backend struct {
	*framework.Backend
}

// Backend returns a new logical.Backend implementation for Tailscale
func Backend(ctx context.Context, c *logical.BackendConfig) (logical.Backend, error) {
	var b backend
	b.Backend = &framework.Backend{
		Help: "Tailscale dynamic auth token secrets engine for OpenBao",
		PathsSpecial: &logical.Paths{
			SealWrapStorage: []string{
				"config",
			},
		},
		Paths: []*framework.Path{
			pathConfig(&b),
			pathAuthToken(&b),
		},
		BackendType: logical.TypeLogical,
	}
	return &b, nil
}
