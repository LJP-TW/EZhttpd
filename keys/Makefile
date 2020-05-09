CreateCA:
	echo 'generate CA key...'
	openssl genrsa -aes-256-cbc -out CA-key.pem 2048
	echo 'generate CA certificate...'
	openssl req -new -key CA-key.pem -x509 -days 1000 -out CA-cert.pem

CreateServerKeypair:
	openssl genrsa -aes-256-cbc -out server-key.pem 2048

GenerateServerCSR:
	openssl req -new -key server-key.pem -out server-cert.csr

SignServerCSR:
	openssl x509 -req -days 360 -in server-cert.csr -CA CA-cert.pem -CAkey CA-key.pem -CAcreateserial -out server-cert.pem -sha256 -extfile server.ext

clear:
	rm ./*.pem
	rm ./*.csr
	rm ./*.srl
