all: gobi_loader

%.o: %.c
	$(CC) $(CFLAGS) $(EXTRA_CFLAGS) -c -o $@ $^

gobi_loader: gobi_loader.o
	$(CC) -o $@ $^

clean:
	rm -f *.o gobi_loader
