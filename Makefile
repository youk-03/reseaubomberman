CC=gcc
CFLAGS=-DMAC -Wall 
DEPS= format_messages.h serveur/joueur.h serveur/partie.h client/auxiliaire.h
EXEC=serveur/serveur client/client

all : serveur/serveur client/client


client/client : client/client.o client/auxiliaire.o
	$(CC) -o $@ $^ $(LIBS) 
	
client/%.o: %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

serveur/serveur : serveur/serveur.o serveur/partie.o serveur/joueur.o
	$(CC) -o $@ $^ $(LIBS)

serveur/%.o: %.c $(DEPS)
	$(CC) -o $@ -c $< $(CFLAGS)

clean :
	rm -rf $(EXEC) */*.o