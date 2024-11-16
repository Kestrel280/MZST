package com.example.androidserver;

import android.content.Context;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;
import android.widget.TextView;

import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity {

    private static TextView debugTv;
    private WifiManager wifiManager;

    public static Handler uiMessageHandler = new Handler(Looper.getMainLooper()) {
        @Override
        public void handleMessage(Message inputMessage) {
            //Log.i("mainThread", "Main thread received message " + inputMessage.getData().getCharSequence("MESSAGE"));
            debugTv.setText(inputMessage.getData().getCharSequence("MESSAGE"));
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        debugTv = findViewById(R.id.debugText);
        debugTv.setText("Waiting for client");

        wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);

        Server server = new Server(wifiManager, Server.PORT);

        Log.i("main", "Main activity done");
    }
}