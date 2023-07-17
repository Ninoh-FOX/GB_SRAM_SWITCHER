TARGET = gameboy_srm

CC = $(CROSS_COMPILE)g++

SOURCES = .
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
OFILES = $(CFILES:.cpp=.o)

ifeq ($(CROSS_COMPILE),)
  CFLAGS = -DPLATFORM_PC -O3 -Wall
  LDFLAGS = -lSDL -lSDL_ttf -lSDL_image -s
else
  CFLAGS = -O3 -marm -mtune=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard -march=armv7ve -Wall
  LDFLAGS = -lSDL -lSDL_ttf -lSDL_image -lmi_sys -lmi_gfx -s
endif

$(TARGET): $(OFILES)
	$(CC) $(OFILES) -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OFILES)
