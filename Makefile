DEVICE  = m328p
CLOCK		= 8000000

BUILDDIR = ./build
ASSETS   = ./assets

LIBRARIES = ../libraries/ukhasnet-rfm69

SOURCES = $(wildcard *.cpp) $(wildcard */*.cpp)
OBJECTS = $(addprefix $(BUILDDIR)/,$(SOURCES:.cpp=.o)) ../libraries/ukhasnet-rfm69/ukhasnet-rfm69.o ../libraries/ukhasnet-rfm69/spi_conf/atmega168/spi_conf.o

INCLUDES = $(patsubst %,-I %,$(LIBRARIES))
# Compiler flags. Optimise for code size. Allow C99 standards.
COMPILE = avr-g++ -Wall -Wextra -pedantic -Os -gdwarf-2 -std=c++1y -DF_CPU=$(CLOCK) -D'AVR=' -mmcu=atmega328p $(INCLUDES)

all: firmware_version.h $(ASSETS)/main.hex $(ASSETS)/main.eep

# "::" means allways remake file.
firmware_version.h::
	python ./hg_hooks/versionheader.py

# TODO: do the same for .c files
$(BUILDDIR)/%.o: %.cpp firmware_version.h
	@mkdir -p "$(@D)"
	$(COMPILE) -c $< -o $@

.c.o:
	$(COMPILE) -c $< -o $@

.S.o:
	$(COMPILE) -x assembler-with-cpp -c $< -o $@

$(ASSETS):;mkdir -p $@

$(BUILDDIR)/main.elf: $(OBJECTS)
	$(COMPILE) -o $@ $(OBJECTS)
	avr-size -C --mcu=$(DEVICE) $@

$(ASSETS)/main.hex: $(BUILDDIR)/main.elf
	@mkdir -p "$(@D)"
	rm -f $@
	avr-objcopy -j .text -j .data -O ihex $< $@

$(ASSETS)/main.eep: $(BUILDDIR)/main.elf
	@mkdir -p "$(@D)"
	avr-objcopy -j .eeprom -O ihex $< $@

.PHONY: clean
clean:
	rm -rv $(BUILDDIR) $(ASSETS)

#vpath %.o $(BUILDDIR)
