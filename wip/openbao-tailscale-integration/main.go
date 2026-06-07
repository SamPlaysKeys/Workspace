package main

import (
	"context"
	"log"
	"os"

	"github.com/openbao/openbao/sdk/v2/logical"
	"github.com/openbao/openbao/sdk/v2/plugin"
)

func main() {
	providerFunc := func(ctx context.Context, c *logical.BackendConfig) (logical.Backend, error) {
		return Backend(ctx, c)
	}

	err := plugin.Serve(&plugin.ServeOpts{
		BackendFactoryFunc: providerFunc,
	})
	if err != nil {
		log.Println(err)
		os.Exit(1)
	}
}
