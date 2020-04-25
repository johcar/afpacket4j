
all: java c


java: AFPacket4j.java
	javac -h . AFPacket4j.java

c: AFPacket4j.c AFPacket4j.h
	gcc -fPIC -I"${JAVA_HOME}/include" -I"${JAVA_HOME}/include/linux" -shared -o libafpacket4j.so AFPacket4j.c

clean:
	rm -f *.class
	rm -f *.so
