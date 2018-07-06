openssl genrsa -out private.key 2048;
openssl rsa -in private.key -pubout -out public.key;
mv -f *.key ../src/config
