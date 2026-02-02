CFLAGS += -O3 -I../../src -I.
LINKFLAGS += -lm
OBJDIR = _build

SRC = $(wildcard *.c)
OBJ = $(patsubst %,$(OBJDIR)/%,$(SRC:%.c=%.o))
$(info SRC=$(SRC))
$(info OBJ=$(OBJ))

all: glb_to_lvl

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

glb_to_lvl: $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LINKFLAGS)

clean:
	rm -rf ./_build ./glb_to_lvl
