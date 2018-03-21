#define _XOPEN_SOURCE 700   /* So as to allow use of `fdopen` and `getline`.  */

#include "server_thread.h"

//Added includes
#include "../protocol/communication.h"
#include <semaphore.h>

#include <netinet/in.h>
#include <netdb.h>

#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <signal.h>

#include <time.h>

enum { NUL = '\0' };

enum {
  /* Configuration constants.  */
  max_wait_time = 30,
  server_backlog_size = 5
};

int server_socket_fd;

// Nombre de client enregistré.
int nb_registered_clients;

// Variable du journal.
// Nombre de requêtes acceptées immédiatement (ACK envoyé en réponse à REQ).
unsigned int count_accepted = 0;

// Nombre de requêtes acceptées après un délai (ACK après REQ, mais retardé).
unsigned int count_wait = 0;

// Nombre de requête erronées (ERR envoyé en réponse à REQ).
unsigned int count_invalid = 0;

// Nombre de clients qui se sont terminés correctement
// (ACK envoyé en réponse à CLO).
unsigned int count_dispatched = 0;

// Nombre total de requête (REQ) traités.
unsigned int request_processed = 0;

// Nombre de clients ayant envoyé le message CLO.
unsigned int clients_ended = 0;

//Mutex
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Banker's Algorithm data
int * bankersAlgoAvailable;
int * bankersAlgoMaxResources;
unsigned int bankersAlgoNbResources;

static void sigint_handler(int signum) {
  // Code terminaison.
  accepting_connections = 0;
}

void
st_init()
{
	// Handle interrupt
	signal(SIGINT, &sigint_handler);

	// Initialise le nombre de clients connecté.
	nb_registered_clients = 0;

	// TODO

	// Attend la connection d'un client et initialise les structures pour
	// l'algorithme du banquier.
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);
	int socket = accept(server_socket_fd, (struct sockaddr *)&addr, &len);
	struct communication_data reading_data;
	struct communication_data writing_data;

	while (socket < 0) {
		socket = accept(server_socket_fd, (struct sockaddr *)&addr, &len);
	}

	FILE * socket_r = fdopen(socket, "r");
	FILE * socket_w = fdopen(socket, "w");

	//BEG
	read_communication(socket_r, &reading_data);

	printf("\n\nflag before BEG while loop\n\n");
	while (reading_data.communication_type != BEG) {
		free(&reading_data.args);
		read_communication(socket_r, &reading_data);
	}
	if (reading_data.communication_type == BEG) {
		printf("\n\n---BEG---");
	}
	printf("\n\nFlag after\n\n");
	writing_data.communication_type = ACK;
	write_communication(socket_w, &writing_data);
	bankersAlgoNbResources = reading_data.args[0];

	//PRO
	read_communication(socket_r, &reading_data);
	printf("\n\nflag before PRO while loop\n\n");
	while (reading_data.communication_type != PRO) {
		read_communication(socket_r, &reading_data);
	}
	if (reading_data.communication_type == PRO) {
		printf("\n\n---PRO---");
	}
	printf("\n\nFlag after\n\n");
	printf("\n\nPRO");
	bankersAlgoAvailable = calloc(bankersAlgoNbResources, sizeof(int));
	bankersAlgoMaxResources = calloc(bankersAlgoNbResources, sizeof(int));
	for (int i = 0; i < reading_data.args_count; i++) {
		printf(" %d", reading_data.args[i]);
		bankersAlgoMaxResources[i] = reading_data.args[i];
		bankersAlgoAvailable[i] = reading_data.args[i];
	}
	printf("\n\n");
	writing_data.communication_type = ACK;
	write_communication(socket_w, &writing_data);

	fclose(socket_r);
	fclose(socket_w);
	close(socket);
}

void
st_process_requests (server_thread * st, int socket_fd)
{
  FILE *socket_r = fdopen (socket_fd, "r");
  FILE *socket_w = fdopen (socket_fd, "w");

  struct communication_data reading_data;
  struct communication_data writing_data;

  //INI
  read_communication(socket_r, &reading_data);
  printf("\n\nflag before INI while loop\n\n");
  while (reading_data.communication_type != INI) {
	  read_communication(socket_r, &reading_data);
  }
  if (reading_data.communication_type == INI) {
	  printf("\n\n---INI---");
  }
  printf("\n\nFlag after\n\n");
  printf("\n\nINI");
  for (int i = 0; i < reading_data.args_count; i++) {
	  printf(" %d", reading_data.args[i]);
  }
  printf("\n\n");
  writing_data.communication_type = ACK;
  write_communication(socket_w, &writing_data);

  while (true) {
	  read_communication(socket_r, &reading_data);

	  //printf("\n\nType: %d, Args[0]: %d, Args_count: %d\n\n", reading_data.communication_type, reading_data.args[0], reading_data.args_count);

	  //TODO: Set all bool in REQ and CLO to false when not debugging
	  
	  if (reading_data.communication_type == REQ) {
		  //REQ
		  bool requestAccepted = true;
		  bool requestProcessed = true;

		  //TODO: Set up banker's requests

		  if (requestAccepted) {
			  writing_data.communication_type = ACK;
			  write_communication(socket_w, &writing_data);
			  pthread_mutex_lock(&mutex);
			  count_accepted++;
			  pthread_mutex_unlock(&mutex);
		  }
		  else {
			  write_errorMessage(socket_w, "Request rejected.");
			  pthread_mutex_lock(&mutex);
			  count_invalid++;
			  pthread_mutex_unlock(&mutex);
		  }
		  if (requestProcessed) {
			  pthread_mutex_lock(&mutex);
			  request_processed++;
			  pthread_mutex_unlock(&mutex);
		  }
	  }
	  else if (reading_data.communication_type == CLO) {
		  //CLO
		  if (st_signal(socket_w)) {
			  //Exit while(true)
			  break;
		  }
	  }
  }
  printf("WHILE TRUE LOOP ABORTED");

  fclose (socket_r);
  fclose (socket_w);
}


bool
st_signal (FILE * socket_w)
{
	// TODO: mettre à false
	bool threadDispatched = true;
	struct communication_data writing_data;

	//TODO: close banker thread and set threadDispatched to true if thread could be dispatched

	if (threadDispatched) {
		writing_data.communication_type = ACK;
		write_communication(socket_w, &writing_data);
		pthread_mutex_lock(&mutex);
		count_dispatched++;
		pthread_mutex_unlock(&mutex);
	}

	return threadDispatched;
}

int st_wait() {
  struct sockaddr_in thread_addr;
  socklen_t socket_len = sizeof (thread_addr);
  int thread_socket_fd = -1;
  int end_time = time (NULL) + max_wait_time;

  while(thread_socket_fd < 0 && accepting_connections) {
    thread_socket_fd = accept(server_socket_fd,
        (struct sockaddr *)&thread_addr,
        &socket_len);
    if (time(NULL) >= end_time) {
      break;
    }
  }
  return thread_socket_fd;
}

void *
st_code (void *param)
{
  server_thread *st = (server_thread *) param;

  int thread_socket_fd = -1;
 
  printf("\n\nServer thread %d\n\n", st->id);

  // Boucle de traitement des requêtes.
  while (accepting_connections)
  {
    // Wait for a I/O socket.
    thread_socket_fd = st_wait();
    if (thread_socket_fd < 0)
    {
      fprintf (stderr, "Time out on thread %d.\n", st->id);
      continue;
    }

    if (thread_socket_fd > 0)
    {
      st_process_requests (st, thread_socket_fd);
	  close(thread_socket_fd);
    }
  }
  return NULL;
}


//
// Ouvre un socket pour le serveur.
//
void
st_open_socket (int port_number)
{
  server_socket_fd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_socket_fd < 0) {
	  perror("ERROR opening socket");
	  exit(1);
  }

  if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0) {
    perror("setsockopt()");
    exit(1);
  }

  struct sockaddr_in serv_addr;
  memset (&serv_addr, 0, sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons (port_number);

  if (bind
      (server_socket_fd, (struct sockaddr *) &serv_addr,
       sizeof (serv_addr)) < 0)
    perror ("ERROR on binding");

  listen (server_socket_fd, server_backlog_size);
}


//
// Affiche les données recueillies lors de l'exécution du
// serveur.
// La branche else ne doit PAS être modifiée.
//
void
st_print_results (FILE * fd, bool verbose)
{
  if (fd == NULL) fd = stdout;
  if (verbose)
  {
    fprintf (fd, "\n---- Résultat du serveur ----\n");
    fprintf (fd, "Requêtes acceptées: %d\n", count_accepted);
	fprintf(fd, "Requêtes en attente: %d\n", count_wait);
    fprintf (fd, "Requêtes invalides: %d\n", count_invalid);
    fprintf (fd, "Clients : %d\n", count_dispatched);
    fprintf (fd, "Requêtes traitées: %d\n", request_processed);
  }
  else
  {
    fprintf (fd, "%d %d %d %d %d\n", count_accepted, count_wait,
        count_invalid, count_dispatched, request_processed);
  }
}


/***** MESSAGE COMMUNICATION PART *****/

//Reads client queries
void read_communication(FILE * socket_r, struct communication_data * data) {
	setup_communication_data(socket_r, data);
	if (data->communication_type == BEG) {
		fprintf(socket_r, "BEG %d\n", data->args[0]);
	}
	else if (data->communication_type == PRO) {
		fprintf(socket_r, "PRO");
		for (int i = 0; i < data->args_count; i++) {
			fprintf(socket_r, " %d", data->args[i]);
		}
		fprintf(socket_r, "\n");
	}
	else if (data->communication_type == END) {
		fprintf(socket_r, "END\n");
	}
	else if (data->communication_type == INI) {
		fprintf(socket_r, "INI");
		for (int i = 0; i < data->args_count; i++) {
			fprintf(socket_r, " %d", data->args[i]);
		}
		fprintf(socket_r, "\n");
	}
	else if (data->communication_type == REQ) {
		fprintf(socket_r, "REQ");
		for (int i = 0; i < data->args_count; i++) {
			fprintf(socket_r, " %d", data->args[i]);
		}
		fprintf(socket_r, "\n");
	}
	else if (data->communication_type == CLO) {
		fprintf(socket_r, "CLO %d\n", data->args[0]);
	}
}

void write_errorMessage(FILE * socket_w, char * errorMessage) {
	fprintf(socket_w, "ERR %s\n", errorMessage);
	fflush(socket_w);
}

//Writes to client
void write_communication(FILE * socket_w, struct communication_data * data) {
	if (data->communication_type == ACK) {
		printf("\n\nWRITING ACK");
		fprintf(socket_w, "ACK\n");
		printf("\nACK WRITTEN\n\n");
	}
	else if (data->communication_type == WAIT) {
		if (data->args_count == 1) {
			fprintf(socket_w, "WAIT %d\n", data->args[0]);
		}
	}
	fflush(socket_w);
}

//Set the data up for lecture
void setup_communication_data(FILE * socket_r, struct communication_data * data) {
	char comm_type[8];
	printf("\n\n flag 1\n\n");

	//Reads the communication_type
	fscanf(socket_r, "%s", comm_type);
	fgetc(socket_r);

	if (strcmp(comm_type, "BEG") == 0) {
		data->communication_type = BEG;
		data->args = malloc(sizeof(int));
		fscanf(socket_r, "%d", &data->args[0]);
		data->args_count = 1;
	}
	else if (strcmp(comm_type, "PRO") == 0) {
		data->communication_type = PRO;
		data->args_count = bankersAlgoNbResources;
		data->args = malloc(data->args_count * sizeof(int));
		for (int i = 0; i < data->args_count; i++) {
			fscanf(socket_r, "%d", &data->args[i]);
			fgetc(socket_r);
		}
	}
	else if (strcmp(comm_type, "END") == 0) {
		data->communication_type = END;
		data->args_count = 0;
	}
	else if (strcmp(comm_type, "INI") == 0) {
		data->communication_type = INI;
		data->args_count = bankersAlgoNbResources + 1;
		data->args = malloc(data->args_count * sizeof(int));
		for (int i = 0; i < data->args_count; i++) {
			fscanf(socket_r, "%d", &data->args[i]);
			fgetc(socket_r);
		}
	}
	else if (strcmp(comm_type, "REQ") == 0) {
		printf("\n\nREQUEST--------------------------------\n\n");
		data->communication_type = REQ;
		data->args_count = bankersAlgoNbResources + 1;
		data->args = malloc(data->args_count * sizeof(int));
		for (int i = 0; i < data->args_count; i++) {
			fscanf(socket_r, "%d", &data->args[i]);
			fgetc(socket_r);
		}
	}
	else if (strcmp(comm_type, "CLO") == 0) {
		data->communication_type = CLO;
		data->args_count = 1;
		data->args = malloc(sizeof(int));
		fscanf(socket_r, "%d", &data->args[0]);
		fgetc(socket_r);
	}
	else {
		data->communication_type = INVALID;
	}
}