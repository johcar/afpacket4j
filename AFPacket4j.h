/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class AFPacket4j */

#ifndef _Included_AFPacket4j
#define _Included_AFPacket4j
#ifdef __cplusplus
extern "C" {
#endif
#undef AFPacket4j_AF_PACKET
#define AFPacket4j_AF_PACKET 17L
#undef AFPacket4j_SOCK_RAW
#define AFPacket4j_SOCK_RAW 3L
/*
 * Class:     AFPacket4j
 * Method:    callJniTest
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_AFPacket4j_callJniTest
  (JNIEnv *, jobject);

/*
 * Class:     AFPacket4j
 * Method:    socket
 * Signature: (II)I
 */
JNIEXPORT jint JNICALL Java_AFPacket4j_socket
  (JNIEnv *, jobject, jint, jint);

/*
 * Class:     AFPacket4j
 * Method:    bind
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_AFPacket4j_bind
  (JNIEnv *, jobject, jint);

/*
 * Class:     AFPacket4j
 * Method:    recv
 * Signature: (I[B)I
 */
JNIEXPORT jint JNICALL Java_AFPacket4j_recv
  (JNIEnv *, jobject, jint, jbyteArray);

#ifdef __cplusplus
}
#endif
#endif
