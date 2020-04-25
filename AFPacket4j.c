#include <jni.h>        // JNI header provided by JDK
#include <stdio.h>      // C Standard IO Header
#include "AFPacket4j.h" // Generated

#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_addr.h>
#include <linux/if.h>
#include <errno.h>
JNIEXPORT void JNICALL Java_AFPacket4j_callJniTest(JNIEnv *env, jobject thisObj)
{
   printf("Hello World!\n");
   return;
}

JNIEXPORT jint JNICALL Java_AFPacket4j_socket(JNIEnv *env, jobject thisobj, jint domain, jint type)
{
   int socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
   if (socket_fd == -1)
   {
      fprintf(stderr, "failed socket() call: %s\n", strerror(errno));
   }
   return socket_fd;
}

JNIEXPORT jint JNICALL Java_AFPacket4j_bind(JNIEnv *env, jobject thisobj, jint socket, jstring ifname)
{
   struct sockaddr_ll bind_address;
   memset(&bind_address, 0, sizeof(bind_address));

   const char *ifnamec = (*env)->GetStringUTFChars(env, ifname, NULL);

   int interface_number = get_interface_number_by_device_name(socket, ifnamec);
   bind_address.sll_family = AF_PACKET;
   bind_address.sll_protocol = htons(ETH_P_ALL);
   bind_address.sll_ifindex = interface_number;
   int bind_result = bind(socket, (struct sockaddr *)&bind_address, sizeof(bind_address));
   if (bind_result == -1)
   {
      fprintf(stderr, "failed bind() call: %s\n", strerror(errno));
   }
   return bind_result;
}

JNIEXPORT jint JNICALL Java_AFPacket4j_recv(JNIEnv *env, jobject thisobj, jint socket, jbyteArray buffer)
{
   jbyte *buf;
   int result;

   buf = (*env)->GetByteArrayElements(env, buffer, NULL);
   result = recvfrom(socket, buf, 9000, 0, NULL, NULL);
   (*env)->ReleaseByteArrayElements(env, buffer, buf, 0);

   return result;
}

// Get interface number by name
int get_interface_number_by_device_name(int socket_fd, const char *interface)
{
   struct ifreq ifr;
   memset(&ifr, 0, sizeof(ifr));

   strncpy(ifr.ifr_name, interface, sizeof(ifr.ifr_name));

   if (ioctl(socket_fd, SIOCGIFINDEX, &ifr) == -1)
   {
      return -1;
   }

   return ifr.ifr_ifindex;
}
