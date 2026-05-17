
TARGET      := deltarune_wii
BUILD       := build
SOURCES     := source
INCLUDES    := include
DATA        := deltarune


ifeq ($(strip $(DEVKITPRO)),)
  $(error "DEVKITPRO not set")
endif

DEVKITPPC   := $(DEVKITPRO)/devkitPPC
export PATH := $(DEVKITPPC)/bin:$(PATH)


CC      := powerpc-eabi-gcc
CXX     := powerpc-eabi-g++


CFLAGS   := -g -O2 -Wall -DGEKKO -mrvl
CXXFLAGS := $(CFLAGS) -std=gnu++17


INCLUDE  := -I$(CURDIR)/$(INCLUDES) \
            -I$(DEVKITPRO)/libogc/include \
            -I$(DEVKITPRO)/portlibs/ppc/include \
            -I$(DEVKITPRO)/portlibs/wii/include


LIBS := \
-lgrrlib \
-lpngu \
-lpng \
-lfreetype \
-lbrotlidec \
-lbrotlicommon \
-lbz2 \
-lz \
-lasnd \
-lwiiuse \
-lbte \
-lfat \
-logc \
-lm

LIBPATHS := \
-L$(DEVKITPRO)/portlibs/wii/lib \
-L$(DEVKITPRO)/portlibs/ppc/lib \
-L$(DEVKITPRO)/libogc/lib/wii

CPPFILES := $(wildcard $(SOURCES)/*.cpp)
CFILES   := $(wildcard $(SOURCES)/*.c)
SFILES   := $(wildcard $(SOURCES)/*.s)

OFILES := $(addprefix $(BUILD)/,$(notdir $(CPPFILES:.cpp=.o))) \
          $(addprefix $(BUILD)/,$(notdir $(CFILES:.c=.o))) \
          $(addprefix $(BUILD)/,$(notdir $(SFILES:.s=.o)))


BINFILES := $(wildcard $(DATA)/*.*)
DATAOFILES := $(addprefix $(BUILD)/,$(notdir $(BINFILES:=.o)))


all: $(BUILD) $(TARGET).dol

$(BUILD):
	@mkdir -p $(BUILD)

$(TARGET).elf: $(OFILES) $(DATAOFILES)
	@echo Linking...
	@$(CXX) $(CXXFLAGS) $^ $(LIBPATHS) $(LIBS) -o $@

$(TARGET).dol: $(TARGET).elf
	@$(DEVKITPRO)/tools/bin/elf2dol $< $@
	@echo Built OK


$(BUILD)/%.o: $(SOURCES)/%.cpp
	@echo Compiling $<
	@$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.c
	@echo Compiling $<
	@$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(BUILD)/%.o: $(SOURCES)/%.s
	@echo Assembling $<
	@$(CC) -x assembler-with-cpp -c $< -o $@


$(BUILD)/%.o: $(DATA)/%
	@echo Embedding asset $<
	@$(DEVKITPRO)/tools/bin/bin2s -a 32 $< > $(BUILD)/$*.s
	@$(CC) -x assembler-with-cpp -c $(BUILD)/$*.s -o $@

clean:
	@rm -rf $(BUILD) $(TARGET).elf $(TARGET).dol
	@echo Cleaned