raspi_camera: raspi_io.o raspi_ov7670.o 
	g++ -o raspi_camera raspi_io.o raspi_ov7670.o 
raspi_io.o:raspi_io.c raspi_io.h
	gcc -c raspi_io.c
raspi_ov7670.o:raspi_ov7670.cpp raspi_io.h raspi_ov7670.h 
	gcc -c raspi_ov7670.cpp 
clean:
	rm raspi_camera raspi_ov7670.o raspi_io.o