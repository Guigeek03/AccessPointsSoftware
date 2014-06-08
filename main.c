#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap.h>
#include <signal.h>
#include <pthread.h>
#include "pcap-thread.h"
#include "rssi_list.h"
#include <semaphore.h>
#include <time.h>
#include <net/if.h>
#include <sys/ioctl.h>


#define PORT 8080
#define GET 0
#define POST 1

typedef struct {
  char * interfaces;
} thread_params;

//________________________________Created lists______________________________________
extern volatile sig_atomic_t got_sigint;
Element * rssi_list = NULL;
sem_t semaphore;

//________________________________Prototypes___________________________________
u_char* intmac(const char* mac);
static int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls);
void *pcap_function(void *arg);
char * getMacRouter(char * mac);

void *pcap_function(void *arg)
{
  thread_params * tp = (thread_params*) arg; 
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t * handle = NULL;
  struct ieee80211_radiotap_header * rtap_head;
  struct ieee80211_header * eh;
  struct pcap_pkthdr header;
  const u_char * packet;
  u_char * mac;
  u_char first_flags;
  int offset = 0, val = 0;
  char rssi;
  Element * dev_info; //= malloc(sizeof(Element));
  //dev_info->measurements = malloc(sizeof(Deque));

  handle = pcap_open_live(tp->interfaces, BUFSIZ, 1, 1000, errbuf);
  if (handle == NULL) {
    printf("Could not open pcap on %s\n", tp->interfaces);
    pthread_exit((void *) -1);
  }
  
  while (1) {//got_sigint == 1
    packet = pcap_next(handle, &header);
    if (!packet)
      continue;
    rtap_head = (struct ieee80211_radiotap_header *) packet;
    int len = (int) rtap_head->it_len[0] + 256 * (int) rtap_head->it_len[1];
    eh = (struct ieee80211_header *) (packet + len);
      mac = eh->source_addr;
    if ((eh->frame_control & 0x03) == 0x01) {
      first_flags = rtap_head->it_present[0];
      offset = 8;
      offset += ((first_flags & 0x01) == 0x01) ? 8 : 0 ;
      offset += ((first_flags & 0x02) == 0x02) ? 1 : 0 ;
      offset += ((first_flags & 0x04) == 0x04) ? 1 : 0 ;
      offset += ((first_flags & 0x08) == 0x08) ? 4 : 0 ;
      offset += ((first_flags & 0x10) == 0x10) ? 2 : 0 ;
      rssi = *((char *) rtap_head + offset) - 0x100;
      printf("%d bytes -- %02X:%02X:%02X:%02X:%02X:%02X -- RSSI: %d dBm\n",len, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], (int) rssi); 
      sem_wait(&semaphore);
      if ((dev_info = find_mac(rssi_list, mac)) == NULL) {
	dev_info = add_element(rssi_list, mac);
      }
      clear_outdated_values(dev_info->measurements);
      add_value(dev_info->measurements, (int) rssi);
      showFromMac(rssi_list);
      sem_post(&semaphore);

    }
  }
  pcap_close(handle);
  pthread_exit((void *) 0);
}

//Check requests from server and send answers
static int answer_to_connection (void *cls, struct MHD_Connection *connection, const char *url, const char *method, const char *version, const char *upload_data, size_t *upload_data_size, void **con_cls)
{
	struct MHD_Response *response;
	int ret;
	
	if (0 == strcmp (url,"/getRssi")) {
		const char *mac = MHD_lookup_connection_value(connection, MHD_GET_ARGUMENT_KIND, "mac");
		char * json = "";
		char * macAp = malloc(sizeof(char)*18);
		macAp = getMacRouter(macAp);
		u_char* mobile =  intmac(mac);

		sem_wait(&semaphore);
		const char * page = build_buffer(rssi_list, json ,macAp, mobile);
		sem_post(&semaphore);

		response = MHD_create_response_from_buffer (strlen (page), (void *) page,MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);
		macAp = NULL;
		free(macAp);
		return ret;
	}
	else if (0 == strcmp (url,"/checkRouter")) {
		const char *page = "{ \"success\" : true }";
		response = MHD_create_response_from_buffer (strlen (page), (void *) page,MHD_RESPMEM_PERSISTENT);
		printf("\nCheck sent");
		ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);
		return ret;
	}else{
		const char *page = "{ \"what\" : true }";
		response = MHD_create_response_from_buffer (strlen (page), (void *) page,MHD_RESPMEM_PERSISTENT);
		ret = MHD_queue_response (connection, MHD_HTTP_OK, response);
		MHD_destroy_response (response);
		return ret;
	}
	return 0;
}

//Convert mac with : to mach without
u_char* intmac(const char* mac)
{
	u_char* final;
	u_char * imac = malloc(sizeof(char)*12);
	
	imac[0] = (char) mac[0];
	imac[1] = (char) mac[1];
	imac[2] = (char) mac[3];
	imac[3] = (char) mac[4];
	imac[4] = (char) mac[6];
	imac[5] = (char) mac[7];
	imac[6] = (char) mac[9];
	imac[7] = (char) mac[10];
	imac[8] = (char) mac[12];
	imac[9] = (char) mac[13];
	imac[10] = (char) mac[15];
	imac[11] = (char) mac[16];

	final = (u_char*) imac;
	return final;
}

char * getMacRouter(char * mac){
	int s;
	struct ifreq buffer;
	s = socket(PF_INET, SOCK_DGRAM, 0);
	memset(&buffer, 0x00, sizeof(buffer));
	strcpy(buffer.ifr_name, "eth0");
	ioctl(s, SIOCGIFHWADDR, &buffer);
	close(s);

	sprintf( mac, "%02x-%02x-%02x-%02x-%02x-%02x", (u_char) buffer.ifr_hwaddr.sa_data[0], (u_char)buffer.ifr_hwaddr.sa_data[1], (u_char)buffer.ifr_hwaddr.sa_data[2], (u_char)buffer.ifr_hwaddr.sa_data[3], (u_char)buffer.ifr_hwaddr.sa_data[4], (u_char)buffer.ifr_hwaddr.sa_data[5]);

	return mac;
}

//_________________________MAIN_______________________
int main ()
{
	rssi_list = malloc(sizeof(Element));
	sem_init(&semaphore, 0, 1);
	thread_params tp;
	tp.interfaces = "wlan0-1";
	pthread_t writer;
	int ret;
	struct MHD_Daemon *daemon;

	daemon = MHD_start_daemon (MHD_USE_SELECT_INTERNALLY, PORT, NULL, NULL, &answer_to_connection, NULL, MHD_OPTION_END);
	if (NULL == daemon)
		return 1;

        ret = pthread_create(&writer, NULL, pcap_function, (void *) &tp);
	pthread_join(writer, (void **) &ret);

	getchar ();
	MHD_stop_daemon (daemon);

	return 0;
}
