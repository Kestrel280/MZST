package com.example.androidserver;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.charset.Charset;

public class ClientMessage {
    public static final int PacketSize = 8;

    public static final byte MTYPE_REGISTER_USER = 1;
    public static final byte MTYPE_REGISTER_NODE = 2;
    public static final byte MTYPE_REGISTER_TRANSMITTER = 3;

    public static final byte MTYPE_TRIGGER = 100;
    public static final byte MTYPE_TIMESTAMPRESET = 66;
    public static final byte MTYPE_ERROR = 33;

    public static final byte MTYPE_ACK = 111; // If we send a "REQ_ACK" message, node should reply with this

    public short code;
    public short id;
    public int data;

    ClientMessage(char[] raw) {

        ByteBuffer bb = Charset.forName("ASCII").encode(CharBuffer.wrap(raw));
        bb.order(ByteOrder.LITTLE_ENDIAN);

        this.code = bb.getShort(0);
        this.id = bb.getShort(2);
        this.data = bb.getInt(4); // TODO Something wrong here, probably related to sign
    }
}
