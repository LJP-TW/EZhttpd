# EZhttpd

Support:
- TLS
- Read Normal File / HTML page
- Keep-Alive Mechanism
- Execute CGI program

## Environment
- Ubuntu 18.04
- gcc version 7.5.0

## Build
```
sudo apt-get install libssl-dev
git clone https://github.com/LJP-TW/EZhttpd.git
cd EZhttpd
make
```

## Usage
```
./bin/ezhttpd -i [ipv4] -p [port] -f [config file]
```

for example:
```
./bin/ezhttpd -i 0.0.0.0 -p 20002 -f config/server_config_ssl
```

open your browser to view index.html ;)

## Config File
| Key | Description | Example |
| -------- | -------- | -------- |
| ENABLE_SSL | Set it to 1 to support TLS/SSL | ENABLE_SSL=1 |
| SERVER_CERT |  Path of server certificate | SERVER_CERT=./keys/server-cert.pem |
| SERVER_PKEY |  Path of server private key | SERVER_PKEY=./keys/server-key.pem |
| WEB_ROOT | Root of web directory | WEB_ROOT=www/ |
| LIST_FILES | Set it to 1 to list files in dir | LIST_FILES=1 |
| DOWNLOADABLE | Set it to 1 to make file downloadable | DOWNLOADABLE=1 |

## CGI
There are many testing cgi program in www/cgi-bin, and their source codes are in www/cgi-src

run `touch /tmp/tmp_db` to create a fake database for `insert.cgi` `search.cgi` `view.cgi`

## TLS
### Key 
Check out ./keys/

