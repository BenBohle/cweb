
![Logo](http://cwebframework.com/assets/cweb.gif)



# CWEB

A high perfomance Full Stack Web Framework fully written in C


## Installation

### Recommended: package repository setup

The easiest way on Debian/Ubuntu, Fedora/RHEL and Alpine is the cweb archive setup script:

```bash
curl -fsSL https://archive.cwebframework.com/setup.sh | sudo -E sh
```

The script automatically:

- detects `apt`, `dnf`/`yum` or `apk`
- installs the matching cweb repository signing key
- adds the cweb package repository
- refreshes the package cache

After that, install the framework and CLI:

```bash
# Debian / Ubuntu
sudo apt install -y libcweb-dev cweb-cli

# Fedora / RHEL
sudo dnf install -y libcweb-dev cweb-cli

# Alpine
sudo apk add libcweb-dev cweb-cli
```

On very small Docker images you may need to install `curl` and CA certificates before downloading the setup script:

```bash
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y ca-certificates curl
curl -fsSL https://archive.cwebframework.com/setup.sh | sh
```

### Manual repository setup

If you do not want to run the setup script, you can add the repository manually.

Debian / Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y ca-certificates curl
sudo install -d -m 0755 /etc/apt/keyrings
curl -fsSL https://archive.cwebframework.com/keys/cweb-archive.asc \
  | sudo tee /etc/apt/keyrings/cweb-archive.asc >/dev/null
echo 'deb [signed-by=/etc/apt/keyrings/cweb-archive.asc] https://archive.cwebframework.com/apt/ ./' \
  | sudo tee /etc/apt/sources.list.d/cweb.list
sudo apt update
sudo apt install -y libcweb-dev cweb-cli
```

Fedora / RHEL:

```bash
sudo rpm --import https://archive.cwebframework.com/keys/RPM-GPG-KEY-cweb
sudo tee /etc/yum.repos.d/cweb.repo >/dev/null <<'EOF'
[cweb]
name=cweb Packages
baseurl=https://archive.cwebframework.com/rpm/
enabled=1
gpgcheck=1
repo_gpgcheck=1
gpgkey=https://archive.cwebframework.com/keys/RPM-GPG-KEY-cweb
EOF
sudo dnf install -y libcweb-dev cweb-cli
```

Alpine:

```bash
curl -fsSL https://archive.cwebframework.com/keys/cweb-apk.rsa.pub \
  -o /etc/apk/keys/cweb-apk.rsa.pub
echo 'https://archive.cwebframework.com/apk/' >> /etc/apk/repositories
apk update
apk add libcweb-dev cweb-cli
```

### Development with Docker Compose

For local development you can also use Docker Compose. This is mainly useful on Windows systems, in fresh dev environments, or when you do not want to install all build dependencies directly on your machine.

```bash
docker compose up -d mariadb
docker compose run --rm app
```

The compose setup is intended for development only. For normal usage, prefer installing `libcweb-dev` and `cweb-cli` from the package repository.
    
## Documentation

[Documentation](https://cwebframework.com)

[Contribution Guide](CONTRIBUTOR_INSTALL.md)


## Related


[STARTER REPO](https://github.com/BenBohle/cweb_starter)


## Features

- TemplateEngine (write c Code and HTML mixed)
- cwagger (swaggger like auto api documentation)
- In memory FileCache/FileServer 
- Auto Routing
- compression support
- http/1.1
- fetch 
- Debug tools like Logger
- and many more..

## Authors

- [@BenBohle](https://www.github.com/BenBohle)


## Contact

Email: kontakt@benbohle.de