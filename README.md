# l3-streamingserver

Serveur streaming musical de fichier wav gérant le multi-client.
Il se compose de 4 fichiers :
- server.c : Fichier permettant de créer un serveur qui envoie en streaming une musique
- client.c : Fichier permettant de demander une musique au serveur
- lecteur.c : Fichier apportant des fonctions permettant une gestion aisé de la lecture des fichiers wav
- socketlvl2.c : Fichier apportant des fonctions permettant une gestion aisé des sockets

## Compilation
`make`
## Démarrer le serveur audio
`./audioserver`
## Démarrer le client
`./audioclient nomdufichier`

On peut demander l'application de filtres au server

`./audioclient nomdufichier [MONO|VOLUME|ECHO]`

Ce qui peut donner une commande comme celle ci :

`./audioclient nomdufichier ECHO MONO`

Le serveur enverra alors le son streaming du fichier en mono avec un effet d'echo
