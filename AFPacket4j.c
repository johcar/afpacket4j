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
#include <poll.h>
#include <signal.h>
#include <linux/ip.h>

// 4194304 bytes
unsigned int blocksize = 1 << 22;
// 2048 bytes
unsigned int framesize = 1 << 11;
unsigned int blocknumber = 64;

// ring descriptor
struct ring
{
   struct iovec *rd;
   uint8_t *map;
   struct tpacket_req3 req;
};

// block descriptor
struct block_desc
{
   uint32_t version;
   uint32_t offset_to_priv;
   struct tpacket_hdr_v1 h1;
};

// this is the callback to java
static jmethodID packet_callback;
// method signature
static char *signature_string = "(Ljava/nio/ByteBuffer;)V";
JNIEXPORT jint JNICALL Java_AFPacket4j_init(JNIEnv *env, jobject thisObj)
{
   printf("Hello World!\n");
   jclass class = (*env)->GetObjectClass(env, thisObj);
   // Init - One time to initialize the method id, (use an init() function)
   packet_callback = (*env)->GetMethodID(env, class, "handlePacket", signature_string);
   if (packet_callback == 0)
   {
      printf("unable to find method id\n");
      return -1;
   }
   else
   {
      printf("method id is %d\n", packet_callback);
   }
   return 1;
}

JNIEXPORT jint JNICALL Java_AFPacket4j_socket(JNIEnv *env, jobject thisobj, jint domain, jint type)
{
   int socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
   if (socket_fd == -1)
   {
      fprintf(stderr, "failed socket() call: %s\n", strerror(errno));
      return -1;
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
      return -1;
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

JNIEXPORT jint JNICALL Java_AFPacket4j_rx_1ring(JNIEnv *env, jobject thisobj, jint socket)
{
   unsigned int blocknum = 0;
   unsigned int blocks = 64;

   // ring setup
   struct ring ring;
   memset(&ring, 0, sizeof(ring));
   rx_ring_setup(&ring, socket);
   // packet block descriptor struct
   struct block_desc *pbd;
   // pollfd struct setup
   struct pollfd pfd;
   memset(&pfd, 0, sizeof(pfd));
   pfd.fd = socket;
   pfd.events = POLLIN | POLLERR;
   pfd.revents = 0;

   while (1)
   {
      pbd = (struct block_desc *)ring.rd[blocknum].iov_base;
      if ((pbd->h1.block_status & TP_STATUS_USER) == 0)
      {
         poll(&pfd, 1, -1);
         continue;
      }
      walk_block(env, thisobj, pbd, blocknum);
      pbd->h1.block_status = TP_STATUS_KERNEL; // flush block
      blocknum = (blocknum + 1) % blocks;
   }
}

void walk_block(JNIEnv *env, jobject thisobj, struct block_desc *pbd, const int block_num)
{
   int num_pkts = pbd->h1.num_pkts, i;
   unsigned long bytes = 0;
   struct tpacket3_hdr *ppd;

   ppd = (struct tpacket3_hdr *)((uint8_t *)pbd +
                                 pbd->h1.offset_to_first_pkt);
   for (i = 0; i < num_pkts; ++i)
   {
      bytes += ppd->tp_snaplen;
      //display(ppd);
      handlePacket(env, thisobj, ((uint8_t *)ppd + ppd->tp_mac), ppd->tp_len); //bring JNIEnv etc here...
      // TODO java callback using directallocated bytebuffer?

      ppd = (struct tpacket3_hdr *)((uint8_t *)ppd +
                                    ppd->tp_next_offset);
   }

   //packets_total += num_pkts;
   //bytes_total += bytes;
}

void handlePacket(JNIEnv *env, jobject o, uint8_t *buffer, unsigned int len)
{
   jobject bytebuffer = (*env)->NewDirectByteBuffer(env, (void *)buffer, len);
   (*env)->CallVoidMethod(env, o, packet_callback, bytebuffer);
}

void display(struct tpacket3_hdr *ppd)
{
   struct ethhdr *eth = (struct ethhdr *)((uint8_t *)ppd + ppd->tp_mac);
   struct iphdr *ip = (struct iphdr *)((uint8_t *)eth + ETH_HLEN);

   if (eth->h_proto == htons(ETH_P_IP))
   {
      struct sockaddr_in ss, sd;
      char sbuff[1025], dbuff[1025];

      memset(&ss, 0, sizeof(ss));
      ss.sin_family = PF_INET;
      ss.sin_addr.s_addr = ip->saddr;
      getnameinfo((struct sockaddr *)&ss, sizeof(ss),
                  sbuff, sizeof(sbuff), NULL, 0, 1);

      memset(&sd, 0, sizeof(sd));
      sd.sin_family = PF_INET;
      sd.sin_addr.s_addr = ip->daddr;
      getnameinfo((struct sockaddr *)&sd, sizeof(sd),
                  dbuff, sizeof(dbuff), NULL, 0, 1);

      printf("%s -> %s, ", sbuff, dbuff);
   }

   printf("rxhash: 0x%x\n", ppd->hv1.tp_rxhash);
}

int rx_ring_setup(struct ring *ring, int socket_fd)
{
   printf("ring is set\n");
   // set packet version
   int version = TPACKET_V3; // use the best version by default
   // TODO implement support for the other versions later
   int set_tpacket_opt_result = setsockopt(socket_fd, SOL_PACKET, PACKET_VERSION, &version, sizeof(version));
   if (set_tpacket_opt_result == -1)
   {
      fprintf(stderr, "failed setsockopt() call: %s\n", strerror(errno));
      return set_tpacket_opt_result;
   }

   // setup tpacket req3
   memset(&ring->req, 0, sizeof(ring->req));

   ring->req.tp_block_size = blocksize;
   ring->req.tp_frame_size = framesize;
   ring->req.tp_block_nr = blocknumber;
   ring->req.tp_frame_nr = (blocksize * blocknumber) / framesize;
   ring->req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;
   ring->req.tp_retire_blk_tov = 60; // timeout in ms

   int set_ring_opt_result = setsockopt(socket_fd, SOL_PACKET, PACKET_RX_RING, &ring->req, sizeof(ring->req));
   if (set_ring_opt_result == -1)
   {
      fprintf(stderr, "failed setsockopt() call: %s\n", strerror(errno));
   }
   ring->map = mmap(NULL, ring->req.tp_block_nr * ring->req.tp_block_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, socket_fd, 0);
   if (ring->map == MAP_FAILED)
   {
      perror("mmap");
      return -1;
   }
   ring->rd = malloc(ring->req.tp_block_nr * sizeof(*ring->rd));
   assert(ring->rd);
   for (int i = 0; i < ring->req.tp_block_nr; ++i)
   {
      ring->rd[i].iov_base = ring->map + (i * ring->req.tp_block_size);
      ring->rd[i].iov_len = ring->req.tp_block_size;
   }
   return 1;
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
