raspi_camera: raspi_io.o raspi_SCCB.o
	cc -o raspi_camera raspi_io.o raspi_SCCB.o
raspi_io.o:raspi_io.c raspi_io.h
	cc -c raspi_io.c
raspi_SCCB.o:raspi_SCCB.c raspi_io.h raspi_ov7670.h
	cc -c raspi_SCCB.c
clean:
	rm raspi_camera raspi_SCCB.o raspi_io.o