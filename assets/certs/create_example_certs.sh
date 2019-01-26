#!/bin/bash
# Copyright (c) 2018 Denis Trofimov (den.a.trofimov@yandex.ru)
# Distributed under the MIT License.
# See accompanying file LICENSE.md or copy at http://opensource.org/licenses/MIT

set -ev

# Used to create certs for localhost

# Don`t forget to add it to Chromium browser
# Chromium -> Setting -> (Advanced) Manage Certificates -> Import -> 'server_rootCA.pem'

# Read https://robmclarty.com/blog/how-to-secure-your-web-app-using-https-with-letsencrypt
# Read https://medium.freecodecamp.org/how-to-get-https-working-on-your-local-development-environment-in-5-minutes-7af615770eec
# Read https://maxrival.com/sozdaniie-dh-diffie-hellman-siertifikata/

# clean old certs
rm server.csr.cnf || true
rm v3.ext || true
rm *.key || true
rm *.pem || true
rm *.crt || true
rm *.csr || true


cat << EOF > server.csr.cnf
[req]
default_bits = 2048
prompt = no
default_md = sha256
distinguished_name = dn

[dn]
C=US
ST=RandomState
L=RandomCity
O=RandomOrganization
OU=RandomOrganizationUnit
emailAddress=hello@example.com
CN = localhost
EOF

cat << EOF > v3.ext
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
EOF

# root certificate can then be used to sign any number of certificates you might generate for individual domains
# root key
openssl genrsa -des3 -out rootCA.key 2048
# root pem
openssl req -x509 -new -nodes -key rootCA.key -sha256 -days 10000 -out rootCA.pem

# https://forum.seafile.com/t/tutorial-for-a-complete-certificate-chain-with-your-own-certificate-authority-ca/124
openssl dhparam -out dh.pem 4096

# Domain SSL certificate
# issue a certificate specifically for your local development environment located at localhost.
# server.csr.cnf so you can import these settings when creating a certificate instead of entering them on the command line.
# v3.ext file in order to create a X509 v3 certificate.

# Create a certificate key for localhost using the configuration settings stored in server.csr.cnf. This key is stored in server.key.
openssl req -new -sha256 -nodes -out server.csr -newkey rsa:2048 -keyout server.key -config server.csr.cnf
# sign created certificate
openssl x509 -req -in server.csr -CA rootCA.pem -CAkey rootCA.key -CAcreateserial -out server.crt -days 10000 -sha256 -extfile v3.ext

chmod 400 server.key
chmod 400 rootCA.key

# TODO check certs https://gist.github.com/webtobesocial/5313b0d7abc25e06c2d78f8b767d4bc3
