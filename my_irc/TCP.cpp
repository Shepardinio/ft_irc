#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>

int main()
{
    // 🎯 Étape 1 : Créer un socket
    // socket(domain, type, protocol)
    // AF_INET = IPv4, SOCK_STREAM = TCP, 0 = protocole par défaut
    int listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening == -1)
    {
        std::cerr << "Can't create a socket!" << std::endl;
        return -1;
    }

    // 🎯 Étape 2 : Lier le socket à une adresse IP et un port
    sockaddr_in hint;                    // structure contenant l’adresse du serveur
    hint.sin_family = AF_INET;           // Utilisation d'IPv4
    hint.sin_port = htons(54000);        // Port 54000 (htons = conversion en format réseau)
    inet_pton(AF_INET, "0.0.0.0", &hint.sin_addr); // "0.0.0.0" = écouter sur toutes les interfaces

    // bind() attache le socket à cette adresse IP et port
    if (bind(listening, (sockaddr*)&hint, sizeof(hint)) == -1)
    {
        std::cerr << "Can't bind to IP/port" << std::endl;
        return -2;
    }

    // 🎯 Étape 3 : Mettre le socket en mode "écoute"
    // SOMAXCONN = nombre max de connexions autorisées dans la file d’attente
    if (listen(listening, SOMAXCONN) == -1)
    {
        std::cerr << "Can't listen!" << std::endl;
        return -3;
    }

    // 🎯 Étape 4 : Attendre une connexion client
    sockaddr_in client;                  // Structure pour stocker les infos du client
    socklen_t clientSize = sizeof(client); // Taille de la structure
    char host[NI_MAXHOST];              // Nom du client
    char svc[NI_MAXSERV];               // Service (port)

    int clientSocket = accept(listening, (sockaddr *)&client, &clientSize);
    /*Elle attend qu'un client se connecte au serveur, et une fois qu’un client tente de se connecter, la fonction :
    -accepte la connexion,
    -remplit la structure client avec les informations (adresse IP et port du client),
    -et retourne un nouveau socket (ici clientSocket) qu’on utilisera pour communiquer avec ce client en particulier.

    Explication des arguments :
    -listening : le socket du serveur (créé avec socket() et mis en écoute avec listen()).
    -(sockaddr*)&client : pointeur vers une structure sockaddr_in qui sera remplie avec l’adresse du client.
    -&clientSize : la taille de la structure client, nécessaire pour que accept() sache combien d'octets il peut remplir.

    En résumé :
    On attend une connexion. Dès qu’un client arrive, on accepte la connexion, on récupère ses infos, et on obtient un nouveau socket (clientSocket) qu’on utilise pour parler avec ce client uniquement. */

    if (clientSocket == -1)
    {
        std::cerr << "Problem with client connecting!" << std::endl;
        return -4;
    }

    // 🎯 Étape 5 : Fermer le socket d'écoute (on ne veut plus de nouveaux clients)
    close(listening);

    // 🎯 Étape 6 : Afficher les infos du client connecté
    memset(host, 0, NI_MAXHOST);
    memset(svc, 0, NI_MAXSERV);

    int result = getnameinfo((sockaddr*)&client, sizeof(client), host, NI_MAXHOST, svc, NI_MAXSERV, 0);
    /*Elle essaie de récupérer le nom d’hôte (nom du client, comme "machine.local") et le service (généralement le numéro de port, sous forme de texte).

    Explication des arguments :
    -(sockaddr*)&client : l’adresse du client, récupérée via accept().
    -sizeof(client) : taille de cette structure.
    -host : un tableau de caractères (vide au départ) où sera stocké le nom d'hôte.
    -NI_MAXHOST : taille maximale que peut contenir host.
    -svc : tableau où sera stocké le nom du service (souvent, le numéro de port sous forme de chaîne).
    -NI_MAXSERV : taille maximale pour svc.
    -0 : options (on n’utilise aucun flag ici).

    Résultat :
    -Si getnameinfo réussit, host et svc contiennent le nom du client et le service.
    -Sinon, on utilise inet_ntop pour afficher directement l’IP et le port sans passer par les noms.

    En résumé :
    On essaie de convertir l’adresse IP du client en un nom compréhensible (comme ordinateur.local) + le port utilisé (en texte). Si ça ne marche pas, on affiche directement l’IP chiffrée.*/

    if (result)
    {
        std::cout << host << " connected on " << svc << std::endl;
    }
    else
    {
        // Si on n’a pas pu obtenir le nom, on affiche l'adresse IP et le port
        inet_ntop(AF_INET, &client.sin_addr, host, NI_MAXHOST);
        std::cout << host << " connected on " << ntohs(client.sin_port) << std::endl;
    }

    // 🎯 Étape 7 : Boucle de réception / envoi
    char buf[4096]; // Buffer pour recevoir les messages

    while (true)
    {
        memset(buf, 0, 4096); // Vider le buffer à chaque tour

        // recv() attend un message du client (jusqu’à 4096 octets)
        int bytesRecv = recv(clientSocket, buf, 4096, 0);
        if (bytesRecv == -1)
        {
            std::cerr << "There was a connection issue" << std::endl;
            break;
        }

        if (bytesRecv == 0)
        {
            std::cout << "The client disconnected" << std::endl;
            break;
        }

        // Afficher le message reçu dans le terminal
        std::cout << "Received: " << std::string(buf, 0, bytesRecv) << std::endl;

        // Réenvoyer le même message au client (effet "echo")
        send(clientSocket, buf, bytesRecv + 1, 0); // (bytesRecv + 1) = on envoie le '\0' aussi
    }

    // 🎯 Étape 8 : Fermer la connexion avec le client
    close(clientSocket);

    return 0;
}