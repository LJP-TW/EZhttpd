# EZhttpd

Support:
    - Read Normal File / HTML page
    - Execute CGI program

## Environment
- Ubuntu 18.04
- gcc version 7.4.0

## Build
```
git clone https://github.com/LJP-TW/EZhttpd.git
cd EZhttpd
make
```

## Usage
```
./bin/ezhttpd [ipv4] [port] [root]
```

for example:
```
./bin/ezhttpd 172.17.0.2 8787 ./www
```

open your browser to view index.html ;)

## CGI
There are many testing cgi program in www/cgi-bin, and their source codes are in www/cgi-src

run `touch /tmp/tmp_db` to create a fake database for `insert.cgi` `search.cgi` `view.cgi`

