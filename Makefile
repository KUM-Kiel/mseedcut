build/mseedcut: src/mseedcut.c src/progress.c src/progress.h src/options.c src/options.h Makefile
	@mkdir -p build
	$(CC) -o build/mseedcut src/mseedcut.c src/progress.c src/options.c -lpthread

clean:
	rm -rf build/
