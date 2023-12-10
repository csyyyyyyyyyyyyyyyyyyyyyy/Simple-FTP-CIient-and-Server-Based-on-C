build:
	gcc ./src/myftpc.c ./src/client.c ./src/utils.c -o myftpc -w -lm -std=gnu99 -lsqlite3 -L/opt/homebrew/opt/openssl/lib -I/opt/homebrew/opt/openssl/include -lssl -lcrypto && \
    gcc ./src/myftpd.c ./src/server.c ./src/utils.c -o myftpd -w -lm -std=gnu99 -lsqlite3 -L/opt/homebrew/opt/openssl/lib -I/opt/homebrew/opt/openssl/include -lssl -lcrypto

allclean:
	$(MAKE) clean ; rm -rf .git
clean:
	rm myftpc myftpd *.out ; rm -rf .vscode t
tar:
	$(MAKE) clean ; tar czf ../myftp.tgz ./
rand:
	head -c 587654321 /dev/urandom > t