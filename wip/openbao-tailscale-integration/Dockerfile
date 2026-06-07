# Dockerfile to build the OpenBao plugin binaries for macOS and Linux
FROM golang:1.24-alpine AS builder

RUN apk add --no-cache git

WORKDIR /app

# Copy go.mod and prepare dependencies
COPY go.mod ./
RUN go mod tidy

# Copy codebase
COPY . .

# Cross-compile for Linux (amd64 / arm64) and macOS (darwin amd64 / arm64) with CGO disabled
RUN CGO_ENABLED=0 GOOS=linux GOARCH=amd64 go build -ldflags="-s -w" -o openbao-plugin-secrets-tailscale-linux-amd64 .
RUN CGO_ENABLED=0 GOOS=linux GOARCH=arm64 go build -ldflags="-s -w" -o openbao-plugin-secrets-tailscale-linux-arm64 .
RUN CGO_ENABLED=0 GOOS=darwin GOARCH=amd64 go build -ldflags="-s -w" -o openbao-plugin-secrets-tailscale-darwin-amd64 .
RUN CGO_ENABLED=0 GOOS=darwin GOARCH=arm64 go build -ldflags="-s -w" -o openbao-plugin-secrets-tailscale-darwin-arm64 .

# Minimal output container
FROM alpine:3.18
WORKDIR /dist
COPY --from=builder /app/openbao-plugin-secrets-tailscale-* ./

# Default command copies build artifacts to a /out mount
CMD ["sh", "-c", "cp -v /dist/openbao-plugin-secrets-tailscale-* /out/"]
