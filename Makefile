
all:
	gcc -lfuse -D_FILE_OFFSET_BITS=64 -DFUSE_USE_VERSION=22 dir.c -o dir

clean:
	rm -rf dir
