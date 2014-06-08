PREFIX=/home/guest/trunk/staging_dir
export STAGING_DIR=/home/guest/trunk/staging_dir
GCC=$(PREFIX)/toolchain/bin/mips-openwrt-linux-gcc
LD=$(PREFIX)/toolchain/bin/mips-openwrt-linux-ld
ALLFLAGS= -O2 -I$(PREFIX)/toolchain/include/ -I$(PREFIX)/target/usr/include/ -L$(PREFIX)/toolchain/lib/ -L$(PREFIX)/target/usr/lib/ -lpcap -lm -lcurl -lpolarssl -lz -lcrypto -pthread -lmicrohttpd
GCCFLAGS= -O2 -I$(PREFIX)/toolchain/include/ -I$(PREFIX)/target/usr/include/
LDFLAGS=-L$(PREFIX)/toolchain/lib/ -L$(PREFIX)/target/usr/lib/ -lpcap -lm -lcurl -lssl -lz -lcrypto -pthread -lmicrohttpd

TARGET= router


router: *.c *.h

	$(GCC) $(ALLFLAGS) *.c -o router

clean:
	rm -f *.o

backupclean:
	rm -f *~ 
