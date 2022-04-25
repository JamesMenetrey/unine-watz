FROM mcr.microsoft.com/dotnet/sdk:5.0-focal

# ==============================
# Configuration static variables
# ==============================
ARG WASI_SDK_VERSION=12
ARG POLYBENCHC_ROOT_DIR=/polybenchc


# ==============================
# .NET configuration
# ==============================

# Enable another user than root to execute dotnet
ENV DOTNET_CLI_HOME="/tmp/"

# No telemetry
ENV DOTNET_CLI_TELEMETRY_OPTOUT=1

# No startup logo
ENV DOTNET_NOLOGO=true


# ======================
# WASM Compilation tools
# ======================

# WASI SDK
WORKDIR /opt
RUN wget --quiet https://github.com/WebAssembly/wasi-sdk/releases/download/wasi-sdk-${WASI_SDK_VERSION}/wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz  && \
    tar xf wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz && \
    rm wasi-sdk-${WASI_SDK_VERSION}.0-linux.tar.gz && \
    mv wasi-sdk-${WASI_SDK_VERSION}.0 wasi-sdk

ENV PATH="$PATH:/opt/wasi-sdk"

WORKDIR ${POLYBENCHC_ROOT_DIR}

ENTRYPOINT [ "dotnet", "run", "--project", "utilities/compile-wasm/CompileWasm.csproj", "-c", "Release", "--" ]