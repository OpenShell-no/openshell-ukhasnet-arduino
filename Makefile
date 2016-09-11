DEVICE  = m328p
CLOCK		= 8000000

BUILDDIR = ./build/
ASSETS   = ./assets/

SOURCES = $(wildcard *.cpp) $(wildcard */*.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
LIBRARIES = $(wildcard ../libraries/*/)

INCLUDES = $(patsubst %,-I %,$(LIBRARIES))
# Compiler flags. Optimise for code size. Allow C99 standards.
COMPILE = avr-g++ -Wall -Wextra -pedantic -Os -gdwarf-2 -std=c++1y -DF_CPU=$(CLOCK) -D'AVR=' -mmcu=atmega328p $(INCLUDES)

all: firmware_version.h main.hex

firmware_version.h:
	python ./hg_hooks/versionheader.py

.cpp.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@



main.elf: $(OBJECTS)
	$(COMPILE) -o main.elf $(OBJECTS)
	avr-size -C --mcu=$(DEVICE) $@

main.hex: main.elf
	rm -f main.hex
	avr-objcopy -j .text -j .data -O ihex main.elf main.hex
	avr-objcopy -j .eeprom -O ihex main.elf main.eep

.PHONY: clean
clean:
	rm -fq main.hex main.elf main.eep $(OBJECTS)

#vpath %.o $(BUILDDIR)
