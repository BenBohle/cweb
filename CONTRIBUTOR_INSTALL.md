# Manual Development Setup

This guide targets contributors who want to build and iterate on **both** the `libcweb` framework and the `cweb` CLI locally without installing system-wide packages.

---
## 1. Prerequisites

Ensure your development machine (Linux/WSL preferred) has:

```bash
sudo apt update
sudo apt install -y build-essential cmake pkg-config \
    libevent-dev libcurl4-openssl-dev libbrotli-dev zlib1g-dev \
    libmariadb3 libmariadb-dev libcjson-dev libssl-dev
```

---
## 2. Clone & Layout

```bash
cd ~/dev   # adjust as needed
git clone https://github.com/BenBohle/cweb.git
cd cweb
```

The repository contains:
- `framework/` – libcweb sources
- `console/` – cweb CLI sources
- `app/` – example application consuming libcweb

---
## 3. Build Framework Locally

Keep build artefacts in dedicated folders (`buildframework-dev`, etc.) to avoid clashes with install builds.

```bash
cmake -S framework -B buildframework-dev -DCMAKE_BUILD_TYPE=Debug 
cmake --build buildframework-dev
```

# Install globally (or /usr/local without prefix)
```bash
sudo cmake --install buildframework-dev --prefix /usr
```
# if you dont isntall to /usr/local, run:
```bash
sudo ldconfig
```

Verifiy with 
```bash
pkg-config --modversion cweb
pkg-config --cflags --libs cweb
```

# via .deb
```bash
cpack -G DEB
sudo apt install ./libcweb-dev-*.deb
```

This produces a shared library (`libcweb.so`) plus CMake and pkg-config metadata inside `buildframework-dev`.

---
## 4. Build CLI Against Local Framework

Force the CLI to link against the local framework by passing `-DCWEB_CLI_USE_LOCAL_FRAMEWORK=ON`.

```bash
cmake -S console -B buildconsole -DCMAKE_BUILD_TYPE=Debug
cmake --build buildconsole
```

# Install globally (or /usr/local without prefix)
```bash
sudo cmake --install buildconsole --prefix /usr
```

# via .deb
```bash
cmake --build buildframework -- -j$(nproc)
cpack -G DEB
sudo apt install ./cweb-cli-*.deb
```


Verifiy with 
```bash
which cweb
cweb --version
ldd $(which cweb)   
```

---
## 5. Regenerate Templates & Build Example App

The sample app in `app/` consumes the CLI to generate code.

# build the app linking against local framework
```bash
cmake -S . -B buildapp-dev -DCMAKE_BUILD_TYPE=Debug
cmake --build buildapp-dev
```

Run the app:

```bash
./buildapp-dev/cweb
```


Remove afterwards:

```bash
sudo apt remove libcweb-dev cweb-cli
```

---