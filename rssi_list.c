#include "rssi_list.h"
unsigned long long getRealTime(){
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
	return milliseconds;
}

unsigned long long getTime(){
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    unsigned long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000+1000; // caculate milliseconds
	return milliseconds;
}

void showFromMac(Element * list){
	Element * tmp = list->next;
	int i = 0;
	while(tmp != NULL){
      		printf("\n\nvalue %02X:%02X:%02X:%02X:%02X:%02X\n", tmp->mac_addr[0], tmp->mac_addr[1], tmp->mac_addr[2], tmp->mac_addr[3], tmp->mac_addr[4], tmp->mac_addr[5]); 
		showFromRssi(tmp->measurements);
		tmp = tmp->next;
		i++;	
	}
}

void showFromRssi(Deque * list){
	Rssi_sample * tmp = list->head;
	int i = 0;
	while(tmp != NULL){
                printf("[%d] = (%f , %llu) , ",i,tmp->rssi_mW,tmp->deadline);
        	tmp = tmp->next;
		i++;
	}
	printf("\n\n");
}

u_char *string_to_mac(char * buf, u_char * byte_max){
	buf = byte_max;
	return buf;
}

char * mac_to_string(u_char * mac, char * buf){
    sprintf(buf, "%02x-%02x-%02x-%02x-%02x-%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return buf;
}

void clear_values(Deque * list){
	Rssi_sample * tmp = list->head;
	Rssi_sample * removed = NULL;
	while (tmp != NULL){
		removed = tmp;
		tmp = tmp->next;
		removed = NULL;
		free(removed);		
	}
}

void clear_outdated_values(Deque * list){
	if(list == NULL)
		return;

	Rssi_sample * tmp = list->head;
	Rssi_sample * previous = list->head;
	Rssi_sample * removed = NULL;

	while (tmp != NULL){
		if(tmp->deadline <= getRealTime()){
			removed = tmp;
			if(tmp == list->head){
				tmp = tmp->next;
				list->head = tmp;
				removed = NULL;
				free(removed);
			}else if(tmp == list->tail){
				list->tail = previous;
				removed = NULL;
				free(removed);
			}else{
				previous->next = tmp->next;
				removed = NULL;
				free(removed);
			}
		}
		previous = tmp;
		if(tmp != NULL){
			tmp = tmp->next;
		}
	}
}

void add_value(Deque * list, int value){
	Rssi_sample * tmp = list->head;
	Rssi_sample * new = calloc(1,sizeof(Rssi_sample));
	new->next = NULL;
	new->rssi_mW = value;
	new->deadline = getTime();	
	printf("dead : %llu, time : %llu \n",getTime(),getRealTime());

	if(tmp == NULL){
		list->head = new;
	}
	else{
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		if(tmp->deadline < new->deadline){
			tmp->next = new;
			list->tail = new;
		}else{
			new = NULL;
			free(new);
		}
	}
}

void clear_list(Element * list){
	Element * tmp = list;
	Element * removed = NULL;
	while(tmp != NULL){
		removed = tmp;
		tmp = tmp->next;
		clear_outdated_values(removed->measurements);
		removed = NULL;
		free(removed);
	}
}

Element * find_mac(Element * list, u_char * mac_value){
	Element * tmp = list->next;
	while(tmp != NULL){
		if(isSameMac(tmp->mac_addr, mac_value) == 1){
			printf("ENTER \n");
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

Element * find_mac2(Element * list, u_char * mac_value){
	Element * tmp = list->next;
	while(tmp != NULL){
		if(isSameMac2(tmp->mac_addr, mac_value) == 1){
			return tmp;
		}
		tmp = tmp->next;
	}
	return NULL;
}

Element * add_element(Element * list, u_char * mac){
	int i = 0;
	Element * tmp = list;
	Element * new = calloc(1,sizeof(Element));
	new->measurements = calloc(1,sizeof(Deque));
	new->next = NULL;

	for(i = 0; i < 6; i++){
		new->mac_addr[i] = mac[i];
	}

	if(tmp == NULL){
		list = NULL;
		free(list);
		list = new;
	}else{
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = new;
	}
	return new;
}

void delete_element(Element * list, Element * e){
	Element * tmp = list;
	Element * previous = list;
	Element * removed = NULL;
	while(tmp != NULL){
		if(tmp == e){
			removed = tmp;
			if(tmp == list){
				list = tmp->next;
			}
			tmp = tmp->next;
			list = tmp;
			removed = NULL;
			free(removed);
		}
		previous = tmp;
		tmp = tmp->next;
	}
}


void clear_empty_macs(Element * list){
       Element * tmp = list;
        Element * previous = list;
        Element * removed = NULL;
        while(tmp != NULL){
                if(tmp->measurements->head == NULL){
                        removed = tmp;
                        if(tmp == list){
                                list = tmp->next;
                        }
                        tmp = tmp->next;
                        list = tmp;
			removed = NULL;
                        free(removed);
                }
                previous = tmp;
                tmp = tmp->next;
        }
	
}

char * build_element(Element * e, char * buf){
	Rssi_sample * tmp = NULL;
	int number = 0;
	double rssi_value = 0;

	if(e != NULL){
		tmp = e->measurements->head;
	}else
		tmp = NULL;

	while(tmp!=NULL){
		rssi_value = rssi_value + tmp->rssi_mW;
		number++;
		tmp = tmp->next;
	}

	if(number == 0)
		rssi_value = 0;
	else
		rssi_value = rssi_value / number;
	
	char *number_inter = calloc(1,sizeof(number));
	char *rssi_inter = calloc(1,sizeof(rssi_value));
	
	snprintf(number_inter,sizeof(number),"%d",number);
	snprintf(rssi_inter,sizeof(rssi_value),"%d",(int)rssi_value);
	
	buf = concatenate(buf, rssi_inter);
	buf = concatenate(buf,"\", \"samples\" : \"");
	buf = concatenate(buf, number_inter);
	buf = concatenate(buf, "\"}");
	number_inter;
	rssi_inter;
	return buf;
}

char * build_buffer(Element * list, char * buffer, char * my_name, char * macs_requested){
	int i = 0,l = 0;
	char mac[12];
	char str[18];

        buffer = concatenate(buffer, "{\"ap \": \"");
	buffer = concatenate(buffer,  my_name);
        buffer = concatenate(buffer, "\",\"rssi\":");

	while(l < strlen(macs_requested)){
		while(i < 12){
			mac[i] = macs_requested[i];
			l++;
			i++;
		}
		snprintf( str, sizeof(str), "%c%c-%c%c-%c%c-%c%c-%c%c-%c%c", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5],mac[6],mac[7],mac[8],mac[9],mac[10],mac[11]);
		buffer = concatenate(buffer,"[{ \"mac\" : \"");
		buffer = concatenate(buffer, str);
		buffer = concatenate(buffer, "\", \"value\" : \"");
		buffer = build_element(find_mac2(list,mac), buffer);
		l++;		

		if(l < strlen(macs_requested))
			buffer = concatenate(buffer, "], ");
		else
			buffer = concatenate(buffer, "] "); 
	}
	buffer = concatenate(buffer,"}");
	return buffer;
}

char * build_buffer_full(Element * list, char * buffer, char * my_name){
	Element * tmp = list;
	char * mac_list = "";
	//unsigned short nb_macs = 0;
	
	while(tmp != NULL){
		mac_list = concatenate_u(mac_list, tmp->mac_addr);
		tmp = tmp->next;
	}
	return build_buffer(list,buffer,my_name,mac_list);
}

char * concatenate(char * str1, char * str2){
        char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
	int i = 0,l = 0;
	for(i = 0; i < strlen(str1); i++){
		str3[i] = str1[i];
	}
	for(l = 0; l < strlen(str2);l++){
		str3[i+l] = str2[l];
	}
	return str3;
}

u_char * concatenate_u(u_char * str1, u_char * str2){
        u_char * str3 = (char *) malloc(1 + strlen(str1)+ strlen(str2) );
	int i = 0,l = 0;
	for(i = 0; i < strlen(str1); i++){
		str3[i] = str1[i];
	}
	for(l = 0; l < strlen(str2);l++){
		str3[i+l] = str2[l];
	}
	return str3;
}

int isSameMac(u_char * str1, u_char * str2){
	int i = 0;
	for(i = 0; i < 6; i++){
		if(str1[i] != str2[i])
			return 0;
	}
	return 1;
}

int isSameMac2(u_char * str1, u_char * str2){
	int i = 0;
	char str[] = "000000000000";
	snprintf( str, sizeof( str), "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x", str1[0], str1[1], str1[2], str1[3], str1[4], str1[5]);
	printf("S1 : %s \n",str);
	printf("S2 : %s \n\n",str2);
	for(i = 0; i < 12; i++){
		if(str[i] != str2[i])
			return 0;
	}
	return 1;
}
