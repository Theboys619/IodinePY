start:
	clang++ -std=c++17 src/main.cpp

build32:
	g++ -std=c++17 src/main.cpp

clean:
	rm main