FLAGS=CC=gcc LD=gcc LDFLAGS=-lpthread NAME=vernamtunnel
all:
	$(MAKE) -C vernam $(FLAGS)

install:
	$(MAKE) -C vernam $(FLAGS) install

uninstall:
	$(MAKE) -C vernam $(FLAGS) uninstall

clean:
	$(MAKE) -C vernam $(FLAGS) clean

