package bgu.spl.net.impl.stomp;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;

import bgu.spl.net.api.MessageEncoderDecoder;

public class StompEncoderDecoder implements MessageEncoderDecoder<String> {
    
    private final int DEFAULT_MESSAGE_SIZE = 2 << 10; // 1024
    
    byte[] bytes = new byte[DEFAULT_MESSAGE_SIZE];

    private int len = 0;
    
    /**
     * add the next byte to the decoding process
     *
     * @param nextByte the next byte to consider for the currently decoded
     * message
     * @return a message (StompFrame as a String) if this byte completes one or null if it doesnt.
     */
    public String decodeNextByte(byte nextByte) {
        
        if(nextByte == '\u0000' ) {
            return popString();
        }

        pushByte(nextByte);

        return null; // Partial and not full message yet.
    }

    /**
     * encodes the given message to bytes array
     *
     * @param message the message to encode (StompFrame as String INCLUDING the null terminator)
     * @return the encoded bytes
     */
    public byte[] encode(String message) {
        return message.getBytes();
    }   

    private void pushByte(byte nextByte) {
        if (len >= bytes.length) {
            bytes = Arrays.copyOf(bytes, len * 2);
        }

        bytes[len++] = nextByte;
    }

    private String popString() {
        //notice that we explicitly requesting that the string will be decoded from UTF-8
        //this is not actually required as it is the default encoding in java.
        String result = new String(bytes, 0, len, StandardCharsets.UTF_8);
        len = 0;
        return result;
    }
}
