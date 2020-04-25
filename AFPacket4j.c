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
#include <sys/mman.h>
#include <assert.h>

// 4194304 bytes
unsigned int blocksize = 1 << 22;
// 2048 bytes
unsigned int framesize = 1 << 11;
unsigned int blocknumber = 64;

JNIEXPORT void JNICALL Java_AFPacket4j_callJniTest(JNIEnv *env, jobject thisObj)
{
   printf("Hello World!\n");
   return;
}

JNIEXPORT jint JNICALL Java_AFPacket4j_socket(JNIEnv *env, jobject thisobj, jint domain, jint type, jboolean ring)
{
   int socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
   if (socket_fd == -1)
   {
      fprintf(stderr, "failed socket() call: %s\n", strerror(errno));
   }
   if (ring) // rx ring setup
   {
      printf("ring is set\n");
      // set packet version
      int version = TPACKET_V3; // use the best version by default
      // TODO implement support for the other versions later
      int set_tpacket_opt_result = setsockopt(socket_fd, SOL_PACKET, PACKET_VERSION, &version, sizeof(version));
      if (set_tpacket_opt_result == -1)
      {
         fprintf(stderr, "failed setsockopt() call: %s\n", strerror(errno));
      }

      // setup the ring
      struct tpacket_req3 req;
      memset(&req, 0, sizeof(req));
      uint8_t *map;
      struct iovec *rd;

      req.tp_block_size = blocksize;
      req.tp_frame_size = framesize;
      req.tp_block_nr = blocknumber;
      req.tp_frame_nr = (blocksize * blocknumber) / framesize;
      req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;
      req.tp_retire_blk_tov = 60; // timeout in ms

      int set_ring_opt_result = setsockopt(socket_fd, SOL_PACKET, PACKET_RX_RING, &req, sizeof(req));
      if (set_ring_opt_result == -1)
      {
         fprintf(stderr, "failed setsockopt() call: %s\n", strerror(errno));
      }
      map = mmap(NULL, req.tp_block_nr * req.tp_block_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, socket_fd, 0);
      if (map == MAP_FAILED)
      {
         perror("mmap");
      }
      rd = malloc(req.tp_block_nr * sizeof(*rd));
      assert(rd);
      for (int i = 0; i < req.tp_block_nr; ++i)
      {
         rd[i].iov_base = map + (i * req.tp_block_size);
         rd[i].iov_len = req.tp_block_size;
      }
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

JNIEXPORT jint JNICALL Java_AFPacket4j_rxRing(JNIEnv *env, jobject thisobj, jint socket)
{
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
