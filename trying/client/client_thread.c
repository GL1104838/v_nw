/* This `define` tells unistd to define usleep and random.  */
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_thread.h"

//Added includes
#include "../protocol/communication.h"

// Socket library
//#include <netdb.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int port_number = -1;
int num_request_per_client = -1;
int num_resources = -1;
int *provisioned_resources = NULL;

// Variable d'initialisation des threads clients.
unsigned int count = 0;


// Variable du journal.
// Nombre de requête acceptée (ACK reçus en réponse à REQ)
unsigned int count_accepted = 0;

// Nombre de requête en attente (WAIT reçus en réponse à REQ)
unsigned int count_on_wait = 0;

// Nombre de requête refusée (REFUSE reçus en réponse à REQ)
unsigned int count_invalid = 0;

// Nombre de client qui se sont terminés correctement (ACC reçu en réponse à END)
unsigned int count_dispatched = 0;

// Nombre total de requêtes envoyées.
unsigned int request_sent = 0;

//Added Mutex 
pthread_mutex_t mutex;


void initializeServer() {
	int socket_fd = -1;

	// TP2 TODO
	// Connection au server.

	//initialize Mutex
	mutex = PTHREAD_MUTEX_INITIALIZER;

	//CODE ADAPTÉ DE http://liampaull.ca/courses/lectures/pdf/sockets.pdf
	struct sockaddr_in addr;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		perror("initializeServer socket() ");
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_number);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (connect(socket_fd, &addr, sizeof(addr)) < 0) {
		perror("initializeServer connect() ");
		exit(1);
	}
	else {
		printf("Server initialized");
	}

	FILE * socket_r = fdopen(socket_fd, "r");
	FILE * socket_w = fdopen(socket_fd, "w");

	//BEG 
	write_beg(socket_w, num_resources);
	struct communication_data beg_data;
	read_communication(socket_r, &beg_data);
	if (beg_data.communication_type == ACK) {
		printf("\n\nACKNOWLEDGED\n\n");
	}
	
	printf("\n\nflag before PRO\n\n");
	//PRO
	struct communication_data pro_data_w;
	struct communication_data pro_data_r;
	pro_data_w.communication_type = PRO;
	pro_data_w.args = malloc(num_resources * sizeof(int));
	for (int i = 0; i < num_resources; i++) {
		pro_data_w.args[i] = provisioned_resources[i];
	}
	pro_data_w.args_count = num_resources;
	write_communication(socket_w, &pro_data_w);
	printf("\n\nflag after PRO\n\n");
	read_communication(socket_r, &pro_data_r);
	if (pro_data_r.communication_type == ACK) {
		printf("\n\nACKNOWLEDGED\n\n");
	}

	fclose(socket_r);
	fclose(socket_w);
	close(socket_fd);
}

// Vous devez modifier cette fonction pour faire l'envoie des requêtes
// Les ressources demandées par la requête doivent être choisies aléatoirement
// (sans dépasser le maximum pour le client). Elles peuvent être positives
// ou négatives.
// Assurez-vous que la dernière requête d'un client libère toute les ressources
// qu'il a jusqu'alors accumulées.
void
send_request (int client_id, int request_id, int socket_fd, FILE * socket_r, FILE * socket_w)
{
	struct communication_data reading_data;
	struct communication_data writing_data;

  fprintf (stdout, "Client %d is sending its %d request\n", client_id,
      request_id);

  //REQ
  writing_data.communication_type = REQ;
  writing_data.args_count = num_resources + 1;
  writing_data.args = malloc(writing_data.args_count * sizeof(int));
  writing_data.args[0] = client_id;
  for (int i = 1; i < writing_data.args_count; i++) {
	  //SEND REQUEST
	  writing_data.args[i] = 1; ////////////////////////////////////////////////////TEMPORAIRE
  }
  write_communication(socket_w, &writing_data);

  pthread_mutex_lock(&mutex);
  request_sent++;
  pthread_mutex_unlock(&mutex);
}


void *
ct_code(void *param)
{
	int socket_fd = -1;
	client_thread *ct = (client_thread *)param;

	// TP2 TODO
	// Connection au server.

	//CODE ADAPTÉ DE http://liampaull.ca/courses/lectures/pdf/sockets.pdf
	struct sockaddr_in addr;
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) {
		perror("Client socket() ");
		exit(1);
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_number);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	if (connect(socket_fd, &addr, sizeof(addr)) < 0) {
		perror("Client connect() ");
		exit(1);
	}
	else {
		printf("Client %d connected\n", ct->id);
	}

	// Vous devez ici faire l'initialisation des petits clients (`INI`).
	FILE * socket_r = fdopen(socket_fd, "r");
	FILE * socket_w = fdopen(socket_fd, "w");

	struct communication_data comm_data;
	comm_data.communication_type = INI;
	comm_data.args_count = num_resources+1;
	comm_data.args = malloc(comm_data.args_count * sizeof(int));
	comm_data.args[0] = ct->id;
	//Initialize resource allocation (INI)
	for (int i = 0; i < num_resources; i++) {
		comm_data.args[i+1] = ct->maxResources[i];
	}
	write_communication(socket_w, &comm_data);
	struct communication_data ini_data_r;
	read_communication(socket_r, &ini_data_r);
	
  // TP2 TODO:END

  for (unsigned int request_id = 0; request_id < num_request_per_client;
      request_id++)
  {

    // TP2 TODO
    // Vous devez ici coder, conjointement avec le corps de send request,
    // le protocole d'envoi de requête.

    send_request (ct->id, request_id, socket_fd, socket_r, socket_w);

	struct communication_data reading_data;
	read_communication(socket_r, &reading_data);

	if (reading_data.communication_type == ACK) {
		pthread_mutex_lock(&mutex);
		count_accepted++;
		pthread_mutex_unlock(&mutex);
	}
	else if (reading_data.communication_type == ERR) {
		pthread_mutex_lock(&mutex);
		count_invalid++;
		pthread_mutex_unlock(&mutex);
	}
	else if (reading_data.communication_type == WAIT) {
		pthread_mutex_lock(&mutex);
		count_on_wait++;
		pthread_mutex_unlock(&mutex);
		sleep(reading_data.args[0]);
		request_id--;
	}

    // TP2 TODO:END

    /* Attendre un petit peu (0s-0.1s) pour simuler le calcul.  */
    usleep (random () % (100 * 1000));
    /* struct timespec delay;
     * delay.tv_nsec = random () % (100 * 1000000);
     * delay.tv_sec = 0;
     * nanosleep (&delay, NULL); */
  }

  //CLO
  struct communication_data data_clo;
  struct communication_data data_clo_ack;
  data_clo.communication_type = CLO;
  data_clo.args_count = 1;
  data_clo.args = malloc(sizeof(int));
  data_clo.args[0] = ct->id;
  write_communication(socket_w, &data_clo);

  read_communication(socket_r, &data_clo_ack);
  if (data_clo_ack.communication_type == ACK) {
	  pthread_mutex_lock(&mutex);
	  count_dispatched++;
	  pthread_mutex_unlock(&mutex);
  }
  else {
	  perror("Thread did not close properly.");
	  exit(1);
  }

  pthread_exit(NULL);

  return NULL;
}


//
// Vous devez changer le contenu de cette fonction afin de régler le
// problème de synchronisation de la terminaison.
// Le client doit attendre que le serveur termine le traitement de chacune
// de ses requêtes avant de terminer l'exécution.
//
void
ct_wait_server (client_thread * ct, int num_clients)
{
	sleep(15);
	/*for (int i = 0; i < num_clients; i++) {
		pthread_join(ct[i].pt_tid, NULL);
	}*/
}


void
ct_init (client_thread * ct)
{
  ct->id = count++;
  //Added to ct
  ct->maxResources = malloc(num_resources * sizeof(int));
  ct->allocatedResources = malloc(num_resources * sizeof(int));
  for (int i = 1; i < num_resources + 1; i++) {
	  ct->maxResources[i] = random() % provisioned_resources[i];
	  ct->allocatedResources[i] = 0;
  }
}

void
ct_create_and_start (client_thread * ct)
{
  pthread_attr_init (&(ct->pt_attr));
  pthread_create (&(ct->pt_tid), &(ct->pt_attr), &ct_code, ct);
  pthread_detach (ct->pt_tid);
}

//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
  if (fd == NULL)
    fd = stdout;
  if (verbose)
  {
    fprintf (fd, "\n---- Résultat du client ----\n");
    fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
    fprintf (fd, "Requêtes : %d\n", count_on_wait);
    fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
    fprintf (fd, "Clients : %d\n", count_dispatched);
    fprintf (fd, "Requêtes envoyées: %d\n", request_sent);
  }
  else
  {
    fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_on_wait,
        count_invalid, count_dispatched, request_sent);
  }
}

/***** MESSAGE COMMUNICATION PART *****/

//Reads server replies
void read_communication(FILE * socket_r, struct communication_data * data) {
	setup_communication_data(socket_r, data);
	if (data->communication_type == ACK) {
		fprintf(socket_r, "ACK\n");
	}
	else if (data->communication_type == WAIT) {
		fprintf(socket_r, "WAIT %d\n", data->args[0]);
	}
}

//Writes to server
void write_communication(FILE * socket_w, struct communication_data * data) {
	if (data->communication_type == PRO) {
		fprintf(socket_w, "PRO");
		for (int i = 0; i < data->args_count; i++) {
			fprintf(socket_w, " %d", data->args[i]);
		}
		fprintf(socket_w, "\n");
	}
	else if (data->communication_type == END) {
		fprintf(socket_w, "END\n");
	}
	else if (data->communication_type == INI) {
		fprintf(socket_w, "INI");
		for (int i = 0; i < data->args_count; i++) {
			fprintf(socket_w, " %d", data->args[i]);
		}
		fprintf(socket_w, "\n");
	}
	else if (data->communication_type == REQ) {
		fprintf(socket_w, "REQ");
		for (int i = 0; i < data->args_count; i++) {
			fprintf(socket_w, " %d", data->args[i]);
		}
		fprintf(socket_w, "\n");
	}
	else if (data->communication_type == CLO) {
		fprintf(socket_w, "CLO %d\n", data->args[0]);
	}

	fflush(socket_w);
}

//Writes beg to server
void write_beg(FILE * socket_w, int arg) {
	fprintf(socket_w, "BEG %d\n", arg);
	fflush(socket_w);
}

//Set the data up for lecture
void setup_communication_data(FILE * socket_r, struct communication_data * data) {

	char comm_type[5];
	//Reads the communication_type
	fscanf(socket_r, "%s", comm_type);
	fgetc(socket_r);

	if (strcmp(comm_type, "ACK") == 0) {
		data->communication_type = ACK;
		data->args_count = 0;
	}
	else if (strcmp(comm_type, "ERR") == 0) {
		data->communication_type = ERR;
		data->args_count = 0;
		char buffer[256];
		printf("\n\n");
		printf(fgets(buffer, 256, socket_r));
		printf("\n\n");
	}
	else if (strcmp(comm_type, "WAIT") == 0) {
		data->communication_type = WAIT;
		data->args = malloc(sizeof(int));
		fscanf(socket_r, "%d", &data->args[0]);
		data->args_count = 1;
	}
}