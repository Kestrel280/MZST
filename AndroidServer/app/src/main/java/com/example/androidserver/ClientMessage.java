package com.example.androidserver;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.charset.Charset;

public class ClientMessage {
    public static final int PacketSize = 8;

    public static final byte TRIGGERED  = 100;
    public static final byte ERROR      = 33;

    public short type;
    public short id;
    public long timestamp;

    ClientMessage(char[] raw) {

        ByteBuffer bb = Charset.forName("ASCII").encode(CharBuffer.wrap(raw));
        bb.order(ByteOrder.LITTLE_ENDIAN);

        this.type = bb.getShort(0);
        this.id = bb.getShort(2);
        this.timestamp = bb.getInt(4); // TODO Something wrong here, probably related to sign
    }
}
