all:
	gcc -g fs.c crypto.c aes.c mounter.c filefs.c -o filefs
