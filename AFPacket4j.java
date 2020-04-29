import java.io.IOException;
import java.nio.ByteBuffer;

public class AFPacket4j {
	private static final int AF_PACKET = 17;
	private static final int SOCK_RAW = 3;
	long start = -1;
	long current = -1;

	long bytes = 0;
	long packets = 0;
	static {
		System.loadLibrary("afpacket4j");
	}

	public AFPacket4j() throws IOException {
		if (init() == -1) {
			throw new IOException("Failed when initializing jni");
		}
	}

	private native int init();

	private native int socket(int domain, int type);

	private native int bind(int socket, String ifname);

	private native int recv(int socket, byte[] arr);

	private native int rx_ring(int socket);

	public static void main(String[] args) throws IOException {
		AFPacket4j af4j = new AFPacket4j();

		int socket = af4j.socket(AF_PACKET, SOCK_RAW);
		int bindResult = af4j.bind(socket, args[0]);

		/*
		 * while (current - start < 30000) { int result = af4j.recv(socket, buffer);
		 * 
		 * bytes += result; packets += 1; //System.out.println(byteArrayToHex(buffer,
		 * result)); current = System.currentTimeMillis(); }
		 */

		af4j.rx_ring(socket);

	}

	void handlePacket(ByteBuffer buffer) {
		// System.out.println("in handler!");
		// System.out.println(buffer.get(14));
		if (start == -1) {
			start = System.currentTimeMillis();
			current = System.currentTimeMillis();

		}
		if (buffer.hasArray()) {
			System.out.println(byteArrayToHex(buffer.array(), buffer.capacity()));

		} else {
			byte[] rawPacket = new byte[buffer.capacity()];
			buffer.get(rawPacket, 0, rawPacket.length);
			bytes += rawPacket.length;
			packets += 1;
			// System.out.println(byteArrayToHex(rawPacket, rawPacket.length));
		}
		current = System.currentTimeMillis();
		if (current - start > 30000) {

			long timeSecs = (current - start) / 1000;
			System.out.println("ran for " + timeSecs + " seconds");
			System.out.println("bitrate: " + ((bytes * 8) / timeSecs));
			System.out.println("packetrate: " + (packets / timeSecs));
			System.exit(1);
		}

	}

	public static String byteArrayToHex(byte[] array, int len) {
		StringBuilder sb = new StringBuilder(array.length * 2);
		int index = 0;
		for (byte b : array) {
			sb.append(String.format("%02x ", b));
			if (index == len) {
				break;
			}
			index++;
		}
		return sb.toString();
	}

}
