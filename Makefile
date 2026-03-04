CC = gcc
CFLAGS = -Wall -Wextra -g -I.

C_SOURCES = client.c requests.c helpers.c parson.c buffer.c
C_OBJECTS = $(C_SOURCES:.c=.o)

EXECUTABLE = client

.PHONY: all clean  # Spune-i lui make că acestea nu sunt fișiere

all: $(EXECUTABLE)

$(EXECUTABLE): $(C_OBJECTS)
	$(CC) $(LDFLAGS) $(C_OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(C_OBJECTS) $(EXECUTABLE)