Server se otvara na default portu 10000, a koristenjem flaga -p port_num  moze se otvoriti na proizvoljnom portu (npr. ./server -p 10001)

Kao dodatna funkcionalnost implementiran je directory index upotrebom programa mojls iz prve zadace.
Ova funkcionalnost je implementirana pomocu funkcije popen koja otvara pipe na osnovu stringa koji se sastoji od "./mojls " i putanje do trazenog foldera.
Izlaz iz ove funkcije se smjesta u tijelo http odgovora umjesto obicnog fajla.
Da bi funkcionalnost radila, potrebno je da izvrsni fajl pod nazivom "mojls" bude u istom direktoriju kao i server program.
