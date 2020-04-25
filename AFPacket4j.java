

public class AFPacket4j {
    private static final int AF_PACKET = 17;
    private static final int SOCK_RAW = 3;

    static {
        System.loadLibrary("afpacket4j"); 
    }

    private native void callJniTest();

    private native int socket(int domain, int type);
    private native int bind(int socket, String ifname);
    private native int recv(int socket, byte[] arr);

    public static void main(String[] args) {
        AFPacket4j af4j = new AFPacket4j();
        af4j.callJniTest();

        int socket = af4j.socket(AF_PACKET, SOCK_RAW);
        System.out.println("Socket: " + socket);
        int bindResult = af4j.bind(socket, args[0]);
        System.out.println("Bind result: " + bindResult);
		long start = System.currentTimeMillis();
		long current = System.currentTimeMillis();
		
		byte[] buffer = new byte[9000];   
		long bytes = 0;
		long packets = 0; 
		while (current - start < 30000) {
			int result = af4j.recv(socket, buffer);

			bytes += result;
			packets += 1;
			//System.out.println(byteArrayToHex(buffer, result));
			current = System.currentTimeMillis();
		}
		long timeSecs = (current - start) / 1000; 
		System.out.println("ran for " + timeSecs + " seconds");
		System.out.println("bitrate: " + (bytes / timeSecs));
		System.out.println("packetrate: " + (packets / timeSecs));

		
		
    }

	public static String byteArrayToHex(byte[] array, int len) {
		StringBuilder sb = new StringBuilder(array.length * 2);
		int index = 0;
		for (byte b: array) {
			sb.append(String.format("%02x", b));
			if (index == len) {
				break;
			}
			index++;
		}
		return sb.toString();
	}
    
}
